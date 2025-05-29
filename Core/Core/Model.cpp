#include "Model.hpp"

#include <vector>

#include <SDL3/SDL_log.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

Model::Model(const std::string& path)
{
    Assimp::Importer importer;
    const aiScene* scene =
        importer.ReadFile(path, aiProcess_Triangulate | aiProcess_MakeLeftHanded | aiProcess_JoinIdenticalVertices);

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

            position = glm::vec3{mesh_vertices[i].x, mesh_vertices[i].y, mesh_vertices[i].z};
            if (mesh_colors != nullptr) color = glm::vec3{mesh_colors[i].r, mesh_colors[i].g, mesh_colors[i].b};
            if (mesh_tex_coords != nullptr) tex_coord = glm::vec2{mesh_tex_coords[i].x, mesh_tex_coords[i].y};
        }

        const aiFace* mesh_faces = model_mesh.mFaces;

        const size_t face_count = static_cast<int>(model_mesh.mNumFaces);
        std::vector<std::uint32_t> indices(face_count * 3);

        for (size_t i = 0; i < face_count; i++)
        {
            std::memcpy(&indices[i * 3], mesh_faces[i].mIndices, sizeof(std::uint32_t) * 3);
        }

        meshes.push_back(std::make_shared<Mesh>(Renderer::Instance().CreateMesh(vertices, indices)));
    }
}