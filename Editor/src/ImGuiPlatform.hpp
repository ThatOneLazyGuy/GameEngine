#pragma once

#include <imgui.h>
#include <string>

#include <Tools/Types.hpp>


namespace ImGui
{
    class Platform
    {
      public:
        Platform() = default;
        virtual ~Platform() = default;

        virtual void NewFrame() = 0;
        virtual void EndFrame() = 0;

        virtual void RescaleFramebuffer(sint32 width, sint32 height) = 0;
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