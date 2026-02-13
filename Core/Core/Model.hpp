#pragma once

#include <string>

#include <assimp/Importer.hpp>

#include "Rendering/Renderer.hpp"

class ModelParser final : public Resource<ModelParser>
{
  public:
    explicit ModelParser(const std::string& path);

    [[nodiscard]] ResourceRef<Mesh> GetMesh(uint32 index) const;
    [[nodiscard]] std::vector<ResourceRef<Mesh>> GetMeshes() const;

    Assimp::Importer importer;

  private:
    std::string path;
};