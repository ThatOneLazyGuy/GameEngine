#include "Window.hpp"

#include "Input.hpp"
#include "Rendering/Renderer.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_log.h>

namespace Window
{
    namespace
    {
        std::function<bool(const void*)> process_events;

        SDL_Window* window = nullptr;

        sint32 width = 1920;
        sint32 height = 1080;
    } // namespace

    void Init(const std::function<bool(const void*)>& event_process_func)
    {
        process_events = event_process_func;

        SDL_Init(SDL_INIT_VIDEO);

        window = SDL_CreateWindow("Engine", width, height, Renderer::Instance().WindowFlags() | SDL_WINDOW_RESIZABLE);

        if (window == nullptr) SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n", SDL_GetError());

        SDL_ShowWindow(window);
    }

    void Exit()
    {
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    bool PollEvents()
    {
        float2 mouse_pos_delta{0.0f, 0.0f};

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (process_events && process_events(&event)) continue;

            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                return true;

            case SDL_EVENT_WINDOW_RESIZED:
            {
                const SDL_WindowEvent& window_event = event.window;
                if (window_event.windowID == SDL_GetWindowID(window))
                {
                    width = window_event.data1;
                    height = window_event.data2;
                    Renderer::main_target->Resize(width, height);
                }
                break;
            }

            case SDL_EVENT_KEY_UP:
            case SDL_EVENT_KEY_DOWN:
            {
                const SDL_KeyboardEvent& keyboard_event = event.key;
                Input::SetKey(static_cast<Input::Key>(keyboard_event.scancode), keyboard_event.down);
                break;
            }

            case SDL_EVENT_MOUSE_MOTION:
            {
                const SDL_MouseMotionEvent& motion_event = event.motion;
                mouse_pos_delta += float2{motion_event.xrel, motion_event.yrel};
                Input::SetMousePos(motion_event.x, motion_event.y);

                break;
            }

            default:
                break;
            }
        }
        Input::SetMouseDelta(mouse_pos_delta.x(), mouse_pos_delta.y());

        return false;
    }

    sint32 GetWidth() { return width; }
    sint32 GetHeight() { return height; }

    void* GetHandle() { return window; }
} // namespace Window