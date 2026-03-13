#pragma once

#include <imgui.h>
#include <string>

class RenderTarget;

namespace ImGui
{
    class Platform
    {
      public:
        Platform() = default;
        virtual ~Platform() = default;

        virtual void NewFrame() = 0;
        virtual void EndFrame() = 0;

        virtual ImTextureID GetTextureID(RenderTarget& target) = 0;
    };

    void PlatformInit(const std::string& backend_name);
    void PlatformExit();

    void PlatformNewFrame();
    void PlatformEndFrame();

    void LockMouse(bool lock);
    bool IsMouseLocked();

    ImTextureID GetPlatformTextureID(RenderTarget& target);

    void PlatformRescaleGameWindow(ImVec2 viewport_size);
    bool PlatformProcessEvent(const void* event);
} // namespace ImGui