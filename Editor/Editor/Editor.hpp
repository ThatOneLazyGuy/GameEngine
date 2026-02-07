#pragma once

#include <Core/ECS.hpp>

namespace Editor
{

    namespace Internal
    {

        inline std::vector<ECS::Entity> selected_entities;

    }

    struct EditorOnly
    {
    };

    ECS::Entity CreateEntity(std::string name = {});

    inline ECS::Entity GetSelectedEntity()
    {
        if (Internal::selected_entities.empty()) return {};

        return Internal::selected_entities.front();
    }
    inline const std::vector<ECS::Entity>& GetSelectedEntities() { return Internal::selected_entities; }

    inline void AddSelectedEntity(const ECS::Entity& entity)
    {
        using namespace Internal;

        if (std::ranges::find(selected_entities, entity) != selected_entities.end()) return;

        selected_entities.emplace_back(entity);
    }
    template <typename Container>
    void AddSelectedEntities(const Container& container)
    {
        using namespace Internal;

        selected_entities.reserve(selected_entities.size() + std::size(container));

        const auto thing = std::ranges::remove_if(container, [](const ECS::Entity& entity) {
            return std::ranges::find(selected_entities, entity) == selected_entities.end();
        });

        selected_entities.insert(selected_entities.end(), std::begin(container), thing);
    }

    inline void SetSelectedEntity(const ECS::Entity& entity)
    {
        Internal::selected_entities.resize(1);
        Internal::selected_entities[0] = entity;
    }
    template <typename Container>
    void SetSelectedEntities(const Container& container)
    {
        using namespace Internal;

        const auto new_end = std::ranges::remove_if(container, [](const ECS::Entity& entity) {
            return std::ranges::find(selected_entities, entity) == selected_entities.end();
        });

        selected_entities = {std::begin(container), new_end};
    }

    inline bool IsEntitySelected(const ECS::Entity& entity)
    {
        return std::ranges::find(Internal::selected_entities, entity) != Internal::selected_entities.end();
    }

    inline void ClearSelectedEntities() { Internal::selected_entities.clear(); }

} // namespace Editor