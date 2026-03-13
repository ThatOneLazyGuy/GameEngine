#pragma once

#include "Core/ECS.hpp"
#include "Core/Math.hpp"
#include "Core/ResourceManager.hpp"
#include "Core/Window.hpp"

#include <memory>
#include <string>
#include <vector>

namespace Physics
{
    class DebugRenderer;
}

union BufferID
{
    void* pointer = nullptr;
    uint32 id;
};

union TextureID
{
    void* pointer = nullptr;
    uint32 id;
};

union SamplerID
{
    void* pointer = nullptr;
    uint32 id;
};

union ShaderID
{
    void* pointer = nullptr;
    uint32 id;
};

union GraphicsShaderPipelineID
{
    void* pointer = nullptr;
    uint32 id;
};

struct Vertex
{
    float3 position{};
    float3 color{};
    float2 tex_coord{};
};

inline const std::unordered_map<std::string, usize> attribute_sizes{
    {"POSITION",  offsetof(Vertex, position) },
    {"COLOR",     offsetof(Vertex, color)    },
    {"TEX_COORD", offsetof(Vertex, tex_coord)}
};

struct TextureSettings;
struct SamplerSettings;

class Texture : public Resource<"Texture">
{
  public:
    enum ColorFormat : uint8
    {
        COLOR_RGBA_32,
        DEPTH_24
    };

    enum Flags : uint8
    {
        SAMPLER = (1 << 0),
        COLOR_TARGET = (1 << 1),
        DEPTH_TARGET = (1 << 2),
        DIFFUSE = (1 << 3),
        SPECULAR = (1 << 4)
    };

    Texture() = default;
    Texture(const Files::BinaryReadStream& stream);
    Texture(const TextureSettings& texture_settings, const SamplerSettings& sampler_settings);

    Texture(Texture&) = delete;
    Texture(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture(const Texture&&) = delete;
    ~Texture() override;

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
    RenderBuffer(const ResourceRef<Texture>& texture) : texture{texture} {}

    [[nodiscard]] ResourceRef<Texture> GetTexture() const { return texture; }

    float4 clear_color{};

  private:
    ResourceRef<Texture> texture;
};

class RenderTarget final : public Resource<"Render Target">
{
  public:
    RenderTarget() = default;
    explicit RenderTarget(const std::string& name);

    void Resize(sint32 new_width, sint32 new_height);

    [[nodiscard]] sint32 GetWidth() const { return width; }
    [[nodiscard]] sint32 GetHeight() const { return height; }

    void AddRenderBuffer(const ResourceRef<Texture>& render_texture, const float4& clear_color = {});
    void SetDepthBuffer(const ResourceRef<Texture>& depth_texture);

    std::vector<RenderBuffer> render_buffers;
    RenderBuffer depth_buffer;

    uint32 target_id; // Only used for OpenGL.

  private:
    std::string name{};

    sint32 width{1};
    sint32 height{1};
};

class Mesh final : public Resource<"Mesh">
{
  public:
    Mesh() = default;
    Mesh(Files::BinaryReadStream& stream);
    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32>& indices);
    ~Mesh() override;

    [[nodiscard]] uint32 GetVerticesCount() const { return vertices_count; }
    [[nodiscard]] uint32 GetIndicesCount() const { return indices_count; }
    // Mesh index in the model it was loaded from.
    [[nodiscard]] uint32 GetIndex() const { return index; }

    BufferID vertices_buffer;
    BufferID indices_buffer;

    std::vector<ResourceRef<Texture>> textures;

  private:
    uint32 vertices_count;
    uint32 indices_count;
    uint32 index{0}; // Mesh index in the model it was loaded from.
};

struct ShaderSettings;

class Shader final : public Resource<"Shader">
{
  public:
    enum Type : uint8
    {
        VERTEX,
        FRAGMENT,
        COMPUTE,
    };

    Shader() = default;
    Shader(Files::BinaryReadStream& stream);
    Shader(std::string path, const ShaderSettings& shader_info);
    Shader(const void* data, usize count, const ShaderSettings& shader_info);
    ~Shader() override;

    Type type{VERTEX};
    uint32 sampler_count{0};
    uint32 storage_count{0};
    uint32 uniform_count{0};

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

struct VertexAttribute
{
    enum ElementType : uint8
    {
        FLOAT = 0,
        SINT = 1,
        UINT = 2,
    };

    std::string semantic_name;
    ElementType element_type;
    usize element_count;
};

struct GraphicsPipelineSettings
{
    std::vector<VertexAttribute> vertex_attributes;

    // The sizes of all uniforms in the pipeline.
    std::unordered_map<std::string, usize> uniform_sizes;

    ShaderSettings vertex_info;
    ShaderSettings fragment_info;
};

class GraphicsShaderPipeline final : public Resource<"Graphics Shader Pipeline">
{
  public:
    GraphicsShaderPipeline() = default;
    GraphicsShaderPipeline(Files::BinaryReadStream& stream);
    GraphicsShaderPipeline(const std::string& pipeline_path, const GraphicsPipelineSettings& pipeline_settings);
    ~GraphicsShaderPipeline() override;

    [[nodiscard]] bool IsWireframe() const { return wireframe; }

    GraphicsShaderPipelineID shader_pipeline;

    std::vector<VertexAttribute> vertex_attributes;
    std::unordered_map<std::string, usize> uniform_sizes;

  private:
    bool wireframe{false};
};

class Renderer
{
  public:
    struct BackendShaderInfo
    {
        const char* file_extension;
        bool binary;
        const char* profile;
        bool invert_y;
    };

    Renderer(Renderer& other) = delete;
    void operator=(const Renderer&) = delete;

    static void SetupBackend(const char* backend_argument);
    static Renderer& Instance() { return *renderer; }
    static const std::string& GetBackendName() { return backend_name; }

    static inline std::vector<std::shared_ptr<RenderPassInterface>> render_passes;

    virtual constexpr std::size_t WindowFlags() = 0;

    static void Init();
    static void Exit();

    static void Render(const ECS::Entity& camera_entity);
    virtual void SwapBuffer() = 0;

    virtual void* GetContext() = 0;

    virtual void RenderMesh(const Mesh& mesh) = 0;
    virtual void SetTextureSampler(uint32 slot, const Texture& texture) = 0;
    virtual void SetUniform(uint32 slot, const void* data, usize size) = 0;

    template <typename Type>
    static void SetUniform(const uint32 slot, const Type& object)
    {
        Instance().SetUniform(slot, static_cast<const void*>(&object), sizeof(object));
    }

    static const BackendShaderInfo& GetBackendShaderInfo() { return backend_shader_info; }

    static inline ResourceRef<RenderTarget> main_target;

  protected:
    friend class RenderTarget;
    friend class Texture;
    friend class Mesh;
    friend class Shader;
    friend class GraphicsShaderPipeline;
    friend class Physics::DebugRenderer;

    static inline BackendShaderInfo backend_shader_info; // Needs to be setup in InitBackend.
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
    virtual void UpdateRenderBuffer(const RenderTarget& target, usize index) = 0;
    virtual void UpdateDepthBuffer(const RenderTarget& target) = 0;
    virtual void DestroyRenderTarget(RenderTarget& target) = 0;

    virtual void CreateMesh(Mesh& mesh, const std::vector<Vertex>& vertices, const std::vector<uint32>& indices) = 0;
    virtual void DestroyMesh(Mesh& mesh) = 0;

    virtual void CreateShader(Shader& shader, const void* data, usize size) = 0;
    virtual void DestroyShader(Shader& shader) = 0;

    virtual void CreateShaderPipeline(
        GraphicsShaderPipeline& pipeline, const ResourceRef<Shader>& vertex_shader, const ResourceRef<Shader>& fragment_shader
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