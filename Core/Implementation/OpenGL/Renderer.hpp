#pragma once

#include "Core/Renderer.hpp"
#include "SDL3/SDL_video.h"

class OpenGLRenderer final : public Renderer
{
public:
    OpenGLRenderer() = default;
    virtual ~OpenGLRenderer() override = default;

    virtual constexpr std::size_t WindowFlags() { return SDL_WINDOW_OPENGL; }

    virtual void Init(void* window_handle);
    virtual void Exit();

    virtual void Update();
    virtual void SwapBuffer();

    virtual void* GetContext();
    virtual void* GetTexture();

    virtual Mesh CreateMesh(const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices);
    virtual void ReloadMesh(Mesh& mesh);
    virtual void DeleteMesh(Mesh& mesh);
};