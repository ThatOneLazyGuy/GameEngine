#pragma once

#include "ECS.hpp"
#include "Math.hpp"
#include "Resource.hpp"
#include "Window.hpp"

#include <memory>
#include <string>
#include <vector>

namespace Physics
{
    class DebugRenderer;
}

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
    uint32 opengl;
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
    float3 position{};
    float3 color{};
    float2 tex_coord{};
};

class RenderTarget final : public Resource
{
  public:
    static uint64 GetID(const std::string& name)
    {
        constexpr std::hash<std::string> hasher{};
        return hasher(name);
    }

    RenderTarget() = default;
    explicit RenderTarget(const std::string& name);
    ~RenderTarget() override;

    bool Resize(sint32 width, sint32 height);

    sint32 GetWidth() const { return width; }
    sint32 GetHeight() const { return height; }

    TextureID render_texture;
    SamplerID sampler;
    TextureID depth_texture;

    bool clear{true};
    float4 clear_color{};

  private:
    std::string name{};

    sint32 width{1};
    sint32 height{1};
};

class Texture final : public FileResource
{
  public:
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

class Mesh final : public Resource
{
  public:
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

class Shader final : public FileResource
{
  public:
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

class ShaderPipeline final : public Resource
{
  public:
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

    static void SetupBackend(const char* backend_name);
    static Renderer& Instance() { return *renderer; }
    static const std::string& GetBackendName() { return backend_name; }

    virtual constexpr std::size_t WindowFlags() = 0;

    virtual void Init() = 0;
    virtual void Exit() = 0;

    virtual void Update() = 0;
    virtual void SwapBuffer() = 0;

    virtual void* GetContext() = 0;

    static inline std::shared_ptr<RenderTarget> main_target;

  protected:
    friend class RenderTarget;
    friend class Texture;
    friend class Mesh;
    friend class Shader;
    friend class ShaderPipeline;
    friend class Physics::DebugRenderer;

    static inline std::string backend_name;

    virtual void CreateRenderTarget(RenderTarget& target) = 0;
    virtual void RecreateRenderTarget(RenderTarget& target) = 0;
    virtual void DestroyRenderTarget(RenderTarget& target) = 0;

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

class Camera
{
  public:
    Camera() : render_target{Renderer::main_target} {}
    
    Matrix4 GetProjection(const RenderTarget& target) const
    {
        const float aspect = static_cast<float>(target.GetWidth()) / static_cast<float>(target.GetHeight());

        if (Renderer::GetBackendName() == "OpenGL") return Math::PerspectiveNO(fov, aspect, near, far);
        return Math::PerspectiveZO(fov, aspect, near, far);
    }

    Handle<RenderTarget> render_target;

    float fov{Math::ToRadians(45.0f)};

    float near{0.1f};
    float far{1000.0f};
};