#include "Core/Renderer.hpp"
#include "Renderer.hpp"

#include "Core/Model.hpp"
#include "Core/Window.hpp"

#include <SDL3/SDL.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
#include <string>

namespace
{
    SDL_GPUDevice* device = nullptr;
    SDL_GPUGraphicsPipeline* pipeline = nullptr;

    SDL_GPUColorTargetInfo color_target_info{
        .mip_level = 0,
        .clear_color = {0.0f, 1.0f, 0.0f, 1.0f},
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE
    };

    SDL_GPUTransferBuffer* CreateUploadTransferBuffer(const void* upload_data, const size_t data_size)
    {
        SDL_GPUTransferBufferCreateInfo transfer_buffer_info{.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD};

        transfer_buffer_info.size = data_size;
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
} // namespace

Mesh SDL3GPURenderer::CreateMesh(const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices)
{
    Mesh mesh{.vertices = std::move(vertices), .indices = std::move(indices)};
    const auto vertices_size = static_cast<std::uint32_t>(vertices.size() * sizeof(Vertex));
    const auto indices_size = static_cast<std::uint32_t>(indices.size() * sizeof(std::uint32_t));
    auto* device = static_cast<SDL_GPUDevice*>(Renderer::Instance().GetContext());

    SDL_GPUBufferCreateInfo buffer_info{};

    buffer_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    buffer_info.size = vertices_size;
    mesh.render_vertices = SDL_CreateGPUBuffer(device, &buffer_info);
    if (mesh.render_vertices == nullptr)
    {
        SDL_Log("Failed to create vertex buffer");
        return std::move(mesh);
    }


    buffer_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    buffer_info.size = indices_size;
    mesh.render_indices = SDL_CreateGPUBuffer(device, &buffer_info);
    if (mesh.render_indices == nullptr)
    {
        SDL_Log("Failed to create index buffer");
        return std::move(mesh);
    }

    ReloadMesh(mesh);

    return std::move(mesh);
}

void SDL3GPURenderer::DeleteMesh(Mesh& mesh)
{
    auto* device = static_cast<SDL_GPUDevice*>(GetContext());

    SDL_ReleaseGPUBuffer(device, static_cast<SDL_GPUBuffer*>(mesh.render_vertices));
    SDL_ReleaseGPUBuffer(device, static_cast<SDL_GPUBuffer*>(mesh.render_indices));
}

void SDL3GPURenderer::ReloadMesh(Mesh& mesh)
{
    const auto vertices_size = static_cast<std::uint32_t>(mesh.vertices.size() * sizeof(Vertex));
    const auto indices_size = static_cast<std::uint32_t>(mesh.indices.size() * sizeof(std::uint32_t));
    auto* device = static_cast<SDL_GPUDevice*>(GetContext());

    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUTransferBufferLocation buffer_location{};
    SDL_GPUBufferRegion buffer_region{};

    SDL_GPUTransferBuffer* vertex_transfer_buffer = CreateUploadTransferBuffer(mesh.vertices.data(), vertices_size);
    SDL_GPUTransferBuffer* index_transfer_buffer = CreateUploadTransferBuffer(mesh.indices.data(), indices_size);

    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);

    buffer_location.transfer_buffer = vertex_transfer_buffer;
    buffer_region.buffer = static_cast<SDL_GPUBuffer*>(mesh.render_vertices);
    buffer_region.size = vertices_size;
    SDL_UploadToGPUBuffer(copy_pass, &buffer_location, &buffer_region, false);

    buffer_location.transfer_buffer = index_transfer_buffer;
    buffer_region.buffer = static_cast<SDL_GPUBuffer*>(mesh.render_indices);
    buffer_region.size = indices_size;
    SDL_UploadToGPUBuffer(copy_pass, &buffer_location, &buffer_region, false);

    SDL_EndGPUCopyPass(copy_pass);

    if (!SDL_SubmitGPUCommandBuffer(command_buffer)) { SDL_Log("Failed to submit copy command buffer to load model"); }

    SDL_ReleaseGPUTransferBuffer(device, vertex_transfer_buffer);
    SDL_ReleaseGPUTransferBuffer(device, index_transfer_buffer);
}

SDL_GPUShader* LoadShader(SDL_GPUDevice* device, const std::string& shader_filename);

void SDL3GPURenderer::Init(void* const native_window)
{
    auto* window = static_cast<SDL_Window*>(native_window);

    device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, true, nullptr
    );

    if (device == nullptr) SDL_Log("GPUCreateDevice failed");

    if (!SDL_ClaimWindowForGPUDevice(device, window)) SDL_Log("GPUClaimWindow failed: %s", SDL_GetError());

    SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_MAILBOX);

    // Create the shaders
    SDL_GPUShader* vertex_shader = LoadShader(device, "Assets/Shaders/Shader.vert");
    if (vertex_shader == nullptr) SDL_Log("Failed to create vertex shader!");

    SDL_GPUShader* fragment_shader = LoadShader(device, "Assets/Shaders/Shader.frag");
    if (fragment_shader == nullptr) SDL_Log("Failed to create fragment shader!");

    // Create the pipelines
    const SDL_GPUColorTargetDescription color_target_description[] = {
        {.format = SDL_GetGPUSwapchainTextureFormat(device, window)}
    };

    const SDL_GPUGraphicsPipelineTargetInfo target_info{
        .color_target_descriptions = color_target_description,
        .num_color_targets = 1,
    };

    const SDL_GPUVertexBufferDescription vertex_buffer_descriptions[]{
        {.slot = 0, .pitch = sizeof(float) * 8, .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX}
    };

    const SDL_GPUVertexAttribute vertex_attributes[]{
        {.location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = 0                },
        {.location = 1, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = sizeof(float) * 3},
        {.location = 2, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = sizeof(float) * 6}
    };


    const SDL_GPUVertexInputState vertex_input_state{
        .vertex_buffer_descriptions = vertex_buffer_descriptions,
        .num_vertex_buffers = 1,
        .vertex_attributes = vertex_attributes,
        .num_vertex_attributes = 3
    };

    const SDL_GPUDepthStencilState depth_stencil_state{
        .compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL, .enable_depth_test = true, .enable_depth_write = true
    };
    const SDL_GPURasterizerState rasterizer_state{
        .cull_mode = SDL_GPU_CULLMODE_BACK, .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
    };

    SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info{
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .vertex_input_state = vertex_input_state,
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state = rasterizer_state,
        .depth_stencil_state = depth_stencil_state,
        .target_info = target_info
    };
    pipeline_create_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;

    pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_create_info);
    if (pipeline == nullptr) SDL_Log("Failed to create fill pipeline!");

    // Clean up shader resources
    SDL_ReleaseGPUShader(device, vertex_shader);
    SDL_ReleaseGPUShader(device, fragment_shader);

    color_target_info.clear_color = {0.0f, 0.0f, 0.0f, 1.0f};
    color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    color_target_info.store_op = SDL_GPU_STOREOP_STORE;
}

void SDL3GPURenderer::Exit()
{
    auto* window = static_cast<SDL_Window*>(Window::GetHandle());

    SDL_ReleaseGPUGraphicsPipeline(device, pipeline);

    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
}

void SDL3GPURenderer::Update()
{
    static Model loaded_model{"Assets/cube_with_vertexcolors.obj"};

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

    auto model = glm::mat4(1.0f);

    const size_t time = SDL_GetTicks();
    model = glm::translate(model, glm::vec3{0.5f, -0.5f, 1.5f});
    model = glm::rotate(model, static_cast<float>(time) / 600.0f, glm::vec3{0.0f, 1.0f, 0.0f});

    const float width = static_cast<float>(Window::GetWidth());
    const float height = static_cast<float>(Window::GetHeight());

    const glm::vec3 cameraPos{0.0f, Renderer::size_test[1], Renderer::size_test[0]};
    const glm::mat4 view = glm::lookAt(cameraPos, cameraPos + glm::vec3{0.0f, 0.0f, 1.0f}, glm::vec3{0.0f, 1.0f, 0.0f});
    const glm::mat4 projection = glm::perspective(glm::radians(45.0f), width / height, 0.1f, 100.0f);

    const glm::mat4 mvp = projection * view * model;

    SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, nullptr);
    SDL_BindGPUGraphicsPipeline(render_pass, pipeline);

    const SDL_GPUBufferBinding vertex_binding{
        .buffer = static_cast<SDL_GPUBuffer*>(loaded_model.meshes[0]->render_vertices)
    };
    SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_binding, 1);

    const SDL_GPUBufferBinding index_binding{
        .buffer = static_cast<SDL_GPUBuffer*>(loaded_model.meshes[0]->render_indices)
    };
    SDL_BindGPUIndexBuffer(render_pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

    SDL_PushGPUVertexUniformData(command_buffer, 0, glm::value_ptr(mvp), sizeof(glm::mat4));
    SDL_DrawGPUIndexedPrimitives(render_pass, loaded_model.meshes[0]->indices.size(), 1, 0, 0, 0);

    SDL_EndGPURenderPass(render_pass);

    if (!SDL_SubmitGPUCommandBuffer(command_buffer))
    {
        SDL_LogError(SDL_LOG_PRIORITY_ERROR, "Error submitting render texture command buffer: \n%s", SDL_GetError());
        return;
    }
}

void SDL3GPURenderer::SwapBuffer() { color_target_info.texture = nullptr; }

void* SDL3GPURenderer::GetContext() { return device; }
void* SDL3GPURenderer::GetTexture() { return &color_target_info.texture; }

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

    size_t codeSize;
    void* code = SDL_LoadFile(fullPath.c_str(), &codeSize);
    if (code == nullptr)
    {
        SDL_Log("Failed to load shader from disk! %s", fullPath);
        return nullptr;
    }

    const std::uint32_t uniforms = (stage == SDL_GPU_SHADERSTAGE_VERTEX ? 1 : 0);

    SDL_GPUShaderCreateInfo shaderInfo{
        .code_size = codeSize,
        .code = static_cast<const Uint8*>(code),
        .entrypoint = entrypoint,
        .format = format,
        .stage = stage,
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