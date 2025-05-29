#pragma once

#include "src/ImGuiPlatform.hpp"

namespace ImGui
{
    class PlatformSDL3GPU final : public Platform
    {
    public:
        PlatformSDL3GPU();
        virtual ~PlatformSDL3GPU() override;

        virtual void NewFrame();
        virtual void EndFrame();

        virtual void RescaleFramebuffer(int width, int height);
        virtual ImTextureID GetFramebuffer();
    };
} // namespace ImGui