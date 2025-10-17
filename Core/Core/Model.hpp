#pragma once

#include <string>

#include <assimp/Importer.hpp>

#include "Rendering/Renderer.hpp"

struct ModelParser final : FileResource
{
    explicit ModelParser(const std::string& path);

    [[nodiscard]] Handle<Mesh> GetMesh(uint32 index) const;
    [[nodiscard]] std::vector<Handle<Mesh>> GetMeshes() const;

    Assimp::Importer importer;
};