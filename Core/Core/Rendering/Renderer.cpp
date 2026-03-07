#include "Core/Rendering/Renderer.hpp"

#include "Platform/OpenGL/Rendering/Renderer.hpp"
#include "Platform/PC/SDL3GPU/Rendering/Renderer.hpp"

#include "RenderPassInterface.hpp"
#include "Tools/Logging.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include <stb/stb_image.h>
#include <filesystem>
#include <assimp/scene.h>

#include "Tools/Files.hpp"

namespace
{
    std::vector<uint8> LoadTextureImage(const std::string& path, sint32& out_width, sint32& out_height)
    {
        const std::vector<uint8>& file_data = Files::ReadBinary(path);
        const int file_size = static_cast<int>(file_data.size());

        sint32 component_count;
        uint8* data = stbi_load_from_memory(file_data.data(), file_size, &out_width, &out_height, &component_count, 4);
        if (data == nullptr)
        {
            Log::Error("Failed to load image: {}", stbi_failure_reason());
            return {};
        }

        const usize data_size = static_cast<usize>(out_width * out_height) * 4;

        std::vector<uint8_t> return_data{data, data + data_size};
        stbi_image_free(data);

        return return_data;
    }

    std::vector<std::shared_ptr<Texture>> LoadMaterialTextures(
        const aiMaterial& material, const aiTextureType type, const std::string& mesh_path
    )
    {
        std::vector<std::shared_ptr<Texture>> textures;
        for (uint32 i = 0; i < material.GetTextureCount(type); i++)
        {
            aiString string;
            material.GetTexture(type, i, &string);

            Texture::Flags flags = (type == aiTextureType_DIFFUSE ? Texture::Flags::DIFFUSE : Texture::Flags::SPECULAR);

            sint32 width, height;
            const std::string full_path = mesh_path + string.C_Str();
            std::vector<uint8> data = LoadTextureImage(full_path, width, height);

            const TextureSettings texture_settings{
                .width = width,
                .height = height,
                .format = Texture::COLOR_RGBA_32,
                .flags = static_cast<Texture::Flags>(Texture::Flags::SAMPLER | flags),
                .color_data = data.data()
            };

            auto texture_handle = std::make_shared<Texture>(texture_settings, SamplerSettings{});
            textures.push_back(texture_handle);
        }
        return textures;
    }
} // namespace

RenderTarget::RenderTarget(const std::string& name) : name{name} { Renderer::Instance().CreateRenderTarget(*this); }

void RenderTarget::Resize(const sint32 new_width, const sint32 new_height)
{
    if (width == new_width && height == new_height) return;

    width = new_width;
    height = new_height;

    for (RenderBuffer& buffer : render_buffers)
    {
        buffer.GetTexture()->Resize(width, height);
    }

    if (depth_buffer.GetTexture().Valid()) { depth_buffer.GetTexture()->Resize(width, height); }
}

void RenderTarget::AddRenderBuffer(const ResourceRef<Texture>& render_texture, const float4& clear_color)
{
    RenderBuffer& render_buffer = render_buffers.emplace_back(render_texture);
    render_buffer.clear_color = clear_color;

    Renderer::Instance().UpdateRenderBuffer(*this, render_buffers.size() - 1);
}

void RenderTarget::SetDepthBuffer(const ResourceRef<Texture>& depth_texture)
{
    depth_buffer = RenderBuffer{depth_texture};

    Renderer::Instance().UpdateDepthBuffer(*this);
}

Texture::Texture(const Files::BinaryReadStream& stream)
{
    const int file_size = static_cast<int>(stream.Size());

    sint32 component_count;
    uint8* color_data = stbi_load_from_memory(stream.Data(), file_size, &width, &height, &component_count, 4);
    if (color_data == nullptr)
    {
        Log::Error("Failed to load image: {}", stbi_failure_reason());
        return;
    }

    Renderer::Instance().CreateTexture(*this, color_data, {});

    stbi_image_free(color_data);
}

Texture::Texture(const TextureSettings& texture_settings, const SamplerSettings& sampler_settings) :
    width{texture_settings.width}, height{texture_settings.height}, format{texture_settings.format}, flags{texture_settings.flags}
{
    Renderer::Instance().CreateTexture(*this, texture_settings.color_data, sampler_settings);
}

Texture::Texture(Texture&& other) noexcept
{
    std::swap(other.texture, texture);
    std::swap(other.sampler, sampler);

    std::swap(other.width, width);
    std::swap(other.height, height);

    std::swap(other.format, format);
    std::swap(other.flags, flags);
}

Texture::~Texture() { Renderer::Instance().DestroyTexture(*this); }

void Texture::Resize(const sint32 new_width, const sint32 new_height)
{
    if (width == new_width && height == new_height) return;

    Renderer::Instance().ResizeTexture(*this, new_width, new_height);

    // Make sure to manually set the new width and height *after*, since the renderer can't access these and needs both the new and old size.
    width = new_width;
    height = new_height;
}


Mesh::Mesh(Files::BinaryReadStream& stream)
{
    std::vector<Vertex> vertices;
    stream >> vertices;
    vertices_count = static_cast<uint32>(vertices.size());

    std::vector<uint32> indices;
    stream >> indices;
    indices_count = static_cast<uint32>(indices.size());

    usize texture_count;
    stream >> texture_count;

    for (usize i = 0; i < texture_count; i++)
    {
        UUID texture_uuid;
        stream >> texture_uuid;

        textures.push_back(ResourceManager::Load<Texture>(texture_uuid));
    }

    Renderer::Instance().CreateMesh(*this, vertices, indices);
}

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32>& indices)
{
    vertices_count = static_cast<uint32>(vertices.size());
    indices_count = static_cast<uint32>(indices.size());

    Renderer::Instance().CreateMesh(*this, vertices, indices);
}

Mesh::~Mesh() { Renderer::Instance().DestroyMesh(*this); }

Shader::Shader(Files::BinaryReadStream& stream)
{
    stream >> type;
    stream >> sampler_count;
    stream >> storage_count;
    stream >> uniform_count;

    std::vector<uint8> data;
    stream >> data;

    Renderer::Instance().CreateShader(*this, data.data(), data.size());
}

Shader::Shader(std::string path, const ShaderSettings& shader_info) :
    type{shader_info.type}, sampler_count{shader_info.sampler_count}, storage_count{shader_info.storage_count},
    uniform_count{shader_info.uniform_count}
{
    const Renderer::BackendShaderInfo& backend_shader_info = Renderer::GetBackendShaderInfo();

    path = path.substr(0, path.find_last_of('.'));
    path += (shader_info.type == VERTEX ? ".vert" : ".frag");
    path += backend_shader_info.file_extension;

    if (backend_shader_info.binary)
    {
        const std::vector<uint8>& binary_shader = Files::ReadBinary(path);
        Renderer::Instance().CreateShader(*this, binary_shader.data(), binary_shader.size());
    }
    else
    {
        const std::string& text_shader = Files::ReadText(path);
        Renderer::Instance().CreateShader(*this, text_shader.data(), text_shader.size());
    }
}

Shader::Shader(const void* data, const usize count, const ShaderSettings& shader_info) :
    type{shader_info.type}, sampler_count{shader_info.sampler_count}, storage_count{shader_info.storage_count},
    uniform_count{shader_info.uniform_count}
{
    Renderer::Instance().CreateShader(*this, data, count);
}

Shader::~Shader() { Renderer::Instance().DestroyShader(*this); }

GraphicsShaderPipeline::GraphicsShaderPipeline(Files::BinaryReadStream& stream)
{
    usize vertex_attribute_count;
    stream >> vertex_attribute_count;
    vertex_attributes.resize(vertex_attribute_count);

    for (usize i = 0; i < vertex_attribute_count; i++)
    {
        VertexAttribute& attribute = vertex_attributes[i];

        stream >> attribute.semantic_name;

        stream >> attribute.element_type;
        stream >> attribute.element_count;
    }

    usize uniform_sizes_count;
    stream >> uniform_sizes_count;

    for (usize i = 0; i < uniform_sizes_count; i++)
    {
        std::string name;
        stream >> name;

        usize size;
        stream >> size;

        uniform_sizes.emplace(std::move(name), size);
    }

    UUID vertex_uuid;
    stream >> vertex_uuid;
    const ResourceRef<Shader> vertex_shader = ResourceManager::Load<Shader>(vertex_uuid);

    UUID fragment_uuid;
    stream >> fragment_uuid;
    const ResourceRef<Shader> fragment_shader = ResourceManager::Load<Shader>(fragment_uuid);

    Renderer::Instance().CreateShaderPipeline(*this, vertex_shader, fragment_shader);
}

GraphicsShaderPipeline::GraphicsShaderPipeline(const std::string& pipeline_path, const GraphicsPipelineSettings& pipeline_settings) :
    vertex_attributes{pipeline_settings.vertex_attributes}, uniform_sizes{pipeline_settings.uniform_sizes}
{
    const ResourceRef<Shader> vertex_shader = ResourceManager::Create<Shader>(pipeline_path, pipeline_settings.vertex_info);
    const ResourceRef<Shader> fragment_shader = ResourceManager::Create<Shader>(pipeline_path, pipeline_settings.fragment_info);

    Renderer::Instance().CreateShaderPipeline(*this, vertex_shader, fragment_shader);
}

GraphicsShaderPipeline::~GraphicsShaderPipeline() { Renderer::Instance().DestroyShaderPipeline(*this); }

void Renderer::SetupBackend(const char* backend_argument)
{
    if (backend_argument == nullptr) backend_name = "SDL3GPU";
    else backend_name = backend_argument;

    if (backend_name == "OpenGL")
    {
        renderer = new OpenGLRenderer;
        return;
    }

    if (backend_name == "SDL3GPU")
    {
        renderer = new SDL3GPURenderer;
        return;
    }
}

void Renderer::Init() { renderer->InitBackend(); }

void Renderer::Exit()
{
    main_target.Clear();
    render_passes.clear();

    renderer->ExitBackend();
}

void Renderer::Render(const ECS::Entity& camera_entity)
{
    Instance().Update();

    for (std::shared_ptr<RenderPassInterface>& render_pass : render_passes)
    {
        Instance().BeginRenderPass(*render_pass);
        render_pass->Render(camera_entity);
        Instance().EndRenderPass();
    }
}