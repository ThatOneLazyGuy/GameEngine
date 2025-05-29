#include "ImGuiPlatform.hpp"

#include "Core/Renderer.hpp"
#include "Core/Window.hpp"

#include <SDL3/SDL_video.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl3.h>

#include <glad/glad.h>

namespace ImGui
{
    namespace
    {
        std::uint32_t frame_buffer, render_buffer, texture_id;
    }

    PlatformOpenGL::PlatformOpenGL()
    {
        auto* window = static_cast<SDL_Window*>(Window::GetHandle());
        auto* context = static_cast<SDL_GLContext*>(Renderer::Instance().GetContext());

        ImGui_ImplSDL3_InitForOpenGL(window, context);
        ImGui_ImplOpenGL3_Init();

        glGenFramebuffers(1, &frame_buffer);
        glGenTextures(1, &texture_id);
        glGenRenderbuffers(1, &render_buffer);
    }

    PlatformOpenGL::~PlatformOpenGL()
    {
        glDeleteRenderbuffers(1, &render_buffer);
        glDeleteTextures(1, &texture_id);
        glDeleteFramebuffers(1, &frame_buffer);

        ImGui_ImplSDL3_Shutdown();
        ImGui_ImplOpenGL3_Shutdown();
    }

    void PlatformOpenGL::NewFrame()
    {
        ImGui_ImplOpenGL3_NewFrame();

        glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
        constexpr unsigned int buffers[1] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, buffers);
    }

    void PlatformOpenGL::EndFrame()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        Render();
        ImGui_ImplOpenGL3_RenderDrawData(GetDrawData());

        auto* backup_current_window = static_cast<SDL_Window*>(Window::GetHandle());
        const auto backup_current_context = *static_cast<SDL_GLContext*>(Renderer::Instance().GetContext());
        UpdatePlatformWindows();
        RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }

    void PlatformOpenGL::RescaleFramebuffer(const int width, const int height)
    {
        glViewport(0, 0, width, height);

        glBindTexture(GL_TEXTURE_2D, texture_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_id, 0);

        glBindRenderbuffer(GL_RENDERBUFFER, render_buffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, render_buffer);
    }

    ImTextureID PlatformOpenGL::GetFramebuffer() { return texture_id; }
} // namespace ImGui