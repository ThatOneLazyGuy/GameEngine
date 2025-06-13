#pragma once

#include <string>

#include <assimp/Importer.hpp>

#include "Renderer.hpp"

struct Model final : FileResource
{
    explicit Model(const std::string& path);

    Assimp::Importer importer;
};