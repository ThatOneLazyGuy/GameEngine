#pragma once

#include "Core/Renderer.hpp"
#include "SDL3/SDL_video.h"

class OpenGLRenderer final : public Renderer
{
  public:
    OpenGLRenderer() = default;
    ~OpenGLRenderer() override = default;

    constexpr size WindowFlags() override { return SDL_WINDOW_OPENGL; }

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
};