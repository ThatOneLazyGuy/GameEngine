#pragma once

#include "fwd.hpp"

#include <string>
#include <vector>

#include <vec2.hpp>
#include <vec3.hpp>

struct SDL_GPUBuffer;
struct SDL_GPUGraphicsPipeline;

struct Vertex
{
    glm::vec3 position{};
    glm::vec3 color{};
    glm::vec2 tex_coord{};
};

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;

    std::uint32_t bind{};

    union
    {
        SDL_GPUBuffer* vertices_buffer = nullptr;
        std::uint32_t VBO;
    };
    union
    {
        SDL_GPUBuffer* indices_buffer = nullptr;
        std::uint32_t EBO;
    };
};

// struct Texture
// {
//     Texture(const std::vector<std::uint32_t>& color_data);
//
//
//
//     void* handle{};
// };

union Shader
{
    SDL_GPUGraphicsPipeline* pipeline;
    std::uint32_t id;
};

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

    virtual Shader CreateShader(const std::string& vertex_path, const std::string& fragment_path) = 0;
    virtual void DeleteShader(Shader shader) = 0;

  protected:
    Renderer() = default;
    virtual ~Renderer() = default;

    static inline Renderer* renderer;
};