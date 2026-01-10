#include "ECS.hpp"

#include "Components/Transform.hpp"
#include "Tools/Logging.hpp"

#include <flecs.h>

namespace
{
    flecs::world* world;
}

namespace ECS
{

    void Init() { world = new flecs::world{}; }

    void Exit() { delete world; }

    Entity const Entity::null = entity::null();

    Entity::Entity(const entity&& entity) : entity{entity}
    {
        if (is_valid()) add(flecs::OrderedChildren);
    }

    bool Entity::Valid() const { return is_valid(); }

    std::string& Entity::Name() { return GetComponent<std::string>(); }

    const std::string& Entity::Name() const { return GetComponent<std::string>(); }

    bool Entity::HasParent() const { return parent().is_valid(); }

    void Entity::RemoveParent()
    {
        if (!HasParent()) return;

        remove(flecs::ChildOf, parent());
    }

    void Entity::SetParent(const Entity& parent)
    {
        // Traverse the hierarchy to check that we do not create a cyclic relationship.
        Entity traverse = parent.GetParent();
        while (traverse.Valid())
        {
            if (*this == traverse)
            {
                Log::Warning("Can't set entity parent to a child of itself.");
                return;
            }

            traverse = traverse.GetParent();
        }

        child_of(parent);
    }

    Entity Entity::GetParent() const { return parent(); }

    std::vector<Entity> Entity::GetChildren() const
    {
        ecs_iter_t it = ecs_children(world(), *this);
        // We don't actually have to iterate since all entities have the "OrderedChildren" flag, this guarantees only one iterator
        if (!ecs_children_next(&it)) return {};

        std::vector<Entity> children;
        children.reserve(children.size() + it.count);

        for (sint32 i = 0; i < it.count; i++)
        {
            children.emplace_back(entity{world(), it.entities[i]});
        }

        return children;
    }

    uint64 Entity::GetID() const { return id(); }

    Entity CreateEntity(std::string string)
    {
        Entity entity{world->entity()};

        auto&& [name, transform] = entity.AddComponent<std::string, Transform>();
        name = (!string.empty() ? std::move(string) : "Entity_" + std::to_string(entity.GetID()));

        return entity;
    }

    flecs::world& GetWorld() { return *world; }
} // namespace ECS