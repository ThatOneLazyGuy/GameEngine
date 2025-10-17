#include "ImGuiPlatform.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>

#include "Core/Renderer.hpp"
#include "Core/Window.hpp"

#include "Implementation/OpenGL/ImGuiPlatform.hpp"
#include "Implementation/SDL3GPU/ImGuiPlatform.hpp"

#include "Core/Input.hpp"

#include <backends/imgui_impl_sdl3.h>
#include <imgui.h>

namespace ImGui
{
    namespace
    {
        Platform* platform;
        //ImVec2 mouse_move_delta{};

    } // namespace

    void PlatformInit(const std::string& backend_name)
    {
        if (backend_name == "OpenGL") platform = new PlatformOpenGL;
        else if (backend_name == "SDL3GPU") platform = new PlatformSDL3GPU;

        Renderer::main_target = Resource::Load<RenderTarget>("EditorWindow");
        Renderer::main_target->clear_color = float4{0.75f, 0.81f, 0.4f, 1.0f};
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

    void PlatformEndFrame() 
    { 
        platform->EndFrame(); 
    }

    void LockMouse(const bool lock)
    {
        SDL_Window* window = static_cast<SDL_Window*>(Window::GetHandle());

        if (lock)
        {
            const ImVec2 mouse_pos = GetMousePos() - GetWindowViewport()->Pos;

            const SDL_Rect rect{static_cast<sint32>(mouse_pos.x), static_cast<sint32>(mouse_pos.y), 1, 1};
            SDL_SetWindowMouseRect(window, &rect);
        }
        else SDL_SetWindowMouseRect(window, nullptr);

        SDL_SetWindowRelativeMouseMode(window, lock);
    }
    bool IsMouseLocked()
    {
        SDL_Window* window = static_cast<SDL_Window*>(Window::GetHandle());
        return SDL_GetWindowRelativeMouseMode(window);
    }

    ImTextureID GetPlatformTextureID(RenderTarget& target) {return platform->GetTextureID(target); }

    void PlatformRescaleGameWindow(const ImVec2 viewport_size)
    {
        const auto width = static_cast<sint32>(viewport_size.x);
        const auto height = static_cast<sint32>(viewport_size.y);

        if (width == Window::GetWidth() && height == Window::GetHeight()) return;
        if (width <= 0 || height <= 0) return;

        SDL_WindowEvent window_resize_event{
            .type = SDL_EVENT_WINDOW_RESIZED,
            .timestamp = SDL_GetTicksNS(),
            .windowID = SDL_GetWindowID(static_cast<SDL_Window*>(Window::GetHandle())),
            .data1 = width,
            .data2 = height
        };
        SDL_PushEvent(reinterpret_cast<SDL_Event*>(&window_resize_event));
    }

    bool PlatformProcessEvent(const void* event)
    {
        const auto* sdl_event = static_cast<const SDL_Event*>(event);

        bool block_event = true;
        switch (sdl_event->type)
        {
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            if (!GetIO().WantCaptureKeyboard) block_event = false;
            else Input::ClearKeys();
            ImGui_ImplSDL3_ProcessEvent(sdl_event);
            break;

        case SDL_EVENT_MOUSE_MOTION:
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
        case SDL_EVENT_MOUSE_WHEEL:
            if (!GetIO().WantCaptureMouse) block_event = false;
            ImGui_ImplSDL3_ProcessEvent(sdl_event);
            break;

        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_QUIT:
            block_event = false;
        default:
            ImGui_ImplSDL3_ProcessEvent(sdl_event);
            break;
        }

        return block_event;
    }
} // namespace ImGui