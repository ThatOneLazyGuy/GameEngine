#pragma once

#include "Core/Rendering/Renderer.hpp"

struct SDL_GPUCommandBuffer;

class SDL3GPURenderer final : public Renderer
{
  public:
    SDL3GPURenderer();
    ~SDL3GPURenderer() override = default;

    constexpr size WindowFlags() override { return 0; }

    void InitBackend() override;
    void ExitBackend() override;

    void Update() override;
    void SwapBuffer() override;

    void* GetContext() override;

    void RenderMesh(const Mesh& mesh) override;
    void SetTextureSampler(uint32 slot, const Texture& texture) override;
    void SetUniform(uint32 slot, const void* data, size size) override;

    static SDL_GPUCommandBuffer* GetCommandBuffer();

  private:
    void BeginRenderPass(const RenderPassInterface& render_pass) override;
    void EndRenderPass() override;

    void CreateTexture(Texture& texture, const uint8* data, const SamplerSettings& sampler_settings) override;
    void ResizeTexture(Texture& texture, sint32 new_width, sint32 new_height) override;
    void DestroyTexture(Texture& texture) override;

    // Render target doesn't need to be setup for SDL3GPU.
    void CreateRenderTarget(RenderTarget& target) override {}
    void UpdateRenderBuffer(const RenderTarget& target, size index) override {}
    void UpdateDepthBuffer(const RenderTarget& target) override {}
    void DestroyRenderTarget(RenderTarget& target) override {}

    void CreateMesh(Mesh& mesh) override;
    void ReloadMesh(Mesh& mesh) override;
    void DestroyMesh(Mesh& mesh) override;

    void CreateShader(Shader& shader, const void* data, size size) override;
    void DestroyShader(Shader& shader) override;

    void CreateShaderPipeline(GraphicsShaderPipeline& pipeline, const Handle<Shader>& vertex_shader, const Handle<Shader>& fragment_shader)
        override;
    void DestroyShaderPipeline(GraphicsShaderPipeline& pipeline) override;
};