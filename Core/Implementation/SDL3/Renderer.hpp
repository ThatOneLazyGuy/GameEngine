#pragma once

#include "Core/Renderer.hpp"

struct SDL_GPUCommandBuffer;

class SDL3GPURenderer final : public Renderer
{
  public:
    SDL3GPURenderer() = default;
    ~SDL3GPURenderer() override = default;

    constexpr size WindowFlags() override { return 0; }

    void Init() override;
    void Exit() override;

    void Update() override;
    void SwapBuffer() override;

    void* GetContext() override;

    static SDL_GPUCommandBuffer* GetCommandBuffer();

  private:
    void CreateRenderTarget(RenderTarget& target) override;
    void RecreateRenderTarget(RenderTarget& target) override;
    void DestroyRenderTarget(RenderTarget& target) override;

    void CreateTexture(Texture& texture) override;
    void ReloadTexture(Texture& texture) override;
    void DestroyTexture(Texture& texture) override;

    void CreateMesh(Mesh& mesh) override;
    void ReloadMesh(Mesh& mesh) override;
    void DestroyMesh(Mesh& mesh) override;

    void CreateShader(Shader& shader) override;
    void DestroyShader(Shader& shader) override;

    void CreateShaderPipeline(
        ShaderPipeline& pipeline, const Handle<Shader>& vertex_shader, const Handle<Shader>& fragment_shader
    ) override;
    void DestroyShaderPipeline(ShaderPipeline& shader) override;
};