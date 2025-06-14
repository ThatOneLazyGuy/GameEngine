#include "ImGuiPlatform.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>

#include "Core/Renderer.hpp"
#include "Core/Window.hpp"

#include "Implementation/OpenGL/ImGuiPlatform.hpp"
#include "Implementation/SDL3GPU/ImGuiPlatform.hpp"

#include <backends/imgui_impl_sdl3.h>
#include <imgui.h>

namespace ImGui
{
    namespace
    {
        Platform* platform;
        sint32 frame_buffer_width{0};
        sint32 frame_buffer_height{0};

    } // namespace

    void PlatformInit(const std::string& backend_name)
    {
        if (backend_name == "OpenGL") platform = new PlatformOpenGL;
        else if (backend_name == "SDL3GPU") platform = new PlatformSDL3GPU;

        RescaleFramebuffer(ImVec2{1.0f, 1.0f});
    }

    void PlatformExit()
    {
        delete platform;
        DestroyContext();
    }

    void PlatformNewFrame()
    {
        ImGui_ImplSDL3_NewFrame();
        platform->NewFrame();
        NewFrame();
    }

    void PlatformEndFrame() { platform->EndFrame(); }

    void RescaleFramebuffer(const ImVec2 viewport_size)
    {
        const auto width = static_cast<sint32>(viewport_size.x);
        const auto height = static_cast<sint32>(viewport_size.y);

        if (frame_buffer_width == width & frame_buffer_height == height) return;
        if (width <= 0 || height <= 0) return;

        frame_buffer_width = width;
        frame_buffer_width = height;

        SDL_WindowEvent test_event{
            .type = SDL_EVENT_WINDOW_RESIZED,
            .timestamp = SDL_GetTicksNS(),
            .windowID = SDL_GetWindowID(static_cast<SDL_Window*>(Window::GetHandle())),
            .data1 = width,
            .data2 = height
        };
        SDL_PushEvent(reinterpret_cast<SDL_Event*>(&test_event));

        platform->RescaleFramebuffer(width, height);
    }

    ImTextureID GetFramebuffer() { return platform->GetFramebuffer(); }

    void PlatformProcessEvent(const void* event) { ImGui_ImplSDL3_ProcessEvent(static_cast<const SDL_Event*>(event)); }
} // namespace ImGui