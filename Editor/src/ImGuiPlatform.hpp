#pragma once

#include <string>
#include <imgui.h>

namespace ImGui
{
    class Platform
    {
      public:
        Platform() = default;
        virtual ~Platform() = default;

        virtual void NewFrame() = 0;
        virtual void EndFrame() = 0;

        virtual void RescaleFramebuffer(int width, int height) = 0;
        virtual ImTextureID GetFramebuffer() = 0;
    };

    void PlatformInit(const std::string& backend_name);
    void PlatformExit();

    void PlatformNewFrame();
    void PlatformEndFrame();

    void RescaleFramebuffer(ImVec2 viewport_size);
    ImTextureID GetFramebuffer();
    void PlatformProcessEvent(const void* event);
} // namespace ImGui