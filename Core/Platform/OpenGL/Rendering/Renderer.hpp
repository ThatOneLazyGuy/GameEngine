#pragma once

#include "Core/Rendering/Renderer.hpp"
#include "SDL3/SDL_video.h"

class OpenGLRenderer final : public Renderer
{
  public:
    OpenGLRenderer() = default;
    ~OpenGLRenderer() override = default;

    constexpr size WindowFlags() override { return SDL_WINDOW_OPENGL; }

    void InitBackend() override;
    void ExitBackend() override;

    void Update() override;
    void SwapBuffer() override;

    void* GetContext() override;

    void RenderMesh(const Mesh& mesh) override;
    void SetTextureSampler(uint32 slot, const Texture& texture) override;
    void SetUniform(uint32 slot, const void* data, size size) override;

  private:
    void BeginRenderPass(const RenderPassInterface& render_pass) override;
    void EndRenderPass() override;

    void CreateTexture(Texture& texture, const uint8* data, const SamplerSettings& sampler_settings) override;
    void ResizeTexture(Texture& texture, sint32 new_width, sint32 new_height) override;
    void DestroyTexture(Texture& texture) override;

    void CreateRenderTarget(RenderTarget& target) override;
    void UpdateRenderBuffer(const RenderTarget& target, size index) override;
    void UpdateDepthBuffer(const RenderTarget& target) override;
    void DestroyRenderTarget(RenderTarget& target) override;

    void CreateMesh(Mesh& mesh) override;
    void ReloadMesh(Mesh& mesh) override;
    void DestroyMesh(Mesh& mesh) override;

    void CreateShader(Shader& shader) override;
    void DestroyShader(Shader& shader) override;

    void CreateShaderPipeline(GraphicsShaderPipeline& pipeline, const Handle<Shader>& vertex_shader, const Handle<Shader>& fragment_shader)
        override;
    void DestroyShaderPipeline(GraphicsShaderPipeline& pipeline) override;
};