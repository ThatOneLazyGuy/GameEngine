#pragma once

#include "src/ImGuiPlatform.hpp"

namespace ImGui
{
    class PlatformSDL3GPU final : public Platform
    {
      public:
        PlatformSDL3GPU();
        ~PlatformSDL3GPU() override;

        void NewFrame() override;
        void EndFrame() override;

        ImTextureID GetTextureID(RenderTarget& target) override;
    };
} // namespace ImGui