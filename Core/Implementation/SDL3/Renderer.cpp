#include "Core/Renderer.hpp"
#include "Renderer.hpp"

#include "Core/Model.hpp"
#include "Core/Window.hpp"

#include <SDL3/SDL.h>

#include <filesystem>
#include <string>

namespace
{
    SDL_GPUDevice* device = nullptr;
    Shader shader;
    std::vector<SDL_GPUTexture*> global_textures;

    SDL_GPUColorTargetInfo color_target_info{
        .layer_or_depth_plane = 0,
        .clear_color = {0.75f, 0.81f, 0.4f, 1.0f},
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE
    };

    SDL_GPUTexture* depth_texture;
    SDL_GPUDepthStencilTargetInfo depth_stencil_target_info{
        .clear_depth = true,
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
    };

    SDL_GPUTransferBuffer* CreateUploadTransferBuffer(const void* upload_data, const size data_size)
    {
        SDL_GPUTransferBufferCreateInfo transfer_buffer_info{
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

    SDL_GPUShader* LoadShader(SDL_GPUDevice*, const std::string& shader_filename)
    {
        // Auto-detect the shader stage from the file name for convenience
        SDL_GPUShaderStage stage;
        if (shader_filename.find(".vert") != std::string::npos) stage = SDL_GPU_SHADERSTAGE_VERTEX;
        else if (shader_filename.find(".frag") != std::string::npos) stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
        else
        {
            SDL_Log("Invalid shader stage!");
            return nullptr;
        }

        SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(device);
        SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
        const char* entrypoint;

        std::string fullPath = std::filesystem::absolute(shader_filename).generic_string();
        if (backendFormats & SDL_GPU_SHADERFORMAT_SPIRV)
        {
            fullPath += ".spv";
            format = SDL_GPU_SHADERFORMAT_SPIRV;
            entrypoint = "main";
        }
        else if (backendFormats & SDL_GPU_SHADERFORMAT_MSL)
        {
            fullPath += ".msl";
            format = SDL_GPU_SHADERFORMAT_MSL;
            entrypoint = "main0";
        }
        else if (backendFormats & SDL_GPU_SHADERFORMAT_DXIL)
        {
            fullPath += ".dxil";
            format = SDL_GPU_SHADERFORMAT_DXIL;
            entrypoint = "main";
        }
        else
        {
            SDL_Log("%s", "Unrecognized backend shader format!");
            return nullptr;
        }

        size codeSize;
        void* code = SDL_LoadFile(fullPath.c_str(), &codeSize);
        if (code == nullptr)
        {
            SDL_Log("Failed to load shader from disk! %s", fullPath);
            return nullptr;
        }

        const uint32 samplers = (stage == SDL_GPU_SHADERSTAGE_VERTEX ? 0 : 1);
        constexpr uint32 storages = 0;
        const uint32 uniforms = (stage == SDL_GPU_SHADERSTAGE_VERTEX ? 3 : 0);

        const SDL_GPUShaderCreateInfo shaderInfo{
            .code_size = codeSize,
            .code = static_cast<const Uint8*>(code),
            .entrypoint = entrypoint,
            .format = format,
            .stage = stage,
            .num_samplers = samplers,
            .num_storage_buffers = storages,
            .num_uniform_buffers = uniforms
        };
        SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shaderInfo);
        if (shader == nullptr)
        {
            SDL_Log("Failed to create shader: %s", SDL_GetError());
            SDL_free(code);

            return nullptr;
        }

        SDL_free(code);
        return shader;
    }
} // namespace

void SDL3GPURenderer::Init(void* window_handle)
{
    auto* window = static_cast<SDL_Window*>(window_handle);

    device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, true, nullptr
    );

    if (device == nullptr) SDL_Log("GPUCreateDevice failed");

    if (!SDL_ClaimWindowForGPUDevice(device, window))
    {
        SDL_Log("GPUClaimWindow failed: %s", SDL_GetError());
        return;
    }

    SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_MAILBOX);

    shader = CreateShader("Assets/Shaders/Shader.vert", "Assets/Shaders/Shader.frag");
}

void SDL3GPURenderer::Exit()
{
    auto* window = static_cast<SDL_Window*>(Window::GetHandle());

    SDL_ReleaseGPUTexture(device, depth_texture);
    SDL_ReleaseGPUGraphicsPipeline(device, shader.pipeline);

    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
}

void SDL3GPURenderer::Update()
{
    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
    if (command_buffer == nullptr)
    {
        SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        return;
    }

    if (color_target_info.texture == nullptr)
    {
        if (!SDL_WaitAndAcquireGPUSwapchainTexture(
                command_buffer,
                static_cast<SDL_Window*>(Window::GetHandle()),
                &color_target_info.texture,
                nullptr,
                nullptr
            ))
        {
            SDL_Log("WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
            return;
        }
    }

    if (color_target_info.texture == nullptr)
    {
        SDL_SubmitGPUCommandBuffer(command_buffer);
        return;
    }

    auto model = Identity<Mat4>();

    const size time = SDL_GetTicks();
    model *= Rotation(static_cast<float>(time) / 600.0f, Vec3{0.0f, 1.0f, 0.0f});
    model *= Translation(Vec3{0.5f, -0.5f, -2.5f});

    const auto width = static_cast<float>(Window::GetWidth());
    const auto height = static_cast<float>(Window::GetHeight());

    const Vec3 cameraPos{position};
    const Mat4 view = LookAt(cameraPos, Vec3{0.0f, 0.0f, -1.0f}, Vec3{0.0f, 1.0f, 0.0f});
    const Mat4 projection = PerspectiveZO(ToRadians(fov), width / height, 0.1f, 100.0f);

    depth_stencil_target_info.texture = depth_texture;
    SDL_GPURenderPass* render_pass =
        SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, &depth_stencil_target_info);
    SDL_BindGPUGraphicsPipeline(render_pass, shader.pipeline);

    SDL_PushGPUVertexUniformData(command_buffer, 0, model.data(), sizeof(Mat4));
    SDL_PushGPUVertexUniformData(command_buffer, 1, view.data(), sizeof(Mat4));
    SDL_PushGPUVertexUniformData(command_buffer, 2, projection.data(), sizeof(Mat4));

    for (const auto& mesh_handle : Resource::GetResources<Mesh>())
    {
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
                .texture = texture->texture,
                .sampler = texture->sampler,
            };

            SDL_BindGPUFragmentSamplers(render_pass, sampler_slot, &binding, 1);
        }

        const SDL_GPUBufferBinding vertex_binding{.buffer = mesh_handle->vertices_buffer};
        SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_binding, 1);

        const SDL_GPUBufferBinding index_binding{.buffer = mesh_handle->indices_buffer};
        SDL_BindGPUIndexBuffer(render_pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

        SDL_DrawGPUIndexedPrimitives(render_pass, mesh_handle->indices.size(), 1, 0, 0, 0);
    }

    SDL_EndGPURenderPass(render_pass);

    if (!SDL_SubmitGPUCommandBuffer(command_buffer))
    {
        SDL_LogError(SDL_LOG_PRIORITY_ERROR, "Error submitting render texture command buffer: \n%s", SDL_GetError());
    }
}

void SDL3GPURenderer::SwapBuffer() { color_target_info.texture = nullptr; }

void* SDL3GPURenderer::GetContext() { return device; }

void SDL3GPURenderer::CreateMesh(
    Mesh& mesh, const std::vector<Vertex>& vertices, const std::vector<uint32>& indices,
    const std::vector<Handle<Texture>>& textures
)
{
    mesh.vertices = vertices;
    mesh.indices = indices;
    mesh.textures = textures;

    const auto vertices_size = static_cast<uint32>(vertices.size() * sizeof(Vertex));
    const auto indices_size = static_cast<uint32>(indices.size() * sizeof(uint32));

    SDL_GPUBufferCreateInfo buffer_info{};

    buffer_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    buffer_info.size = vertices_size;
    mesh.vertices_buffer = SDL_CreateGPUBuffer(device, &buffer_info);
    if (mesh.vertices_buffer == nullptr)
    {
        SDL_Log("Failed to create vertex buffer");
        return;
    }

    buffer_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    buffer_info.size = indices_size;
    mesh.indices_buffer = SDL_CreateGPUBuffer(device, &buffer_info);
    if (mesh.indices_buffer == nullptr)
    {
        SDL_Log("Failed to create index buffer");
        return;
    }

    ReloadMesh(mesh);
}

void SDL3GPURenderer::DeleteMesh(Mesh& mesh)
{
    auto* device = static_cast<SDL_GPUDevice*>(GetContext());

    SDL_ReleaseGPUBuffer(device, mesh.vertices_buffer);
    SDL_ReleaseGPUBuffer(device, mesh.indices_buffer);
}

void SDL3GPURenderer::ReloadMesh(Mesh& mesh)
{
    const auto vertices_size = static_cast<uint32>(mesh.vertices.size() * sizeof(Vertex));
    const auto indices_size = static_cast<uint32>(mesh.indices.size() * sizeof(uint32));
    auto* device = static_cast<SDL_GPUDevice*>(GetContext());

    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUTransferBufferLocation buffer_location{};
    SDL_GPUBufferRegion buffer_region{};

    SDL_GPUTransferBuffer* vertex_transfer_buffer = CreateUploadTransferBuffer(mesh.vertices.data(), vertices_size);
    SDL_GPUTransferBuffer* index_transfer_buffer = CreateUploadTransferBuffer(mesh.indices.data(), indices_size);

    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);

    buffer_location.transfer_buffer = vertex_transfer_buffer;
    buffer_region.buffer = mesh.vertices_buffer;
    buffer_region.size = vertices_size;
    SDL_UploadToGPUBuffer(copy_pass, &buffer_location, &buffer_region, false);

    buffer_location.transfer_buffer = index_transfer_buffer;
    buffer_region.buffer = mesh.indices_buffer;
    buffer_region.size = indices_size;
    SDL_UploadToGPUBuffer(copy_pass, &buffer_location, &buffer_region, false);

    SDL_EndGPUCopyPass(copy_pass);

    if (!SDL_SubmitGPUCommandBuffer(command_buffer)) { SDL_Log("Failed to submit copy command buffer to load model"); }

    SDL_ReleaseGPUTransferBuffer(device, vertex_transfer_buffer);
    SDL_ReleaseGPUTransferBuffer(device, index_transfer_buffer);
}

void SDL3GPURenderer::CreateTexture(
    Texture& texture, const uint32 width, const uint32 height, const std::vector<uint32>& colors,
    const Texture::Type type
)
{
    texture.type = type;
    texture.width = width;
    texture.height = height;
    texture.colors = colors;

    constexpr SDL_GPUSamplerCreateInfo sampler_info{
        SDL_GPU_FILTER_NEAREST,
        SDL_GPU_FILTER_NEAREST,
        SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
        SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE
    };
    texture.sampler = SDL_CreateGPUSampler(device, &sampler_info);
    if (texture.sampler == nullptr) SDL_LogError(SDL_LOG_PRIORITY_ERROR, "Error creating the texture sampler");

    ReloadTexture(texture);
}
void SDL3GPURenderer::ReloadTexture(Texture& texture)
{
    if (texture.texture != nullptr) SDL_ReleaseGPUTexture(device, texture.texture);

    const SDL_GPUTextureCreateInfo texture_info{
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = texture.width,
        .height = texture.height,
        .layer_count_or_depth = 1,
        .num_levels = 1
    };

    texture.texture = SDL_CreateGPUTexture(device, &texture_info);
    if (texture.texture == nullptr)
    {
        SDL_LogError(SDL_LOG_PRIORITY_ERROR, "Error creating the target texture");
        return;
    }

    global_textures.push_back(texture.texture);
    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);

    SDL_GPUTransferBuffer* texture_transfer_buffer =
        CreateUploadTransferBuffer(texture.colors.data(), texture.colors.size() * sizeof(uint32));

    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);

    const SDL_GPUTextureTransferInfo texture_transfer_info{
        .transfer_buffer = texture_transfer_buffer,
        .offset = 0,
        .pixels_per_row = texture.width,
        .rows_per_layer = texture.height
    };


    const SDL_GPUTextureRegion texture_destination{
        .texture = texture.texture,
        .w = texture.width,
        .h = texture.height,
        .d = 1,
    };
    SDL_UploadToGPUTexture(copy_pass, &texture_transfer_info, &texture_destination, false);

    SDL_EndGPUCopyPass(copy_pass);

    if (!SDL_SubmitGPUCommandBuffer(command_buffer)) SDL_Log("Failed to submit copy command buffer to load texture!");

    SDL_ReleaseGPUTransferBuffer(device, texture_transfer_buffer);
}

void SDL3GPURenderer::DeleteTexture(Texture& texture)
{
    SDL_ReleaseGPUTexture(device, texture.texture);
    SDL_ReleaseGPUSampler(device, texture.sampler);
}

Shader SDL3GPURenderer::CreateShader(const std::string& vertex_path, const std::string& fragment_path)
{
    auto* window = static_cast<SDL_Window*>(Window::GetHandle());

    // Create the shaders
    SDL_GPUShader* vertex_shader = LoadShader(device, vertex_path);
    if (vertex_shader == nullptr) SDL_Log("Failed to create vertex shader!");

    SDL_GPUShader* fragment_shader = LoadShader(device, fragment_path);
    if (fragment_shader == nullptr) SDL_Log("Failed to create fragment shader!");

    constexpr SDL_GPUColorTargetBlendState blend_state{
        .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
        .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        .color_blend_op = SDL_GPU_BLENDOP_ADD,
        .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
        .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
        .enable_blend = true
    };

    // Create the pipelines
    const SDL_GPUColorTargetDescription color_target_description{
        .format = SDL_GetGPUSwapchainTextureFormat(device, window), .blend_state = blend_state
    };

    const SDL_GPUGraphicsPipelineTargetInfo target_info{
        .color_target_descriptions = &color_target_description,
        .num_color_targets = 1,
        .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM,
        .has_depth_stencil_target = true
    };

    constexpr SDL_GPUVertexBufferDescription vertex_buffer_description{
        .slot = 0, .pitch = sizeof(float) * 8, .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX
    };

    constexpr SDL_GPUVertexAttribute vertex_attributes[3]{
        {.location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = 0                },
        {.location = 1, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = sizeof(float) * 3},
        {.location = 2, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = sizeof(float) * 6}
    };

    const SDL_GPUVertexInputState vertex_input_state{
        .vertex_buffer_descriptions = &vertex_buffer_description,
        .num_vertex_buffers = 1,
        .vertex_attributes = vertex_attributes,
        .num_vertex_attributes = 3
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
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .vertex_input_state = vertex_input_state,
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state = rasterizer_state,
        .depth_stencil_state = depth_stencil_state,
        .target_info = target_info
    };

    Shader shader{.pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_create_info)};
    if (shader.pipeline == nullptr) SDL_Log("Failed to create fill pipeline!");

    // Clean up shader resources
    SDL_ReleaseGPUShader(device, vertex_shader);
    SDL_ReleaseGPUShader(device, fragment_shader);

    return shader;
}
void SDL3GPURenderer::DeleteShader(const Shader shader) { SDL_ReleaseGPUGraphicsPipeline(device, shader.pipeline); }

void SDL3GPURenderer::ReloadDepthBuffer()
{
    if (depth_texture != nullptr) SDL_ReleaseGPUTexture(device, depth_texture);

    const SDL_GPUTextureCreateInfo depth_texture_info{
        SDL_GPU_TEXTURETYPE_2D,
        SDL_GPU_TEXTUREFORMAT_D24_UNORM,
        SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        static_cast<uint32>(Window::GetWidth()),
        static_cast<uint32>(Window::GetHeight()),
        1,
        1,
        SDL_GPU_SAMPLECOUNT_1
    };

    depth_texture = SDL_CreateGPUTexture(device, &depth_texture_info);
    if (depth_texture == nullptr) SDL_LogError(SDL_LOG_PRIORITY_ERROR, "Error creating the depth texture");
}

SDL_GPUColorTargetInfo& SDL3GPURenderer::GetColorTarget() { return color_target_info; }