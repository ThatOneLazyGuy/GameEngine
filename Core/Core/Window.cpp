#include "Window.hpp"

#include "Renderer.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_log.h>

namespace Window
{
    namespace
    {

        std::function<void(const void*)> process_events;

        SDL_Window* window = nullptr;

        size_t width = 1920;
        size_t height = 1080;
    } // namespace

    void Init(const std::function<void(const void*)>& event_process_func)
    {
        process_events = event_process_func;

        SDL_Init(SDL_INIT_VIDEO);

        window = SDL_CreateWindow(
            "Engine",                 // window title
            static_cast<int>(width),  // width, in pixels
            static_cast<int>(height), // height, in pixels
            Renderer::Instance().WindowFlags() | SDL_WINDOW_RESIZABLE
        );

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
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (process_events) process_events(&event);

            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                return true;

            case SDL_EVENT_WINDOW_RESIZED:
                width = event.display.data1;
                height = event.display.data2;
                break;

            default:
                break;
            }
        }

        return false;
    }

    size_t GetWidth() { return width; }
    size_t GetHeight() { return height; }

    void* GetHandle() { return window; }
} // namespace Window