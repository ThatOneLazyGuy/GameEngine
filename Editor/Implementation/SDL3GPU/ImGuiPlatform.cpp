#include "ImGuiPlatform.hpp"

#include "Core/Renderer.hpp"
#include "Core/Window.hpp"
#include "Implementation/SDL3/Renderer.hpp"

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_log.h"

#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlgpu3.h"

namespace ImGui
{
    namespace
    {
        SDL_GPUTextureSamplerBinding viewport_binding;
        SDL_GPUColorTargetInfo target_info{
            .clear_color = SDL_FColor{1.0f, 0.0f, 0.0f, 1.0f},
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE
        };
    } // namespace

    PlatformSDL3GPU::PlatformSDL3GPU()
    {
        auto* window = static_cast<SDL_Window*>(Window::GetHandle());
        auto* device = static_cast<SDL_GPUDevice*>(Renderer::Instance().GetContext());

        ImGui_ImplSDL3_InitForSDLGPU(window);
        ImGui_ImplSDLGPU3_InitInfo init_info = {};
        init_info.Device = device;
        init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(device, window);
        init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
        ImGui_ImplSDLGPU3_Init(&init_info);

        constexpr SDL_GPUSamplerCreateInfo sampler_info{
            SDL_GPU_FILTER_NEAREST,
            SDL_GPU_FILTER_NEAREST,
            SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
            SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE
        };
        viewport_binding.sampler = SDL_CreateGPUSampler(device, &sampler_info);
        if (viewport_binding.sampler == nullptr)
            SDL_LogError(SDL_LOG_PRIORITY_ERROR, "Error creating the texture sampler");
    }

    PlatformSDL3GPU::~PlatformSDL3GPU()
    {
        auto* device = static_cast<SDL_GPUDevice*>(Renderer::Instance().GetContext());

        SDL_ReleaseGPUTexture(device, viewport_binding.texture);
        SDL_ReleaseGPUSampler(device, viewport_binding.sampler);

        ImGui_ImplSDL3_Shutdown();
        SDL_WaitForGPUIdle(device);
        ImGui_ImplSDLGPU3_Shutdown();
    }

    void PlatformSDL3GPU::NewFrame()
    {
        ImGui_ImplSDLGPU3_NewFrame();

        SDL3GPURenderer::GetColorTarget().texture = viewport_binding.texture;
    }

    void PlatformSDL3GPU::EndFrame()
    {
        auto* window = static_cast<SDL_Window*>(Window::GetHandle());
        auto* device = static_cast<SDL_GPUDevice*>(Renderer::Instance().GetContext());

        Render();
        ImDrawData* draw_data = GetDrawData();
        const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

        SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device); // Acquire a GPU command buffer

        SDL_GPUTexture* swapchain_texture;
        if (!SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, nullptr, nullptr))
        {
            SDL_Log("Error acquiring imgui swapchain texture: %s", SDL_GetError());
            SDL_SubmitGPUCommandBuffer(command_buffer);
            return;
        }

        if (swapchain_texture != nullptr && !is_minimized)
        {
            // This is mandatory: call ImGui_ImplSDLGPU3_PrepareDrawData() to upload the vertex/index buffer!
            ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);

            // Setup and start a render pass
            target_info.texture = swapchain_texture;
            SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &target_info, 1, nullptr);

            // Render ImGui
            ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);

            SDL_EndGPURenderPass(render_pass);
        }

        // Submit the command buffer
        if (!SDL_SubmitGPUCommandBuffer(command_buffer))
        {
            SDL_Log("Error submitting imgui command buffer: %s", SDL_GetError());
        }

        UpdatePlatformWindows();
        RenderPlatformWindowsDefault();
    }

    void PlatformSDL3GPU::RescaleFramebuffer(const int width, const int height)
    {
        auto* window = static_cast<SDL_Window*>(Window::GetHandle());
        auto* device = static_cast<SDL_GPUDevice*>(Renderer::Instance().GetContext());

        if (viewport_binding.texture != nullptr) SDL_ReleaseGPUTexture(device, viewport_binding.texture);

        const SDL_GPUTextureCreateInfo texture_info{
            SDL_GPU_TEXTURETYPE_2D,
            SDL_GetGPUSwapchainTextureFormat(device, window),
            SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
            static_cast<uint32>(width),
            static_cast<uint32>(height),
            1,
            1,
            SDL_GPU_SAMPLECOUNT_1
        };

        viewport_binding.texture = SDL_CreateGPUTexture(device, &texture_info);
        if (viewport_binding.texture == nullptr)
        {
            SDL_LogError(SDL_LOG_PRIORITY_ERROR, "Error creating the target texture");
            return;
        }

        SDL3GPURenderer::GetColorTarget().texture = viewport_binding.texture;
        SDL3GPURenderer::ReloadDepthBuffer();
    }

    ImTextureID PlatformSDL3GPU::GetFramebuffer() { return reinterpret_cast<ImTextureID>(&viewport_binding); }
} // namespace ImGui