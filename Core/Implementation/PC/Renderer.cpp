#include "Core/Renderer.hpp"

#include "Implementation/OpenGL/Renderer.hpp"
#include "Implementation/SDL3/Renderer.hpp"

#include "Core/Model.hpp"

#include <SDL3/SDL_log.h>
#include <assimp/scene.h>

namespace
{
    std::vector<Handle<Texture>> loadMaterialTextures(const aiMaterial& material, const aiTextureType type)
    {
        std::vector<Handle<Texture>> textures;
        for (uint32 i = 0; i < material.GetTextureCount(type); i++)
        {
            aiString string;
            material.GetTexture(type, i, &string);

            auto texture_resource = FileResource::Load<Texture>(
                string.C_Str(), (type == aiTextureType_DIFFUSE ? Texture::Type::DIFFUSE : Texture::Type::SPECULAR)
            );
            textures.push_back(texture_resource);
        }
        return textures;
    }
} // namespace

Texture::Texture(const std::string& path, const Type type) { *this = Renderer::Instance().CreateTexture(path, type); }

void Texture::Delete() { Renderer::Instance().DeleteTexture(*this); }

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

    *this = Renderer::Instance().CreateMesh(vertices, indices, textures);
}

void Mesh::Delete() { Renderer::Instance().DeleteMesh(*this); }

void Renderer::SetupBackend(const std::string& backend_name)
{
    if (backend_name == "OpenGL")
    {
        renderer = new OpenGLRenderer;
        return;
    }

    renderer = new SDL3GPURenderer;
}
