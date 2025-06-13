#include "Core/Renderer.hpp"

#include "Implementation/OpenGL/Renderer.hpp"
#include "Implementation/SDL3/Renderer.hpp"

#include "Core/Model.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <SDL3/SDL_log.h>
#include <assimp/scene.h>
#include <stb_image.h>

namespace
{
    std::vector<Handle<Texture>> loadMaterialTextures(const aiMaterial& material, const aiTextureType type)
    {
        std::vector<Handle<Texture>> textures;
        for (uint32 i = 0; i < material.GetTextureCount(type); i++)
        {
            aiString string;
            material.GetTexture(type, i, &string);

            const Texture::Type real_type =
                (type == aiTextureType_DIFFUSE ? Texture::Type::DIFFUSE : Texture::Type::SPECULAR);

            auto texture_handle = FileResource::Load<Texture>(string.C_Str(), real_type);
            textures.push_back(texture_handle);
        }
        return textures;
    }
} // namespace

Texture::Texture(const std::string& path, const Type type)
{
    sint32 width, height, component_count;
    void* data = stbi_load(("Assets/Backpack/" + path).c_str(), &width, &height, &component_count, 4);

    const uint32 pixel_count = width * height;
    if (data != nullptr)
    {
        const auto* pixel_data = static_cast<const uint32*>(data);
        const std::vector colors(pixel_data, pixel_data + pixel_count);
        stbi_image_free(data);

        Renderer::Instance().CreateTexture(*this, width, height, colors, type);
        return;
    }

    SDL_Log("Failed to load image: %s", stbi_failure_reason());
}

Texture::~Texture() { Renderer::Instance().DeleteTexture(*this); }

Mesh::Mesh(const std::string& path, const uint32 index)
{
    const auto model_handle = FileResource::Load<Model>(path);
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

    const aiMaterial& material = *scene->mMaterials[model_mesh.mMaterialIndex];
    auto diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE);
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

    auto specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR);
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

    Renderer::Instance().CreateMesh(*this, vertices, indices, textures);
}

Mesh::~Mesh() { Renderer::Instance().DeleteMesh(*this); }

void Renderer::SetupBackend(const std::string& backend_name)
{
    if (backend_name == "OpenGL")
    {
        renderer = new OpenGLRenderer;
        return;
    }

    renderer = new SDL3GPURenderer;
}
