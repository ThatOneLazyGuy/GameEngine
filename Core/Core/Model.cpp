#include "Model.hpp"

#include <SDL3/SDL_log.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "Core/ECS.hpp"


ModelParser::ModelParser(const std::string& path)
{
    importer.ReadFile(path, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_PreTransformVertices);
}
Handle<Mesh> ModelParser::GetMesh(const uint32 index) const
{
    if (index >= importer.GetScene()->mNumMeshes)
    {
        SDL_Log("Failed to get mesh at index: %u", index);
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