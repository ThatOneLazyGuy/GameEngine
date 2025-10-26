#include "Core/Rendering/Renderer.hpp"
#include "Renderer.hpp"

#include "Core/ECS.hpp"
#include "Core/Window.hpp"
#include "Core/Physics/Physics.hpp"
#include "Core/Physics/DebugRenderer.hpp"
#include "Core/Rendering/RenderPassInterface.hpp"

#include <SDL3/SDL.h>

#include <filesystem>

namespace
{
    SDL_GPUCommandBuffer* render_command_buffer = nullptr;
    SDL_GPURenderPass* active_render_pass = nullptr;

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

    SDL_GPUTextureUsageFlags ToUsageFlags(const uint32 in_flags)
    {
        SDL_GPUTextureUsageFlags out_flags = 0;

        if (in_flags & Texture::SAMPLER) out_flags |= SDL_GPU_TEXTUREUSAGE_SAMPLER;
        if (in_flags & Texture::COLOR_TARGET) out_flags |= SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
        if (in_flags & Texture::DEPTH_TARGET) out_flags |= SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;

        return out_flags;
    }
} // namespace

void SDL3GPURenderer::InitBackend()
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

    const Handle<Shader> vertex_shader = FileResource::Load<Shader>("Assets/Shaders/Shader.vert.spv", ShaderSettings{Shader::VERTEX, 0, 0, 3});
    const Handle<Shader> fragment_shader = FileResource::Load<Shader>("Assets/Shaders/Shader.frag.spv", ShaderSettings{Shader::FRAGMENT, 1, 0, 0});
    Resource::Load<GraphicsShaderPipeline>(vertex_shader, fragment_shader);
}

void SDL3GPURenderer::ExitBackend()
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
}

void SDL3GPURenderer::SwapBuffer()
{
    if (!SDL_SubmitGPUCommandBuffer(render_command_buffer)) { SDL_Log("Failed to submit render command buffer: %s", SDL_GetError()); }
    render_command_buffer = nullptr;
}

void* SDL3GPURenderer::GetContext() { return device; }

void SDL3GPURenderer::RenderMesh(const Mesh& mesh)
{
    const SDL_GPUBufferBinding vertex_binding{.buffer = mesh.vertices_buffer.sdl3gpu};
    SDL_BindGPUVertexBuffers(active_render_pass, 0, &vertex_binding, 1);

    const SDL_GPUBufferBinding index_binding{.buffer = mesh.indices_buffer.sdl3gpu};
    SDL_BindGPUIndexBuffer(active_render_pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

    SDL_DrawGPUIndexedPrimitives(active_render_pass, static_cast<uint32>(mesh.indices.size()), 1, 0, 0, 0);
}

void SDL3GPURenderer::SetTextureSampler(const uint32 slot, const Texture& texture)
{
    const SDL_GPUTextureSamplerBinding binding{
        .texture = texture.texture.sdl3gpu,
        .sampler = texture.sampler.sdl3gpu,
    };

    SDL_BindGPUFragmentSamplers(active_render_pass, slot, &binding, 1);
}

void SDL3GPURenderer::SetUniform(const uint32 slot, const void* data, const size size)
{
    SDL_PushGPUVertexUniformData(render_command_buffer, slot, data, static_cast<uint32>(size));
    SDL_PushGPUFragmentUniformData(render_command_buffer, slot, data, static_cast<uint32>(size));
    SDL_PushGPUComputeUniformData(render_command_buffer, slot, data, static_cast<uint32>(size));
}

SDL_GPUCommandBuffer* SDL3GPURenderer::GetCommandBuffer() { return render_command_buffer; }

void SDL3GPURenderer::BeginRenderPass(const RenderPassInterface& render_pass)
{
    Handle<RenderTarget> render_target = render_pass.render_target;
    std::vector<SDL_GPUColorTargetInfo> color_target_infos;

    if (render_target->render_buffers.empty())
    {
        auto* window = static_cast<SDL_Window*>(Window::GetHandle());

        SDL_GPUTexture* swapchain_texture = nullptr;
        if (!SDL_WaitAndAcquireGPUSwapchainTexture(render_command_buffer, window, &swapchain_texture, nullptr, nullptr))
        {
            SDL_Log("Failed to acquire swapchain texture: %s", SDL_GetError());
            return;
        }

        const SDL_GPUColorTargetInfo color_target_info{
            .texture = swapchain_texture,
            .layer_or_depth_plane = 0,
            .clear_color = SDL_FColor{},
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE
        };

        color_target_infos.push_back(color_target_info);
    }
    else
    {
        color_target_infos.reserve(render_target->render_buffers.size());
        for (RenderBuffer& render_buffer : render_target->render_buffers)
        {
            const float4& clear_color = render_buffer.clear_color;

            const SDL_GPUColorTargetInfo color_target_info{
                .texture = render_buffer.GetTexture()->texture.sdl3gpu,
                .layer_or_depth_plane = 0,
                .clear_color = SDL_FColor{clear_color.x(), clear_color.y(), clear_color.z(), clear_color.w()},
                .load_op = (render_pass.clear_render_targets ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD),
                .store_op = SDL_GPU_STOREOP_STORE
            };

            color_target_infos.push_back(color_target_info);
        }
    }

    const Handle<Texture> depth_texture = render_target->depth_buffer.GetTexture();
    SDL_GPUDepthStencilTargetInfo* depth_stencil_target_info = nullptr;

    if (depth_texture != nullptr)
    {
        depth_stencil_target_info = new SDL_GPUDepthStencilTargetInfo{

            .texture = depth_texture->texture.sdl3gpu,
            .clear_depth = 1.0f,
            .load_op = (render_pass.clear_render_targets ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD),
            .store_op = SDL_GPU_STOREOP_STORE,
        };
    }

    active_render_pass = SDL_BeginGPURenderPass(
        render_command_buffer, color_target_infos.data(), static_cast<uint32>(color_target_infos.size()), depth_stencil_target_info
    );
    delete depth_stencil_target_info;

    SDL_BindGPUGraphicsPipeline(active_render_pass, render_pass.graphics_pipeline->shader_pipeline.sdl3gpu);
}

void SDL3GPURenderer::EndRenderPass()
{
    if (active_render_pass != nullptr)
    {
        SDL_EndGPURenderPass(active_render_pass);
        active_render_pass = nullptr;
    }
}

void SDL3GPURenderer::CreateTexture(Texture& texture, const uint8* data, const SamplerSettings& sampler_settings)
{
    const uint32 width = texture.GetWidth();
    const uint32 height = texture.GetHeight();

    const SDL_GPUTextureFormat format =
        texture.GetFormat() == Texture::COLOR_RGBA_32 ? SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM : SDL_GPU_TEXTUREFORMAT_D24_UNORM;
    const SDL_GPUTextureCreateInfo texture_create_info{
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = format,
        .usage = ToUsageFlags(texture.GetFlags()),
        .width = width,
        .height = height,
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };

    texture.texture.sdl3gpu = SDL_CreateGPUTexture(device, &texture_create_info);
    if (texture.texture.sdl3gpu == nullptr)
    {
        SDL_Log("Failed to create GPU texture: %s", SDL_GetError());
        return;
    }

    const SDL_GPUSamplerCreateInfo sampler_info{
        .min_filter = static_cast<SDL_GPUFilter>(sampler_settings.down_filter),
        .mag_filter = static_cast<SDL_GPUFilter>(sampler_settings.up_filter),
        .mipmap_mode = static_cast<SDL_GPUSamplerMipmapMode>(sampler_settings.mipmap_mode),
        .address_mode_u = static_cast<SDL_GPUSamplerAddressMode>(sampler_settings.wrap_mode_u),
        .address_mode_v = static_cast<SDL_GPUSamplerAddressMode>(sampler_settings.wrap_mode_v),
        .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE
    };

    texture.sampler.sdl3gpu = SDL_CreateGPUSampler(device, &sampler_info);
    if (texture.sampler.sdl3gpu == nullptr) SDL_LogError(SDL_LOG_PRIORITY_ERROR, "Error creating the texture sampler");

    // If there is no data to upload, we return.
    if (data == nullptr) return;

    const uint32 data_size = SDL_CalculateGPUTextureFormatSize(format, width, height, 1);
    SDL_GPUTransferBuffer* texture_transfer_buffer = CreateUploadTransferBuffer(data, data_size);

    auto& [texture_transfer_info, texture_destination] = texture_copies.emplace_back();

    texture_transfer_info.transfer_buffer = texture_transfer_buffer;
    texture_transfer_info.offset = 0;
    texture_transfer_info.pixels_per_row = width;
    texture_transfer_info.rows_per_layer = height;

    texture_destination.texture = texture.texture.sdl3gpu;
    texture_destination.w = width;
    texture_destination.h = height;
    texture_destination.d = 1;
}
void SDL3GPURenderer::ResizeTexture(Texture& texture, const sint32 new_width, const sint32 new_height)
{
    const SDL_GPUTextureFormat format =
        texture.GetFormat() == Texture::COLOR_RGBA_32 ? SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM : SDL_GPU_TEXTUREFORMAT_D24_UNORM;
    const SDL_GPUTextureCreateInfo texture_create_info{
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = format,
        .usage = ToUsageFlags(texture.GetFlags()),
        .width = static_cast<uint32>(new_width),
        .height = static_cast<uint32>(new_height),
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };

    SDL_GPUTexture* new_texture = SDL_CreateGPUTexture(device, &texture_create_info);
    if (new_texture == nullptr)
    {
        SDL_Log("Failed to recreate GPU texture: %s", SDL_GetError());
        return;
    }

    if (texture.texture.sdl3gpu != nullptr && texture.GetFlags() & Texture::COLOR_RGBA_32)
    {
        const SDL_GPUBlitRegion source_region{
            .texture = texture.texture.sdl3gpu,
            .mip_level = 0,
            .x = 0,
            .y = 0,
            .w = static_cast<uint32>(texture.GetWidth()),
            .h = static_cast<uint32>(texture.GetHeight()),
        };

        const SDL_GPUBlitRegion destination_region{
            .texture = new_texture,
            .mip_level = 0,
            .x = 0,
            .y = 0,
            .w = static_cast<uint32>(new_width),
            .h = static_cast<uint32>(new_height),
        };

        const SDL_GPUBlitInfo blit_info{
            .source = source_region,
            .destination = destination_region,
            .load_op = SDL_GPU_LOADOP_DONT_CARE,
            .filter = SDL_GPU_FILTER_NEAREST
        };

        SDL_GPUCommandBuffer* blit_command_buffer = SDL_AcquireGPUCommandBuffer(device);
        SDL_BlitGPUTexture(blit_command_buffer, &blit_info);
        SDL_SubmitGPUCommandBuffer(blit_command_buffer);

        SDL_ReleaseGPUTexture(device, texture.texture.sdl3gpu);
    }

    texture.texture.sdl3gpu = new_texture;
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
    SDL_GPUShaderFormat format;
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
    GraphicsShaderPipeline& pipeline, const Handle<Shader>& vertex_shader, const Handle<Shader>& fragment_shader
)
{
    constexpr SDL_GPUColorTargetBlendState blend_state{
        .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
        .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        .color_blend_op = SDL_GPU_BLENDOP_ADD,
        .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
        .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
        .enable_blend = true
    };

    static constexpr SDL_GPUColorTargetDescription color_target_description{
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM, .blend_state = blend_state
    };

    constexpr SDL_GPUGraphicsPipelineTargetInfo target_info{
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

void SDL3GPURenderer::DestroyShaderPipeline(GraphicsShaderPipeline& pipeline)
{
    SDL_ReleaseGPUGraphicsPipeline(device, pipeline.shader_pipeline.sdl3gpu);
}