#include "Core/Renderer.hpp"

#include "Implementation/OpenGL/Renderer.hpp"
#include "Implementation/SDL3/Renderer.hpp"

#include "Core/Model.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <SDL3/SDL_log.h>
#include <assimp/scene.h>
#include <filesystem>
#include <fstream>
#include <stb_image.h>

namespace
{
    std::vector<Handle<Texture>> loadMaterialTextures(
        const aiMaterial& material, const aiTextureType type, const std::string& mesh_path
    )
    {
        std::vector<Handle<Texture>> textures;
        for (uint32 i = 0; i < material.GetTextureCount(type); i++)
        {
            aiString string;
            material.GetTexture(type, i, &string);

            const Texture::Type real_type =
                (type == aiTextureType_DIFFUSE ? Texture::Type::DIFFUSE : Texture::Type::SPECULAR);

            const std::string full_path = mesh_path + string.C_Str();
            auto texture_handle = FileResource::Load<Texture>(full_path, real_type);
            textures.push_back(texture_handle);
        }
        return textures;
    }
} // namespace

Texture::Texture(const std::string& path, const Type type) : type{type}
{
    sint32 image_width, image_height, component_count;
    void* data = stbi_load(path.c_str(), &image_width, &image_height, &component_count, 4);

    const uint32 pixel_count = image_width * image_height;
    if (data != nullptr)
    {
        const auto* pixel_data = static_cast<const uint32*>(data);
        colors.insert(colors.end(), pixel_data, pixel_data + pixel_count);
        stbi_image_free(data);

        width = static_cast<uint32>(image_width);
        height = static_cast<uint32>(image_height);

        Renderer::Instance().CreateTexture(*this);
        return;
    }

    SDL_Log("Failed to load image: %s", stbi_failure_reason());
}

Texture::~Texture() { Renderer::Instance().DestroyTexture(*this); }

Mesh::Mesh(const std::string& path, const uint32 index) : index{index}
{
    const auto model_handle = FileResource::Load<ModelParser>(path);
    const aiScene* scene = model_handle->importer.GetScene();

    if (scene == nullptr)
    {
        SDL_Log("Failed to load asset: ", path.c_str());
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

        position = Vec3{mesh_vertices[i].x, mesh_vertices[i].y, mesh_vertices[i].z};
        if (mesh_colors != nullptr) color = Vec3{mesh_colors[i].r, mesh_colors[i].g, mesh_colors[i].b};
        if (mesh_tex_coords != nullptr) tex_coord = Vec2{mesh_tex_coords[i].x, mesh_tex_coords[i].y};
    }

    const aiFace* mesh_faces = model_mesh.mFaces;

    const size face_count = static_cast<int>(model_mesh.mNumFaces);
    indices.resize(face_count * 3);
    for (size i = 0; i < face_count; i++)
    {
        std::memcpy(&indices[i * 3], mesh_faces[i].mIndices, sizeof(uint32) * 3);
    }

    const size last_separator = path.find_last_of('/');
    const std::string mesh_path =
        path.substr(0, (last_separator == std::string::npos) ? last_separator : last_separator + 1);

    const aiMaterial& material = *scene->mMaterials[model_mesh.mMaterialIndex];
    auto diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, mesh_path);
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

    auto specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, mesh_path);
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

    Renderer::Instance().CreateMesh(*this);
}

Mesh::~Mesh() { Renderer::Instance().DestroyMesh(*this); }

Shader::Shader(const std::string& path, const Type type) : type{type}
{
    if (type == VERTEX) uniform_count = 3;
    else sampler_count = 1;

    const std::ifstream::openmode open_mode =
        (Renderer::GetBackendName() == "OpenGL" ? std::ios::ate : std::ios::ate | std::ios::binary);

    std::ifstream shader_file{path.c_str(), open_mode};
    if (!shader_file.is_open())
    {
        SDL_Log("Failed to open shader file: %s", path.c_str());
        return;
    }

    const std::streamsize size = shader_file.tellg();
    code.resize(size);

    shader_file.seekg(0, std::ios::beg);
    shader_file.read(code.data(), size);
    shader_file.close();

    Renderer::Instance().CreateShader(*this);
}

Shader::~Shader() { Renderer::Instance().DestroyShader(*this); }

ShaderPipeline::ShaderPipeline(const std::string& vertex_shader_path, const std::string& fragment_shader_path) :
    vertex_path{vertex_shader_path}, fragment_path{fragment_shader_path}
{
    const auto vertex_shader = FileResource::Load<Shader>(vertex_shader_path, Shader::VERTEX);
    const auto fragment_shader = FileResource::Load<Shader>(fragment_shader_path, Shader::FRAGMENT);
    Renderer::Instance().CreateShaderPipeline(*this, vertex_shader, fragment_shader);
}

ShaderPipeline::ShaderPipeline(const Handle<Shader>& vertex_shader, const Handle<Shader>& fragment_shader) :
    vertex_path{vertex_shader->GetPath()}, fragment_path{fragment_shader->GetPath()}
{
    Renderer::Instance().CreateShaderPipeline(*this, vertex_shader, fragment_shader);
}

ShaderPipeline::~ShaderPipeline() { Renderer::Instance().DestroyShaderPipeline(*this); }

void Renderer::SetupBackend(const std::string& backend_name)
{
    Renderer::backend_name = backend_name;

    if (backend_name == "OpenGL")
    {
        renderer = new OpenGLRenderer;
        return;
    }

    renderer = new SDL3GPURenderer;
}
