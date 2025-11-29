#include "Model.hpp"

#include "Tools/Logging.hpp"
#include "Tools/Types.hpp"
#include "Resource.hpp"

#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "Tools/Files.hpp"

ModelParser::ModelParser(const std::string& path)
{
    // TODO: Figure out how to load from memory, works fine except fails due to texture issues, probably doesn't have access to .mtl files.
    importer.ReadFile(path, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_PreTransformVertices);
}

Handle<Mesh> ModelParser::GetMesh(const uint32 index) const
{
    if (index >= importer.GetScene()->mNumMeshes)
    {
        Log::Error("Failed to get mesh at index: {}", index);
        return nullptr;
    }
    return Resource::Load<Mesh>(GetPath(), index);
}

std::vector<Handle<Mesh>> ModelParser::GetMeshes() const
{
    const uint32 meshes_count = importer.GetScene()->mNumMeshes;

    std::vector<Handle<Mesh>> meshes(meshes_count);
    for (uint32 i = 0; i < meshes_count; i++)
    {
        meshes[i] = Resource::Load<Mesh>(GetPath(), i);
    }

    return meshes;
}