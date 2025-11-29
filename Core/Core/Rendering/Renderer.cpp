#include "Core/Rendering/Renderer.hpp"

#include "Platform/OpenGL/Rendering/Renderer.hpp"
#include "Platform/PC/SDL3GPU/Rendering/Renderer.hpp"

#include "Core/Model.hpp"
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

        const size data_size = static_cast<size>(out_width * out_height) * 4;

        std::vector<uint8_t> return_data{data, data + data_size};
        stbi_image_free(data);

        return return_data;
    }

    std::vector<Handle<Texture>> LoadMaterialTextures(const aiMaterial& material, const aiTextureType type, const std::string& mesh_path)
    {
        std::vector<Handle<Texture>> textures;
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

    if (depth_buffer.GetTexture() != nullptr) { depth_buffer.GetTexture()->Resize(width, height); }
}

void RenderTarget::AddRenderBuffer(const Handle<Texture>& render_texture, const float4& clear_color)
{
    RenderBuffer& render_buffer = render_buffers.emplace_back(render_texture);
    render_buffer.clear_color = clear_color;

    Renderer::Instance().UpdateRenderBuffer(*this, render_buffers.size() - 1);
}

void RenderTarget::SetDepthBuffer(const Handle<Texture>& depth_texture)
{
    depth_buffer = RenderBuffer{depth_texture};

    Renderer::Instance().UpdateDepthBuffer(*this);
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

Mesh::Mesh(const std::string& path, const uint32 index) : index{index}
{
    const auto model_handle = FileResource::Load<ModelParser>(path);
    const aiScene* scene = model_handle->importer.GetScene();

    if (scene == nullptr)
    {
        Log::Error("Failed to load asset: {}", path);
        return;
    }

    const aiMesh& model_mesh = *scene->mMeshes[index];
    const aiVector3D* mesh_vertices = model_mesh.mVertices;
    const aiColor4D* mesh_colors = model_mesh.mColors[0];
    const aiVector3D* mesh_tex_coords = model_mesh.mTextureCoords[0];

    const size vertex_count = model_mesh.mNumVertices;
    vertices.resize(vertex_count);

    for (size i = 0; i < vertex_count; i++)
    {
        auto& [position, color, tex_coord] = vertices[i];

        position = float3{mesh_vertices[i].x, mesh_vertices[i].y, mesh_vertices[i].z};
        if (mesh_colors != nullptr) color = float3{mesh_colors[i].r, mesh_colors[i].g, mesh_colors[i].b};
        if (mesh_tex_coords != nullptr) tex_coord = float2{mesh_tex_coords[i].x, mesh_tex_coords[i].y};
    }

    const aiFace* mesh_faces = model_mesh.mFaces;

    const size face_count = model_mesh.mNumFaces;
    indices.resize(face_count * 3);
    for (size i = 0; i < face_count; i++)
    {
        std::memcpy(&indices[i * 3], mesh_faces[i].mIndices, sizeof(uint32) * 3);
    }

    const size last_separator = path.find_last_of('/');
    const std::string mesh_path = path.substr(0, (last_separator == std::string::npos) ? last_separator : last_separator + 1);

    const aiMaterial& material = *scene->mMaterials[model_mesh.mMaterialIndex];
    auto diffuse_maps = LoadMaterialTextures(material, aiTextureType_DIFFUSE, mesh_path);
    textures.insert(textures.end(), diffuse_maps.begin(), diffuse_maps.end());

    auto specular_maps = LoadMaterialTextures(material, aiTextureType_SPECULAR, mesh_path);
    textures.insert(textures.end(), specular_maps.begin(), specular_maps.end());

    Renderer::Instance().CreateMesh(*this);
}

Mesh::~Mesh() { Renderer::Instance().DestroyMesh(*this); }

uint64 Shader::GetID(const std::string& path, const ShaderSettings& shader_info)
{
    constexpr std::hash<std::string> hasher{};
    return hasher(path + (shader_info.type == VERTEX ? ".vert" : ".frag"));
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

GraphicsShaderPipeline::GraphicsShaderPipeline(
    const std::string& pipeline_path, const ShaderSettings& vertex_settings, const ShaderSettings& fragment_settings
)
{
    const Handle<Shader>& vertex_shader = FileResource::Load<Shader>(pipeline_path, vertex_settings);
    vertex_path = pipeline_path;
    const Handle<Shader>& fragment_shader = FileResource::Load<Shader>(pipeline_path, fragment_settings);
    fragment_path = pipeline_path;

    Renderer::Instance().CreateShaderPipeline(*this, vertex_shader, fragment_shader);

    TryDestroyResource(vertex_shader->Resource::GetID());
    TryDestroyResource(fragment_shader->Resource::GetID());
}

GraphicsShaderPipeline::GraphicsShaderPipeline(const Handle<Shader>& vertex_shader, const Handle<Shader>& fragment_shader) :
    vertex_path{vertex_shader->GetPath()}, fragment_path{fragment_shader->GetPath()}
{
    Renderer::Instance().CreateShaderPipeline(*this, vertex_shader, fragment_shader);
}

GraphicsShaderPipeline::~GraphicsShaderPipeline() { Renderer::Instance().DestroyShaderPipeline(*this); }

void Renderer::SetupBackend(const char* backend_argument)
{
    if (backend_argument == nullptr) backend_name = "SDL3GPU";
    else backend_name = backend_argument;

    main_target = std::make_shared<RenderTarget>();

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
    main_target.reset();
    render_passes.clear();

    renderer->ExitBackend();
}

void Renderer::Render()
{
    Instance().Update();

    for (Handle<RenderPassInterface>& render_pass : render_passes)
    {
        Instance().BeginRenderPass(*render_pass);
        render_pass->Render();
        Instance().EndRenderPass();
    }
}