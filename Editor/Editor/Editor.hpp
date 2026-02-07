#pragma once

#include <Core/ECS.hpp>
#include <Tools/SequentialSet.hpp>

namespace Editor
{

    struct EditorOnly
    {
    };

    ECS::Entity CreateEntity(std::string name = {});

    inline UniqueVector<ECS::Entity, ECS::EntityHasher> selected_entities;

} // namespace Editor