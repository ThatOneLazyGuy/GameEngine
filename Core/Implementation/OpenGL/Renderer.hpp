#pragma once

#include "Core/Renderer.hpp"
#include "SDL3/SDL_video.h"

class OpenGLRenderer final : public Renderer
{
  public:
    OpenGLRenderer() = default;
    virtual ~OpenGLRenderer() override = default;

    virtual constexpr size WindowFlags() { return SDL_WINDOW_OPENGL; }

    virtual void Init(void* window_handle);
    virtual void Exit();

    virtual void Update();
    virtual void SwapBuffer();

    virtual void* GetContext();

    virtual Mesh CreateMesh(
        const std::vector<Vertex>& vertices, const std::vector<uint32>& indices,
        const std::vector<Handle<Texture>>& textures
    );
    virtual void ReloadMesh(Mesh& mesh);
    virtual void DeleteMesh(Mesh& mesh);

    virtual Texture CreateTexture(const std::string& texture_path, Texture::Type type);
    virtual Texture CreateTexture(
        uint32 width, uint32 height, const std::vector<uint32>& colors, Texture::Type type
    );
    virtual void ReloadTexture(Texture& texture);
    virtual void DeleteTexture(Texture& texture);

    virtual Shader CreateShader(const std::string& vertex_path, const std::string& fragment_path);
    virtual void DeleteShader(Shader shader);
};