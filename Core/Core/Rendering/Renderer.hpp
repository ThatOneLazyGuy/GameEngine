#pragma once

#include "Core/ECS.hpp"
#include "Core/Math.hpp"
#include "Core/Resource.hpp"
#include "Core/Window.hpp"

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
union GraphicsShaderPipelineID
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

struct TextureSettings;
struct SamplerSettings;

class Texture
{
  public:
    enum ColorFormat : uint32
    {
        COLOR_RGBA_32,
        DEPTH_24
    };

    enum Flags : uint32
    {
        SAMPLER =       (1 << 0),
        COLOR_TARGET =  (1 << 1),
        DEPTH_TARGET =  (1 << 2),
        DIFFUSE =       (1 << 3),
        SPECULAR =      (1 << 4)
    };

    Texture() = default;
    Texture(const TextureSettings& texture_settings, const SamplerSettings& sampler_settings);

    Texture(Texture&) = delete;
    Texture(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture(const Texture&&) = delete;
    ~Texture();

    void Resize(sint32 new_width, sint32 new_height);

    [[nodiscard]] sint32 GetWidth() const { return width; }
    [[nodiscard]] sint32 GetHeight() const { return height; }

    [[nodiscard]] ColorFormat GetFormat() const { return format; }
    [[nodiscard]] Flags GetFlags() const { return flags; }

    TextureID texture{};
    SamplerID sampler{};

  private:
    sint32 width{0};
    sint32 height{0};

    ColorFormat format{0};
    Flags flags{SAMPLER};
};

struct TextureSettings
{
    sint32 width{0};
    sint32 height{0};
    Texture::ColorFormat format{0};
    Texture::Flags flags{0};
    const uint8* color_data{nullptr};
};

struct SamplerSettings
{
    uint32 down_filter{0};
    uint32 up_filter{0};
    uint32 mipmap_mode{0};
    uint32 wrap_mode_u{2};
    uint32 wrap_mode_v{2};
};

class RenderBuffer
{
  public:
    RenderBuffer() = default;
    RenderBuffer(const Handle<Texture>& texture) : texture{texture} {}

    [[nodiscard]] Handle<Texture> GetTexture() const { return texture; }

    float4 clear_color{};

  private:
    Handle<Texture> texture;
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

    void Resize(sint32 new_width, sint32 new_height);

    [[nodiscard]] sint32 GetWidth() const { return width; }
    [[nodiscard]] sint32 GetHeight() const { return height; }

    void AddRenderBuffer(const Handle<Texture>& render_texture, const float4& clear_color = {});
    void SetDepthBuffer(const Handle<Texture>& depth_texture);

    std::vector<RenderBuffer> render_buffers;
    RenderBuffer depth_buffer;

    uint32 target_id; // Only used for OpenGL.

  private:
    std::string name{};

    sint32 width{1};
    sint32 height{1};
};

class Mesh final : public Resource
{
  public:
    static uint64 GetID(const std::string& path, const uint32 index)
    {
        constexpr std::hash<std::string> hasher{};
        return hasher(path + '-' + std::to_string(index));
    }

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

struct ShaderSettings;

class Shader final : public FileResource
{
  public:
    enum Type
    {
        VERTEX,
        FRAGMENT,
        COMPUTER,
    };

    Shader() = default;
    Shader(const std::string& path, const ShaderSettings& shader_info);
    ~Shader() override;

    Type type{VERTEX};
    uint32 sampler_count{0};
    uint32 storage_count{0};
    uint32 uniform_count{0};

    std::vector<char> code;

    ShaderID shader;
};

struct ShaderSettings
{
    Shader::Type type{Shader::VERTEX};
    uint32 sampler_count{0};
    uint32 storage_count{0};
    uint32 uniform_count{0};
};

class RenderPassInterface;

class GraphicsShaderPipeline final : public Resource
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

    GraphicsShaderPipeline() = default;
    GraphicsShaderPipeline(const std::string& vertex_shader_path, const std::string& fragment_shader_path);
    GraphicsShaderPipeline(const Handle<Shader>& vertex_shader, const Handle<Shader>& fragment_shader);
    ~GraphicsShaderPipeline() override;

    const std::string& GetVertexPath() const { return vertex_path; }
    const std::string& GetFragmentPath() const { return fragment_path; }

    bool IsWireframe() const { return wireframe; }

    GraphicsShaderPipelineID shader_pipeline;

  private:
    std::string vertex_path;
    std::string fragment_path;

    bool wireframe{false};
};

class Renderer
{
  public:
    Renderer(Renderer& other) = delete;
    void operator=(const Renderer&) = delete;

    static void SetupBackend(const char* backend_argument);
    static Renderer& Instance() { return *renderer; }
    static const std::string& GetBackendName() { return backend_name; }

    static inline std::vector<Handle<RenderPassInterface>> render_passes;

    virtual constexpr std::size_t WindowFlags() = 0;

    static void Init();
    static void Exit();

    static void Render();
    virtual void SwapBuffer() = 0;

    virtual void* GetContext() = 0;

    virtual void RenderMesh(const Mesh& mesh) = 0;
    virtual void SetTextureSampler(uint32 slot, const Texture& texture) = 0;
    virtual void SetUniform(uint32 slot, const void* data, size size) = 0;

    template <typename Type>
    static void SetUniform(const uint32 slot, const Type& object)
    {
        Instance().SetUniform(slot, static_cast<const void*>(&object), sizeof(object));
    }

    static inline Handle<RenderTarget> main_target;

  protected:
    friend class RenderTarget;
    friend class Texture;
    friend class Mesh;
    friend class Shader;
    friend class GraphicsShaderPipeline;
    friend class Physics::DebugRenderer;

    static inline std::string backend_name;

    virtual void InitBackend() = 0;
    virtual void ExitBackend() = 0;
    virtual void Update() = 0;

    virtual void BeginRenderPass(const RenderPassInterface& render_pass) = 0;
    virtual void EndRenderPass() = 0;

    virtual void CreateTexture(Texture& texture, const uint8* data, const SamplerSettings& sampler_settings) = 0;
    virtual void ResizeTexture(Texture& texture, sint32 new_width, sint32 new_height) = 0;
    virtual void DestroyTexture(Texture& texture) = 0;

    virtual void CreateRenderTarget(RenderTarget& target) = 0;
    virtual void UpdateRenderBuffer(const RenderTarget& target, size index) = 0;
    virtual void UpdateDepthBuffer(const RenderTarget& target) = 0;
    virtual void DestroyRenderTarget(RenderTarget& target) = 0;

    virtual void CreateMesh(Mesh& mesh) = 0;
    virtual void ReloadMesh(Mesh& mesh) = 0;
    virtual void DestroyMesh(Mesh& mesh) = 0;

    virtual void CreateShader(Shader& shader) = 0;
    virtual void DestroyShader(Shader& shader) = 0;

    virtual void CreateShaderPipeline(
        GraphicsShaderPipeline& pipeline, const Handle<Shader>& vertex_shader, const Handle<Shader>& fragment_shader
    ) = 0;
    virtual void DestroyShaderPipeline(GraphicsShaderPipeline& pipeline) = 0;

    Renderer() = default;
    virtual ~Renderer() = default;

  private:
    static inline Renderer* renderer;
};

class Camera
{
  public:
    Matrix4 GetProjection(const RenderTarget& target) const
    {
        const float aspect = static_cast<float>(target.GetWidth()) / static_cast<float>(target.GetHeight());

        if (Renderer::GetBackendName() == "OpenGL") return Math::PerspectiveNO(fov, aspect, near, far);
        return Math::PerspectiveZO(fov, aspect, near, far);
    }

    float fov{Math::ToRadians(45.0f)};

    float near{0.1f};
    float far{1000.0f};
};