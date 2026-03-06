#pragma once

#include "Editor/Editor.hpp"
#include "Editor/ShaderCompiler.hpp"

#include <Core/Rendering/Renderer.hpp>
#include <Core/ResourceManager.hpp>
#include <Tools/Logging.hpp>
#include <Tools/Types.hpp>
#include <Tools/Files.hpp>

#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include <fstream>

// TODO: Update to not use std::TypeInfo::hash_code since this isn't guaranteed to be the same across devices or even across invocations of the program.
template <ResourceType ResourceType>
void WriteMetaDataFile(const std::string& asset_path)
{
    const uint8* type_hash_data = reinterpret_cast<const uint8*>(&ResourceType::type_hash);
    Files::WriteBinary(asset_path + ".meta", std::span{type_hash_data, type_hash_data + sizeof(usize)});
}

inline void ImportObject(const std::string& path)
{
    Assimp::Importer importer;

    constexpr int import_flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_PreTransformVertices;
    const aiScene* scene = importer.ReadFile(path, import_flags);

    if (scene == nullptr)
    {
        Log::Error("Failed to import object: {}", path);
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

        std::ofstream file{mesh_path, std::ios::trunc | std::ios::binary};
        if (!file.is_open())
        {
            Log::Error("Failed to import mesh, couldn't open file: {}", mesh_path);
            continue;
        }

        Editor::asset_registry->RegisterPath(mesh_path);
        WriteMetaDataFile<Mesh>(mesh_path);

        const usize vertices_count = vertices.size();
        file.write(reinterpret_cast<const char*>(&vertices_count), sizeof(usize));
        file.write(reinterpret_cast<const char*>(vertices.data()), static_cast<sint32>(vertices.size() * sizeof(Vertex)));

        const usize indices_count = indices.size();
        file.write(reinterpret_cast<const char*>(&indices_count), sizeof(usize));
        file.write(reinterpret_cast<const char*>(indices.data()), static_cast<sint32>(indices.size() * sizeof(uint32)));

        const aiMaterial& material = *scene->mMaterials[model_mesh.mMaterialIndex];

        const usize texture_count = material.GetTextureCount(aiTextureType_DIFFUSE);
        file.write(reinterpret_cast<const char*>(&texture_count), sizeof(usize));

        for (uint32 i = 0; i < texture_count; i++)
        {
            aiString string;
            material.GetTexture(aiTextureType_DIFFUSE, i, &string);

            const std::string full_texture_path = base_path + string.C_Str();
            const UUID material_uuid = Editor::asset_registry->RegisterPath(full_texture_path);
            WriteMetaDataFile<Texture>(full_texture_path);

            const std::string uuid_bytes = material_uuid.bytes();
            const usize bytes_size = uuid_bytes.size();

            file.write(reinterpret_cast<const char*>(&bytes_size), sizeof(usize));
            file << uuid_bytes;
        }
    }
}

// TODO: Actually correctly implement importing and loading of graphics pipelines.
// INCORRECT, USES << OPERATOR WHICH WRITES THE STRING VERSION TO THE FILE INSTEAD OF THE BYTES.
inline void ImportGraphicsPipeline(const std::string& path)
{
    Editor::asset_registry->RegisterPath(path);
    const GraphicsPipelineSettings settings = ShaderCompiler::CompileGraphicsShaders(path);

    const std::string metadata_path = path.substr(0, path.find_last_of('.')) + ".metadata";

    std::ofstream file{metadata_path, std::ios::trunc | std::ios::binary};
    if (!file.is_open())
    {
        Log::Error("Failed to import graphics pipeline, couldn't open file: {}", metadata_path);
        return;
    }

    file << settings.vertex_attributes.size();
    for (const VertexAttribute& attribute : settings.vertex_attributes)
    {
        file << attribute.semantic_name.size();
        file << attribute.semantic_name;

        file << attribute.element_type;
        file << attribute.element_count;
    }

    file << settings.uniform_sizes.size();
    for (const auto& [name, size] : settings.uniform_sizes)
    {
        file << name.size();
        file << name;

        file << size;
    }

    file.write(reinterpret_cast<const char*>(&settings.vertex_info), sizeof(VertexAttribute));
    file.write(reinterpret_cast<const char*>(&settings.fragment_info), sizeof(VertexAttribute));

    file << path.size();
    file << path;
}