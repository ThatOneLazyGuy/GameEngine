#include "Model.hpp"

#include <map>
#include <vector>

#include <SDL3/SDL_log.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace
{
    std::map<std::string, std::shared_ptr<Texture>> test;

    std::vector<std::shared_ptr<Texture>> loadMaterialTextures(const aiMaterial& material, const aiTextureType type)
    {
        std::vector<std::shared_ptr<Texture>> textures;
        for (std::uint32_t i = 0; i < material.GetTextureCount(type); i++)
        {
            aiString string;
            material.GetTexture(type, i, &string);

            auto iterator = test.find(string.C_Str());
            if (iterator != test.end())
            {
                textures.push_back(iterator->second);
                continue;
            }


            Texture texture = Renderer::Instance().CreateTexture(
                string.C_Str(), (type == aiTextureType_DIFFUSE ? Texture::Type::DIFFUSE : Texture::Type::SPECULAR)
            );
            auto texture_handle = std::make_shared<Texture>(texture);
            textures.push_back(texture_handle);
            test[string.C_Str()] = texture_handle;
        }
        return textures;
    }
} // namespace

Model::Model(const std::string& path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);

    if (scene == nullptr)
    {
        SDL_Log("Failed to load asset: ", path.c_str());
        return;
    }


    for (size_t mesh_index = 0; mesh_index < scene->mNumMeshes; mesh_index++)
    {
        const aiMesh& model_mesh = *scene->mMeshes[mesh_index];
        const aiVector3D* mesh_vertices = model_mesh.mVertices;
        const aiColor4D* mesh_colors = model_mesh.mColors[0];
        const aiVector3D* mesh_tex_coords = model_mesh.mTextureCoords[0];

        const size_t vertex_count = model_mesh.mNumVertices;
        std::vector<Vertex> vertices(vertex_count);

        for (size_t i = 0; i < vertex_count; i++)
        {
            auto& [position, color, tex_coord] = vertices[i];

            position = Vec3{mesh_vertices[i].x, mesh_vertices[i].y, mesh_vertices[i].z};
            if (mesh_colors != nullptr) color = Vec3{mesh_colors[i].r, mesh_colors[i].g, mesh_colors[i].b};
            if (mesh_tex_coords != nullptr) tex_coord = Vec2{mesh_tex_coords[i].x, mesh_tex_coords[i].y};
        }

        const aiFace* mesh_faces = model_mesh.mFaces;

        const size_t face_count = static_cast<int>(model_mesh.mNumFaces);
        std::vector<std::uint32_t> indices(face_count * 3);

        for (size_t i = 0; i < face_count; i++)
        {
            std::memcpy(&indices[i * 3], mesh_faces[i].mIndices, sizeof(std::uint32_t) * 3);
        }

        std::vector<std::shared_ptr<Texture>> textures;

        const aiMaterial& material = *scene->mMaterials[model_mesh.mMaterialIndex];
        auto diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE);
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        auto specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR);
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

        meshes.push_back(std::make_shared<Mesh>(Renderer::Instance().CreateMesh(vertices, indices, textures)));
    }
}