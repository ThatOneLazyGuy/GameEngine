#pragma once

#include "Core/Renderer.hpp"

class SDL3GPURenderer final : public Renderer
{
public:
    SDL3GPURenderer() = default;
    virtual ~SDL3GPURenderer() override = default;

    virtual constexpr std::size_t WindowFlags() { return 0; }

    virtual void Init(void* window_handle);
    virtual void Exit();

    virtual void Update();
    virtual void SwapBuffer();

    virtual void* GetContext();
    virtual void* GetTexture();

    virtual Mesh CreateMesh(const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices);
    virtual void ReloadMesh(Mesh& mesh);
    virtual void DeleteMesh(Mesh& mesh);

    virtual Shader CreateShader(const std::string& vertex_path, const std::string& fragment_path);
    virtual void DeleteShader(Shader shader);
};