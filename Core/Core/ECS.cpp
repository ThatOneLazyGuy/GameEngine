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

    flecs::world& GetWorld() { return *world; }
} // namespace ECS