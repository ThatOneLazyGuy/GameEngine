#include "Model.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

Model::Model(const std::string& path)
{
    importer.ReadFile(path, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_PreTransformVertices);
}