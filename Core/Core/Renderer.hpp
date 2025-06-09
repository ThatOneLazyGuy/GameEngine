#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Math.hpp"
#include "Resource.hpp"

struct SDL_GPUBuffer;
struct SDL_GPUGraphicsPipeline;
struct SDL_GPUSampler;
struct SDL_GPUTexture;

struct Vertex
{
    Vec3 position{};
    Vec3 color{};
    Vec2 tex_coord{};
};

struct Texture final : FileResource
{
    enum class Type
    {
        DIFFUSE,
        SPECULAR
    };

    Texture() = default;
    explicit Texture(const std::string& path, Type type);
    void Delete();

    Type type{Type::DIFFUSE};

    uint32 width{0};
    uint32 height{0};
    std::vector<uint32> colors{};

    union
    {
        SDL_GPUTexture* texture = nullptr;
        uint32 id;
    };

    SDL_GPUSampler* sampler = nullptr;
};

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<uint32> indices;

    uint32 bind{};

    union
    {
        SDL_GPUBuffer* vertices_buffer = nullptr;
        uint32 VBO;
    };
    union
    {
        SDL_GPUBuffer* indices_buffer = nullptr;
        uint32 EBO;
    };

    std::vector<Handle<Texture>> textures;
};

union Shader
{
    SDL_GPUGraphicsPipeline* pipeline = nullptr;
    uint32 program;
};

class Renderer
{
  public:
    Renderer(Renderer& other) = delete;
    void operator=(const Renderer&) = delete;

    static inline float position[3]{0.0f, 0.0f, 0.0f};
    static inline float fov{90.0f};

    static void SetupBackend(const std::string& backend_name);
    static Renderer& Instance() { return *renderer; }

    virtual constexpr std::size_t WindowFlags() = 0;

    virtual void Init(void* window_handle) = 0;
    virtual void Exit() = 0;

    virtual void Update() = 0;
    virtual void SwapBuffer() = 0;

    virtual void* GetContext() = 0;

    virtual Mesh CreateMesh(
        const std::vector<Vertex>& vertices, const std::vector<uint32>& indices,
        const std::vector<Handle<Texture>>& textures
    ) = 0;
    virtual void ReloadMesh(Mesh& mesh) = 0;
    virtual void DeleteMesh(Mesh& mesh) = 0;

    virtual Texture CreateTexture(const std::string& texture_path, Texture::Type type) = 0;
    virtual Texture CreateTexture(
        uint32 width, uint32 height, const std::vector<uint32>& colors, Texture::Type type
    ) = 0;
    virtual void ReloadTexture(Texture& texture) = 0;
    virtual void DeleteTexture(Texture& texture) = 0;

    virtual Shader CreateShader(const std::string& vertex_path, const std::string& fragment_path) = 0;
    virtual void DeleteShader(Shader shader) = 0;

  protected:
    Renderer() = default;
    virtual ~Renderer() = default;

    static inline Renderer* renderer;
};