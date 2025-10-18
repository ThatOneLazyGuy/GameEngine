#include "Core/Rendering/Renderer.hpp"
#include "Renderer.hpp"

#include "Core/ECS.hpp"
#include "Core/Model.hpp"
#include "Core/Window.hpp"
#include "Core/Physics/Physics.hpp"
#include "Core/Physics/DebugRenderer.hpp"

#include <SDL3/SDL.h>

#include <filesystem>
#include <string>

namespace
{
    SDL_GPUCommandBuffer* render_command_buffer;

    struct TextureCopyInfo
    {
        SDL_GPUTextureTransferInfo transfer_info{};
        SDL_GPUTextureRegion region{};
    };
    std::vector<TextureCopyInfo> texture_copies;

    struct BufferCopyInfo
    {
        SDL_GPUTransferBufferLocation transfer_location{};
        SDL_GPUBufferRegion region{};
    };
    std::vector<BufferCopyInfo> buffer_copies;

    SDL_GPUDevice* device = nullptr;
    std::vector<SDL_GPUTexture*> global_textures;

    SDL_GPUTransferBuffer* CreateUploadTransferBuffer(const void* upload_data, const size data_size)
    {
        const SDL_GPUTransferBufferCreateInfo transfer_buffer_info{
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size = static_cast<uint32>(data_size)
        };

        SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_info);
        if (transfer_buffer == nullptr)
        {
            SDL_Log("Failed to create vertex transfer buffer: %s", SDL_GetError());
            return nullptr;
        }

        void* buffer_mapping = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
        if (buffer_mapping == nullptr)
        {
            SDL_Log("Failed to map transfer buffer: %s", SDL_GetError());
            return transfer_buffer;
        }

        memcpy(buffer_mapping, upload_data, data_size);
        SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

        return transfer_buffer;
    }

    void DataUploadPass()
    {
        if (texture_copies.empty() && buffer_copies.empty()) return;

        SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
        if (command_buffer == nullptr)
        {
            SDL_Log("Failed to acquire copy command buffer: %s", SDL_GetError());
            return;
        }

        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);

        for (const auto& [transfer_info, region] : texture_copies)
        {
            SDL_UploadToGPUTexture(copy_pass, &transfer_info, &region, false);
            SDL_ReleaseGPUTransferBuffer(device, transfer_info.transfer_buffer);
        }

        for (const auto& [transfer_location, region] : buffer_copies)
        {
            SDL_UploadToGPUBuffer(copy_pass, &transfer_location, &region, false);
            SDL_ReleaseGPUTransferBuffer(device, transfer_location.transfer_buffer);
        }

        SDL_EndGPUCopyPass(copy_pass);

        texture_copies.clear();
        buffer_copies.clear();

        if (!SDL_SubmitGPUCommandBuffer(command_buffer)) { SDL_Log("Failed to submit copy command buffer: %s", SDL_GetError()); }
    }

    void RenderMesh(SDL_GPURenderPass* render_pass, const Transform& transform, const Handle<Mesh>& mesh_handle)
    {
        SDL_PushGPUVertexUniformData(render_command_buffer, 0, transform.GetMatrix().data(), sizeof(Matrix4));

        uint32 diffuse_count = 0;
        uint32 specular_count = 0;
        for (const auto& texture : mesh_handle->textures)
        {
            uint32 sampler_slot = 0;
            switch (texture->type)
            {
            case Texture::Type::DIFFUSE:
                sampler_slot = diffuse_count++;
                break;

            case Texture::Type::SPECULAR:
                sampler_slot = 3 + specular_count++;
                break;
            }

            const SDL_GPUTextureSamplerBinding binding{
                .texture = texture->texture.sdl3gpu,
                .sampler = texture->sampler.sdl3gpu,
            };

            SDL_BindGPUFragmentSamplers(render_pass, sampler_slot, &binding, 1);
        }

        const SDL_GPUBufferBinding vertex_binding{.buffer = mesh_handle->vertices_buffer.sdl3gpu};
        SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_binding, 1);

        const SDL_GPUBufferBinding index_binding{.buffer = mesh_handle->indices_buffer.sdl3gpu};
        SDL_BindGPUIndexBuffer(render_pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

        SDL_DrawGPUIndexedPrimitives(render_pass, static_cast<uint32>(mesh_handle->indices.size()), 1, 0, 0, 0);
    }

    void RenderCamera(const Transform& camera_transform, const Camera& camera)
    {
        auto* window = static_cast<SDL_Window*>(Window::GetHandle());
        const Handle<RenderTarget>& render_target = camera.render_target;

        SDL_GPUColorTargetInfo color_target_info{
            .texture = render_target->render_texture.sdl3gpu,
            .layer_or_depth_plane = 0,
            .load_op = SDL_GPU_LOADOP_LOAD,
            .store_op = SDL_GPU_STOREOP_STORE
        };

        if (color_target_info.texture == nullptr &&
            !SDL_WaitAndAcquireGPUSwapchainTexture(render_command_buffer, window, &color_target_info.texture, nullptr, nullptr))
        {
            SDL_Log("Failed to acquire swapchain texture: %s", SDL_GetError());
            return;
        }

        SDL_GPUDepthStencilTargetInfo depth_stencil_target_info{
            .texture = render_target->depth_texture.sdl3gpu,
            .clear_depth = 1.0f,
            .load_op = SDL_GPU_LOADOP_LOAD,
            .store_op = SDL_GPU_STOREOP_STORE,
        };

        if (render_target->clear)
        {
            // Memory copy the clear color (identical memory layout)
            std::memcpy(&color_target_info.clear_color, render_target->clear_color.data(), sizeof(float4));

            color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            depth_stencil_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
        }

        SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(render_command_buffer, &color_target_info, 1, &depth_stencil_target_info);
        SDL_BindGPUGraphicsPipeline(render_pass, Resource::GetResources<ShaderPipeline>()[0]->shader_pipeline.sdl3gpu);

        const Matrix4 view = Math::Inverse(camera_transform.GetMatrix());
        SDL_PushGPUVertexUniformData(render_command_buffer, 1, view.data(), sizeof(Matrix4));

        const Matrix4 projection = camera.GetProjection(*render_target);
        SDL_PushGPUVertexUniformData(render_command_buffer, 2, projection.data(), sizeof(Matrix4));

        const auto mesh_query = ECS::GetWorld().query_builder<const Transform, const Handle<Mesh>>().build();
        mesh_query.each([render_pass](const Transform& transform, const Handle<Mesh>& mesh_handle) {
            RenderMesh(render_pass, transform, mesh_handle);
        });

        for (const auto& [model, mesh] : Physics::RenderDebug(camera_transform.GetPosition()))
        {
            SDL_PushGPUVertexUniformData(render_command_buffer, 0, &model, sizeof(Matrix4));

            uint32 diffuse_count = 0;
            uint32 specular_count = 0;
            for (const auto& texture : mesh->textures)
            {
                uint32 sampler_slot = 0;
                switch (texture->type)
                {
                case Texture::Type::DIFFUSE:
                    sampler_slot = diffuse_count++;
                    break;

                case Texture::Type::SPECULAR:
                    sampler_slot = 3 + specular_count++;
                    break;
                }

                const SDL_GPUTextureSamplerBinding binding{
                    .texture = texture->texture.sdl3gpu,
                    .sampler = texture->sampler.sdl3gpu,
                };

                SDL_BindGPUFragmentSamplers(render_pass, sampler_slot, &binding, 1);
            }

            const SDL_GPUBufferBinding vertex_binding{.buffer = mesh->vertices_buffer.sdl3gpu};
            SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_binding, 1);

            const SDL_GPUBufferBinding index_binding{.buffer = mesh->indices_buffer.sdl3gpu};
            SDL_BindGPUIndexBuffer(render_pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

            SDL_DrawGPUIndexedPrimitives(render_pass, static_cast<uint32>(mesh->indices.size()), 1, 0, 0, 0);
        }

        SDL_EndGPURenderPass(render_pass);
    }
} // namespace

void SDL3GPURenderer::Init()
{
    auto* window = static_cast<SDL_Window*>(Window::GetHandle());

    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, true, "vulkan");

    if (device == nullptr)
    {
        SDL_Log("Failed to create SDL3GPU device: %s", SDL_GetError());
        return;
    }

    if (!SDL_ClaimWindowForGPUDevice(device, window))
    {
        SDL_Log("Failed to claim window for SDL3GPU: %s", SDL_GetError());
        return;
    }

    if (!SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_MAILBOX))
    {
        // If we fail to set mailbox (v-sync with less visual latency) we use regular v-sync.
        SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);
    }
    Resource::Load<ShaderPipeline>("Assets/Shaders/Shader.vert.spv", "Assets/Shaders/Shader.frag.spv");
}

void SDL3GPURenderer::Exit()
{
    auto* window = static_cast<SDL_Window*>(Window::GetHandle());

    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
}

void SDL3GPURenderer::Update()
{
    DataUploadPass();

    render_command_buffer = SDL_AcquireGPUCommandBuffer(device);
    if (render_command_buffer == nullptr)
    {
        SDL_Log("Failed to acquire render command buffer: %s", SDL_GetError());
        return;
    }

    const auto camera_query = ECS::GetWorld().query_builder<const Transform, const Camera>().build();
    camera_query.each(&RenderCamera);
}

void SDL3GPURenderer::SwapBuffer()
{
    if (!SDL_SubmitGPUCommandBuffer(render_command_buffer))
    {
        SDL_Log("Failed to submit render command buffer: %s", SDL_GetError());
    }
    render_command_buffer = nullptr;
}

void* SDL3GPURenderer::GetContext() { return device; }

SDL_GPUCommandBuffer* SDL3GPURenderer::GetCommandBuffer() { return render_command_buffer; }

void SDL3GPURenderer::CreateRenderTarget(RenderTarget& target)
{
    constexpr SDL_GPUSamplerCreateInfo sampler_info{
        SDL_GPU_FILTER_NEAREST,
        SDL_GPU_FILTER_NEAREST,
        SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
        SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE
    };

    target.sampler.sdl3gpu = SDL_CreateGPUSampler(device, &sampler_info);
    if (target.sampler.sdl3gpu == nullptr) SDL_Log("Failed creating sampler for render target: %s", SDL_GetError());

    RecreateRenderTarget(target);
}

void SDL3GPURenderer::RecreateRenderTarget(RenderTarget& target)
{
    if (target.sampler.sdl3gpu != nullptr)
    {
        SDL_ReleaseGPUTexture(device, target.render_texture.sdl3gpu);

        static SDL_GPUTextureCreateInfo render_texture_info{
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = SDL_GetGPUSwapchainTextureFormat(device, static_cast<SDL_Window*>(Window::GetHandle())),
            .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
            .layer_count_or_depth = 1,
            .num_levels = 1
        };

        render_texture_info.width = target.GetWidth();
        render_texture_info.height = target.GetHeight();

        target.render_texture.sdl3gpu = SDL_CreateGPUTexture(device, &render_texture_info);
        if (target.render_texture.sdl3gpu == nullptr) SDL_LogError(SDL_LOG_PRIORITY_ERROR, "Error creating the render texture");
    }

    SDL_ReleaseGPUTexture(device, target.depth_texture.sdl3gpu);

    static SDL_GPUTextureCreateInfo depth_texture_info{
        .format = SDL_GPU_TEXTUREFORMAT_D24_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .layer_count_or_depth = 1,
        .num_levels = 1
    };

    depth_texture_info.width = target.GetWidth();
    depth_texture_info.height = target.GetHeight();

    target.depth_texture.sdl3gpu = SDL_CreateGPUTexture(device, &depth_texture_info);
    if (target.depth_texture.sdl3gpu == nullptr) SDL_LogError(SDL_LOG_PRIORITY_ERROR, "Error creating the depth texture");
}

void SDL3GPURenderer::DestroyRenderTarget(RenderTarget& target)
{
    SDL_ReleaseGPUTexture(device, target.depth_texture.sdl3gpu);
    SDL_ReleaseGPUTexture(device, target.render_texture.sdl3gpu);
    SDL_ReleaseGPUSampler(device, target.sampler.sdl3gpu);
}

void SDL3GPURenderer::CreateTexture(Texture& texture)
{
    constexpr SDL_GPUSamplerCreateInfo sampler_info{
        SDL_GPU_FILTER_NEAREST,
        SDL_GPU_FILTER_NEAREST,
        SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
        SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE
    };
    texture.sampler.sdl3gpu = SDL_CreateGPUSampler(device, &sampler_info);
    if (texture.sampler.sdl3gpu == nullptr) SDL_LogError(SDL_LOG_PRIORITY_ERROR, "Error creating the texture sampler");

    ReloadTexture(texture);
}
void SDL3GPURenderer::ReloadTexture(Texture& texture)
{
    if (texture.texture.sdl3gpu != nullptr) SDL_ReleaseGPUTexture(device, texture.texture.sdl3gpu);

    const SDL_GPUTextureCreateInfo texture_info{
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = texture.width,
        .height = texture.height,
        .layer_count_or_depth = 1,
        .num_levels = 1
    };

    texture.texture.sdl3gpu = SDL_CreateGPUTexture(device, &texture_info);
    if (texture.texture.sdl3gpu == nullptr)
    {
        SDL_LogError(SDL_LOG_PRIORITY_ERROR, "Error creating the target texture");
        return;
    }

    global_textures.push_back(texture.texture.sdl3gpu);

    SDL_GPUTransferBuffer* texture_transfer_buffer =
        CreateUploadTransferBuffer(texture.colors.data(), texture.colors.size() * sizeof(uint32));

    auto& [texture_transfer_info, texture_destination] = texture_copies.emplace_back();

    texture_transfer_info.transfer_buffer = texture_transfer_buffer;
    texture_transfer_info.offset = 0;
    texture_transfer_info.pixels_per_row = texture.width;
    texture_transfer_info.rows_per_layer = texture.height;

    texture_destination.texture = texture.texture.sdl3gpu;
    texture_destination.w = texture.width;
    texture_destination.h = texture.height;
    texture_destination.d = 1;
}

void SDL3GPURenderer::DestroyTexture(Texture& texture)
{
    SDL_ReleaseGPUTexture(device, texture.texture.sdl3gpu);
    SDL_ReleaseGPUSampler(device, texture.sampler.sdl3gpu);
}

void SDL3GPURenderer::CreateMesh(Mesh& mesh)
{
    const auto vertices_size = static_cast<uint32>(mesh.vertices.size() * sizeof(Vertex));
    const auto indices_size = static_cast<uint32>(mesh.indices.size() * sizeof(uint32));

    SDL_GPUBufferCreateInfo buffer_info{};

    buffer_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    buffer_info.size = vertices_size;
    mesh.vertices_buffer.sdl3gpu = SDL_CreateGPUBuffer(device, &buffer_info);
    if (mesh.vertices_buffer.sdl3gpu == nullptr)
    {
        SDL_Log("Failed to create vertex buffer");
        return;
    }

    buffer_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    buffer_info.size = indices_size;
    mesh.indices_buffer.sdl3gpu = SDL_CreateGPUBuffer(device, &buffer_info);
    if (mesh.indices_buffer.sdl3gpu == nullptr)
    {
        SDL_Log("Failed to create index buffer");
        return;
    }

    ReloadMesh(mesh);
}

void SDL3GPURenderer::ReloadMesh(Mesh& mesh)
{
    const auto vertices_size = static_cast<uint32>(mesh.vertices.size() * sizeof(Vertex));
    const auto indices_size = static_cast<uint32>(mesh.indices.size() * sizeof(uint32));

    SDL_GPUTransferBuffer* vertex_transfer_buffer = CreateUploadTransferBuffer(mesh.vertices.data(), vertices_size);
    SDL_GPUTransferBuffer* index_transfer_buffer = CreateUploadTransferBuffer(mesh.indices.data(), indices_size);

    auto& [vertices_buffer_location, vertices_buffer_region] = buffer_copies.emplace_back();
    vertices_buffer_location.transfer_buffer = vertex_transfer_buffer;
    vertices_buffer_region.buffer = mesh.vertices_buffer.sdl3gpu;
    vertices_buffer_region.size = vertices_size;

    auto& [indices_buffer_location, indices_buffer_region] = buffer_copies.emplace_back();
    indices_buffer_location.transfer_buffer = index_transfer_buffer;
    indices_buffer_region.buffer = mesh.indices_buffer.sdl3gpu;
    indices_buffer_region.size = indices_size;
}

void SDL3GPURenderer::DestroyMesh(Mesh& mesh)
{
    SDL_ReleaseGPUBuffer(device, mesh.vertices_buffer.sdl3gpu);
    SDL_ReleaseGPUBuffer(device, mesh.indices_buffer.sdl3gpu);
}

void SDL3GPURenderer::CreateShader(Shader& shader)
{
    const auto stage = static_cast<SDL_GPUShaderStage>(shader.type);

    const SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(device);
    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
    const char* entrypoint;

    if (backendFormats & SDL_GPU_SHADERFORMAT_SPIRV)
    {
        format = SDL_GPU_SHADERFORMAT_SPIRV;
        entrypoint = "main";
    }
    else if (backendFormats & SDL_GPU_SHADERFORMAT_MSL)
    {
        format = SDL_GPU_SHADERFORMAT_MSL;
        entrypoint = "main0";
    }
    else if (backendFormats & SDL_GPU_SHADERFORMAT_DXIL)
    {
        format = SDL_GPU_SHADERFORMAT_DXIL;
        entrypoint = "main";
    }
    else
    {
        SDL_Log("%s", "Unrecognized backend shader format!");
        return;
    }

    const SDL_GPUShaderCreateInfo shaderInfo{
        .code_size = shader.code.size(),
        .code = reinterpret_cast<const uint8*>(shader.code.data()),
        .entrypoint = entrypoint,
        .format = format,
        .stage = stage,
        .num_samplers = shader.sampler_count,
        .num_storage_buffers = shader.storage_count,
        .num_uniform_buffers = shader.uniform_count
    };

    shader.shader.sdl3gpu = SDL_CreateGPUShader(device, &shaderInfo);
    if (shader.shader.sdl3gpu == nullptr) SDL_Log("Failed to create shader: %s", SDL_GetError());
}

void SDL3GPURenderer::DestroyShader(Shader& shader) { SDL_ReleaseGPUShader(device, shader.shader.sdl3gpu); }

void SDL3GPURenderer::CreateShaderPipeline(
    ShaderPipeline& pipeline, const Handle<Shader>& vertex_shader, const Handle<Shader>& fragment_shader
)
{
    auto* window = static_cast<SDL_Window*>(Window::GetHandle());

    constexpr SDL_GPUColorTargetBlendState blend_state{
        .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
        .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        .color_blend_op = SDL_GPU_BLENDOP_ADD,
        .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
        .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
        .enable_blend = true
    };

    const SDL_GPUColorTargetDescription color_target_description{
        .format = SDL_GetGPUSwapchainTextureFormat(device, window), .blend_state = blend_state
    };

    const SDL_GPUGraphicsPipelineTargetInfo target_info{
        .color_target_descriptions = &color_target_description,
        .num_color_targets = 1,
        .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM,
        .has_depth_stencil_target = true
    };

    static constexpr SDL_GPUVertexBufferDescription vertex_buffer_description{
        .slot = 0, .pitch = sizeof(float) * 8, .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX
    };

    static constexpr SDL_GPUVertexAttribute vertex_attributes[3]{
        {.location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = 0                },
        {.location = 1, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = sizeof(float) * 3},
        {.location = 2, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = sizeof(float) * 6}
    };

    constexpr SDL_GPUVertexInputState vertex_input_state{
        .vertex_buffer_descriptions = &vertex_buffer_description,
        .num_vertex_buffers = 1,
        .vertex_attributes = vertex_attributes,
        .num_vertex_attributes = sizeof(vertex_attributes) / sizeof(SDL_GPUVertexAttribute)
    };

    constexpr SDL_GPUDepthStencilState depth_stencil_state{
        .compare_op = SDL_GPU_COMPAREOP_LESS,
        .enable_depth_test = true,
        .enable_depth_write = true,
    };
    constexpr SDL_GPURasterizerState rasterizer_state{
        .fill_mode = SDL_GPU_FILLMODE_FILL,
        .cull_mode = SDL_GPU_CULLMODE_BACK,
        .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
        .enable_depth_clip = true
    };

    const SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info{
        .vertex_shader = vertex_shader->shader.sdl3gpu,
        .fragment_shader = fragment_shader->shader.sdl3gpu,
        .vertex_input_state = vertex_input_state,
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state = rasterizer_state,
        .depth_stencil_state = depth_stencil_state,
        .target_info = target_info
    };

    pipeline.shader_pipeline.sdl3gpu = SDL_CreateGPUGraphicsPipeline(device, &pipeline_create_info);
    if (pipeline.shader_pipeline.sdl3gpu == nullptr) SDL_Log("Failed to create shader pipeline: %s", SDL_GetError());
}

void SDL3GPURenderer::DestroyShaderPipeline(ShaderPipeline& shader)
{
    SDL_ReleaseGPUGraphicsPipeline(device, shader.shader_pipeline.sdl3gpu);
}