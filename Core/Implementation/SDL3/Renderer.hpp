#pragma once

#include "Core/Renderer.hpp"
#include "SDL3/SDL_gpu.h"

class SDL3GPURenderer final : public Renderer
{
  public:
    SDL3GPURenderer() = default;
    ~SDL3GPURenderer() override = default;

    constexpr std::size_t WindowFlags() override { return 0; }

    void Init(void* window_handle) override;
    void Exit() override;

    void Update() override;
    void SwapBuffer() override;
    void OnResize(uint32 width, uint32 height) override;

    void* GetContext() override;

    void CreateMesh(
        Mesh& mesh, const std::vector<Vertex>& vertices, const std::vector<uint32>& indices,
        const std::vector<Handle<Texture>>& textures
    ) override;
    void ReloadMesh(Mesh& mesh) override;
    void DeleteMesh(Mesh& mesh) override;

    void CreateTexture(
        Texture& texture, uint32 width, uint32 height, const std::vector<uint32>& colors, Texture::Type type
    ) override;
    void ReloadTexture(Texture& texture) override;
    void DeleteTexture(Texture& texture) override;

    Shader CreateShader(const std::string& vertex_path, const std::string& fragment_path) override;
    void DeleteShader(Shader shader) override;

    static SDL_GPUColorTargetInfo& GetColorTarget();
};