#include "ImGuiPlatform.hpp"

#include "Core/Rendering/Renderer.hpp"
#include "Core/Window.hpp"

#include <SDL3/SDL_video.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl3.h>

namespace ImGui
{
    PlatformOpenGL::PlatformOpenGL()
    {
        auto* window = static_cast<SDL_Window*>(Window::GetHandle());
        auto* context = static_cast<SDL_GLContext*>(Renderer::Instance().GetContext());

        ImGui_ImplSDL3_InitForOpenGL(window, context);
        ImGui_ImplOpenGL3_Init();
    }

    PlatformOpenGL::~PlatformOpenGL()
    {
        ImGui_ImplSDL3_Shutdown();
        ImGui_ImplOpenGL3_Shutdown();
    }

    void PlatformOpenGL::NewFrame() { ImGui_ImplOpenGL3_NewFrame(); }

    void PlatformOpenGL::EndFrame()
    {
        Render();
        ImGui_ImplOpenGL3_RenderDrawData(GetDrawData());

        auto* backup_current_window = static_cast<SDL_Window*>(Window::GetHandle());
        const auto backup_current_context = *static_cast<SDL_GLContext*>(Renderer::Instance().GetContext());

        UpdatePlatformWindows();
        RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }

    ImTextureID PlatformOpenGL::GetTextureID(RenderTarget& target) { return target.render_buffers[0].GetTexture()->texture.id; }
} // namespace ImGui