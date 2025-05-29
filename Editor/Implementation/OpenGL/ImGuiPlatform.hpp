#pragma once

#include "src/ImGuiPlatform.hpp"

namespace ImGui
{
    class PlatformOpenGL final : public Platform
    {
      public:
        PlatformOpenGL();
        virtual ~PlatformOpenGL() override;

        virtual void NewFrame();
        virtual void EndFrame();

        virtual void RescaleFramebuffer(int width, int height);
        virtual ImTextureID GetFramebuffer();
    };
} // namespace ImGui
