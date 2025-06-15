#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Math.hpp"
#include "Resource.hpp"

struct SDL_GPUBuffer;
union BufferID
{
    SDL_GPUBuffer* sdl3gpu = nullptr;
    uint32 opengl;
};

struct SDL_GPUTexture;
union TextureID
{
    SDL_GPUTexture* sdl3gpu = nullptr;
    uint32 opengl;
};

struct SDL_GPUSampler;
union SamplerID
{
    SDL_GPUSampler* sdl3gpu = nullptr;
};

struct SDL_GPUShader;
union ShaderID
{
    SDL_GPUShader* sdl3gpu = nullptr;
    uint32 opengl;
};

struct SDL_GPUGraphicsPipeline;
union ShaderPipelineID
{
    SDL_GPUGraphicsPipeline* sdl3gpu = nullptr;
    uint32 opengl;
};

struct Vertex
{
    Vec3 position{};
    Vec3 color{1.0f, 0.0f, 1.0f};
    Vec2 tex_coord{};
};

struct Texture final : FileResource
{
    enum Type
    {
        DIFFUSE,
        SPECULAR
    };

    Texture() = default;
    explicit Texture(const std::string& path, Type type);
    ~Texture() override;

    Type type{DIFFUSE};

    uint32 width{0};
    uint32 height{0};
    std::vector<uint32> colors{};

    TextureID texture;
    SamplerID sampler;
};

struct Mesh final : Resource
{
    static uint64 GetID(const std::string& path, const uint32 index)
    {
        constexpr std::hash<std::string> hasher{};
        return hasher(path + '-' + std::to_string(index));
    };

    Mesh() = default;
    Mesh(const std::string& path, uint32 index);
    ~Mesh() override;

    std::vector<Vertex> vertices;
    std::vector<uint32> indices;

    uint32 bind{};

    BufferID vertices_buffer;
    BufferID indices_buffer;

    std::vector<Handle<Texture>> textures;

    uint32 index; // Mesh index in the model it was loaded from.
};

struct Shader final : FileResource
{
    enum Type
    {
        VERTEX,
        FRAGMENT
    };

    Shader() = default;
    Shader(const std::string& path, Type type);
    ~Shader();

    Type type{VERTEX};

    uint32 sampler_count{0};
    uint32 storage_count{0};
    uint32 uniform_count{0};
    std::vector<char> code;

    ShaderID shader;
};

struct ShaderPipeline final : Resource
{
    static uint64 GetID(const std::string& vertex_shader_path, const std::string& fragment_shader_path)
    {
        constexpr std::hash<std::string> hasher{};
        return hasher(vertex_shader_path + '+' + fragment_shader_path);
    }
    static uint64 GetID(const Handle<Shader>& vertex_shader, const Handle<Shader>& fragment_shader)
    {
        constexpr std::hash<std::string> hasher{};
        return hasher(vertex_shader->GetPath() + '+' + fragment_shader->GetPath());
    }

    ShaderPipeline() = default;
    ShaderPipeline(const std::string& vertex_shader_path, const std::string& fragment_shader_path);
    ShaderPipeline(const Handle<Shader>& vertex_shader, const Handle<Shader>& fragment_shader);
    ~ShaderPipeline() override;

    const std::string& GetVertexPath() const { return vertex_path; }
    const std::string& GetFragmentPath() const { return fragment_path; }

    ShaderPipelineID shader_pipeline;

  private:
    std::string vertex_path;
    std::string fragment_path;
};

class Renderer
{
  public:
    Renderer(Renderer& other) = delete;
    void operator=(const Renderer&) = delete;

    static inline Vec3 position{0.0f, 0.0f, 0.0f};
    static inline Vec3 forward{0.0f, 0.0f, 0.0f};
    static inline Vec3 up{0.0f, 0.0f, 0.0f};
    static inline float fov{90.0f};

    static void SetupBackend(const std::string& backend_name);
    static Renderer& Instance() { return *renderer; }
    static const std::string& GetBackendName() { return backend_name; }

    virtual constexpr std::size_t WindowFlags() = 0;

    virtual void Init() = 0;
    virtual void Exit() = 0;

    virtual void Update() = 0;
    virtual void SwapBuffer() = 0;
    virtual void OnResize(uint32 width, uint32 height) = 0;

    virtual void* GetContext() = 0;

  protected:
    friend struct Texture;
    friend struct Mesh;
    friend struct Shader;
    friend struct ShaderPipeline;

    static inline std::string backend_name;

    virtual void CreateTexture(Texture& texture) = 0;
    virtual void ReloadTexture(Texture& texture) = 0;
    virtual void DestroyTexture(Texture& texture) = 0;

    virtual void CreateMesh(Mesh& mesh) = 0;
    virtual void ReloadMesh(Mesh& mesh) = 0;
    virtual void DestroyMesh(Mesh& mesh) = 0;

    virtual void CreateShader(Shader& shader) = 0;
    virtual void DestroyShader(Shader& shader) = 0;

    virtual void CreateShaderPipeline(
        ShaderPipeline& pipeline, const Handle<Shader>& vertex_shader, const Handle<Shader>& fragment_shader
    ) = 0;
    virtual void DestroyShaderPipeline(ShaderPipeline& shader) = 0;

    Renderer() = default;
    virtual ~Renderer() = default;

  private:
    static inline Renderer* renderer;
};