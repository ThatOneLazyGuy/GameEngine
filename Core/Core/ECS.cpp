#include "ECS.hpp"

#include <flecs.h>

namespace
{
    flecs::world* world;
}

namespace ECS
{
    void Init() { world = new flecs::world(); }

    void Exit() { delete world; }

    Entity CreateEntity(const std::string& string)
    {
        return Entity{world->entity(string.c_str())}.AddComponent<Transform>();
    }

    flecs::world& GetWorld() { return *world; }
} // namespace ECS