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

Texture::Texture(const std::vector<uint8>& data)
{
    const int file_size = static_cast<int>(data.size());

    sint32 component_count;
    uint8* color_data = stbi_load_from_memory(data.data(), file_size, &width, &height, &component_count, 4);
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


Mesh::Mesh(const std::vector<uint8>& data)
{
    usize read_index = 0;
    vertices_count = static_cast<uint32>(*reinterpret_cast<const usize*>(data.data() + read_index));
    read_index += sizeof(usize);

    std::vector<Vertex> vertices;
    vertices.resize(vertices_count);
    std::memcpy(vertices.data(), data.data() + read_index, vertices_count * sizeof(Vertex));
    read_index += vertices_count * sizeof(Vertex);

    indices_count = static_cast<uint32>(*reinterpret_cast<const usize*>(data.data() + read_index));
    read_index += sizeof(usize);

    std::vector<uint32> indices;
    indices.resize(indices_count);
    std::memcpy(indices.data(), data.data() + read_index, indices_count * sizeof(uint32));
    read_index += indices_count * sizeof(uint32);

    const usize texture_count = *reinterpret_cast<const usize*>(data.data() + read_index);
    read_index += sizeof(usize);

    for (usize i = 0; i < texture_count; i++)
    {
        const UUID texture_uuid{data.data() + read_index};
        read_index += sizeof(UUID);

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

std::string Shader::GetID(const std::string& path, const ShaderSettings& shader_info)
{
    return path + (shader_info.type == VERTEX ? ".vert" : ".frag");
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

Shader::~Shader() { Renderer::Instance().DestroyShader(*this); }

GraphicsShaderPipeline::GraphicsShaderPipeline(const std::string& text) {}

GraphicsShaderPipeline::GraphicsShaderPipeline(const std::string& pipeline_path, const GraphicsPipelineSettings& pipeline_settings) :
    vertex_attributes{pipeline_settings.vertex_attributes}, uniform_sizes{pipeline_settings.uniform_sizes}
{
    const ResourceRef<Shader> vertex_shader = ResourceManager::Create<Shader>(pipeline_path, pipeline_settings.vertex_info);
    vertex_path = pipeline_path;
    const ResourceRef<Shader> fragment_shader = ResourceManager::Create<Shader>(pipeline_path, pipeline_settings.fragment_info);
    fragment_path = pipeline_path;

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