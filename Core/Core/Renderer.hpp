#pragma once

#include <vector>
#include <string>

#include <vec2.hpp>
#include <vec3.hpp>

struct SDL_GPUBuffer;

struct Vertex
{
    glm::vec3 position{};
    glm::vec3 color{};
    glm::vec2 tex_coord{};
};

struct Mesh
{
    using RenderVertices = void*;
    using RenderIndices = void*;

    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;

    std::uint32_t bind{};
    RenderVertices render_vertices{};
    RenderIndices render_indices{};
};

// struct Texture
// {
//     Texture() = default;
//     Texture(const std::vector<std::uint32_t>& color_data);
// };

class Renderer
{
  public:
    Renderer(Renderer& other) = delete;
    void operator=(const Renderer&) = delete;

    static inline float size_test[2]{0.0f, 0.0f};

    static void SetupBackend(const std::string& backend_name);
    static Renderer& Instance() { return *renderer; }

    virtual constexpr std::size_t WindowFlags() = 0;

    virtual void Init(void* window_handle) = 0;
    virtual void Exit() = 0;

    virtual void Update() = 0;
    virtual void SwapBuffer() = 0;

    virtual void* GetContext() = 0;
    virtual void* GetTexture() = 0;

    virtual Mesh CreateMesh(const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices) = 0;
    virtual void ReloadMesh(Mesh& mesh) = 0;
    virtual void DeleteMesh(Mesh& mesh) = 0;

  protected:
    Renderer() = default;
    virtual ~Renderer() = default;

    static inline Renderer* renderer;
};