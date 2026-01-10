#pragma once

#include "Math.hpp"

#include <flecs.h>

namespace ECS
{
    struct IgnoreTag
    {
    };

    class Entity : protected flecs::entity
    {
      public:
        static const Entity null;

        Entity() = default;
        Entity(const entity&& entity);

        [[nodiscard]] bool Valid() const;

        [[nodiscard]] std::string& Name();
        [[nodiscard]] const std::string& Name() const;

        [[nodiscard]] bool HasParent() const;
        void RemoveParent();

        void SetParent(const Entity& parent);
        [[nodiscard]] Entity GetParent() const;

        [[nodiscard]] std::vector<Entity> GetChildren() const;

        [[nodiscard]] uint64 GetID() const;

        template <typename Type>
        Type& AddComponent()
        {
            return add<Type>().template get_mut<Type>();
        }
        template <typename... Types>
        requires(sizeof...(Types) > 1)
        std::tuple<Types&...> AddComponent()
        {
            (add<Types>(), ...);
            return {get_mut<Types>()...};
        }

        template <typename Type>
        [[nodiscard]] Type& GetComponent()
        {
            return get_mut<Type>();
        }
        template <typename... Types>
        requires(sizeof...(Types) > 1)
        [[nodiscard]] std::tuple<Types&...> GetComponent()
        {
            return {get_mut<Types>()...};
        }

        template <typename Type>
        [[nodiscard]] const Type& GetComponent() const
        {
            return get<Type>();
        }
        template <typename... Types>
        requires(sizeof...(Types) > 1)
        [[nodiscard]] std::tuple<const Types&...> GetComponent() const
        {
            return {get<Types>()...};
        }

        template <typename... Tags>
        void AddTag()
        {
            (add<Tags>(), ...);
        }

        template <typename... Tags>
        void RemoveTag()
        {
            (remove<Tags>(), ...);
        }

        // Returns true if the entity has all the given tags/components.
        template <typename... Types>
        [[nodiscard]] bool Has() const
        {
            return (has<Types>() && ...);
        }
        // Returns true if the entity has any of the given tags/components.
        template <typename... Types>
        [[nodiscard]] bool HasAny() const
        {
            return (has<Types>() || ...);
        }
    };

    void Init();

    void Exit();

    Entity CreateEntity(std::string string = {});

    [[nodiscard]] flecs::world& GetWorld();
} // namespace ECS