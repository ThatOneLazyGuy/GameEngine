#include "ObjectImporter.hpp"

#include "Editor.hpp"

#include <Rendering/Renderer.hpp>

#include <assimp/Importer.hpp>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

void ObjImporter::ImportAsset(const std::string& path)
{
    constexpr int import_flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_PreTransformVertices;

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, import_flags);

    if (scene == nullptr)
    {
        Log::Error("Failed to import object, {}: {}", importer.GetErrorString(), path);
        return;
    }

    const usize last_separator = path.find_last_of('/');
    const std::string base_path = path.substr(0, (last_separator == std::string::npos) ? last_separator : last_separator + 1);

    for (uint32 mesh_index = 0; mesh_index < scene->mNumMeshes; mesh_index++)
    {
        const usize extension_position = path.find_last_of('.');
        const std::string mesh_path = path.substr(0, extension_position) + "-" + std::to_string(mesh_index) + ".mesh";

        const aiMesh& model_mesh = *scene->mMeshes[mesh_index];
        const aiVector3D* mesh_vertices = model_mesh.mVertices;
        const aiColor4D* mesh_colors = model_mesh.mColors[0];
        const aiVector3D* mesh_tex_coords = model_mesh.mTextureCoords[0];

        const usize vertex_count = model_mesh.mNumVertices;
        std::vector<Vertex> vertices{vertex_count};

        for (usize i = 0; i < vertex_count; i++)
        {
            Vertex& vertex = vertices[i];

            vertex.position = float3{mesh_vertices[i].x, mesh_vertices[i].y, mesh_vertices[i].z};
            if (mesh_colors != nullptr) vertex.color = float3{mesh_colors[i].r, mesh_colors[i].g, mesh_colors[i].b};
            if (mesh_tex_coords != nullptr) vertex.tex_coord = float2{mesh_tex_coords[i].x, mesh_tex_coords[i].y};
        }

        const aiFace* mesh_faces = model_mesh.mFaces;

        const usize face_count = model_mesh.mNumFaces;
        std::vector<uint32> indices(face_count * 3);
        for (usize i = 0; i < face_count; i++)
        {
            std::memcpy(&indices[i * 3], mesh_faces[i].mIndices, sizeof(uint32) * 3);
        }

        Files::BinaryWriteStream file{mesh_path};
        if (!file.IsOpen())
        {
            Log::Error("Failed to import mesh, couldn't open file: {}", mesh_path);
            continue;
        }

        Editor::asset_registry->RegisterPath<Mesh>(mesh_path);

        file << vertices;
        file << indices;

        const aiMaterial& material = *scene->mMaterials[model_mesh.mMaterialIndex];

        const usize texture_count = material.GetTextureCount(aiTextureType_DIFFUSE);
        file << texture_count;

        for (uint32 i = 0; i < texture_count; i++)
        {
            aiString string;
            material.GetTexture(aiTextureType_DIFFUSE, i, &string);

            const std::string full_texture_path = base_path + string.C_Str();
            const UUID material_uuid = Editor::asset_registry->RegisterPath<Texture>(full_texture_path);

            file << material_uuid;
        }
    }
}