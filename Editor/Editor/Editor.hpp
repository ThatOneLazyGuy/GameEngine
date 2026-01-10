#pragma once

#include <Core/ECS.hpp>

namespace Editor
{
    struct EditorOnly
    {
    };

    ECS::Entity CreateEntity(std::string name = {});
} // namespace Editor