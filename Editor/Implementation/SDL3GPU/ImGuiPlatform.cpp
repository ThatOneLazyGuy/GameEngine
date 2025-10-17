#include "ImGuiPlatform.hpp"

#include <Core/Rendering/Renderer.hpp>
#include <Core/Window.hpp>
#include <Platform/PC/SDL3GPU/Rendering/Renderer.hpp>

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_render.h>

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlgpu3.h>

namespace ImGui
{
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
    }

    PlatformSDL3GPU::~PlatformSDL3GPU()
    {
        auto* device = static_cast<SDL_GPUDevice*>(Renderer::Instance().GetContext());

        ImGui_ImplSDL3_Shutdown();
        SDL_WaitForGPUIdle(device);
        ImGui_ImplSDLGPU3_Shutdown();
    }

    void PlatformSDL3GPU::NewFrame() { ImGui_ImplSDLGPU3_NewFrame(); }

    void PlatformSDL3GPU::EndFrame()
    {
        auto* window = static_cast<SDL_Window*>(Window::GetHandle());

        Render();

        SDL_GPUCommandBuffer* command_buffer = SDL3GPURenderer::GetCommandBuffer();

        SDL_GPUTexture* swapchain_texture;
        if (!SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, nullptr, nullptr))
        {
            SDL_Log("Failed to acquire ImGui swapchain texture: %s", SDL_GetError());
            return;
        }

        if (swapchain_texture != nullptr)
        {
            ImDrawData* draw_data = GetDrawData();

            // This is mandatory: call ImGui_ImplSDLGPU3_PrepareDrawData() to upload the vertex/index buffer!
            ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);

            const SDL_GPUColorTargetInfo target_info{
                .texture = swapchain_texture,
                .clear_color = SDL_FColor{1.0f, 0.0f, 0.0f, 1.0f},
                .load_op = SDL_GPU_LOADOP_CLEAR,
                .store_op = SDL_GPU_STOREOP_STORE
            };

            SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &target_info, 1, nullptr);
            ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);
            SDL_EndGPURenderPass(render_pass);
        }

        UpdatePlatformWindows();
        RenderPlatformWindowsDefault();
    }

    ImTextureID PlatformSDL3GPU::GetTextureID(RenderTarget& target) { return reinterpret_cast<ImTextureID>(&target.render_texture); }
} // namespace ImGui