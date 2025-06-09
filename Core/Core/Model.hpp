#pragma once

#include "Renderer.hpp"

#include <memory>
#include <string>

class Model
{
  public:
    explicit Model(const std::string& path);

    std::vector<Handle<Mesh>> meshes;
};