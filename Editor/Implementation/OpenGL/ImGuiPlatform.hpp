#pragma once

#include "src/ImGuiPlatform.hpp"

namespace ImGui
{
    class PlatformOpenGL final : public Platform
    {
      public:
        PlatformOpenGL();
        ~PlatformOpenGL() override;

        void NewFrame() override;
        void EndFrame() override;

        ImTextureID GetTextureID(RenderTarget& target) override;
    };
} // namespace ImGui
