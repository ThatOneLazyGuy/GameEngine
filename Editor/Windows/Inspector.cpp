#include "Inspector.hpp"

#include "Editor/Editor.hpp"

#include <Core/Components/Transform.hpp>
#include <Core/Physics/Physics.hpp>
#include <core/Rendering/Renderer.hpp>

#include <imgui.h>
#include <imgui_stdlib.h>

namespace
{
    template <typename>
    constexpr std::string_view GetComponentName()
    {
        return "Unknown";
    }

    template <>
    constexpr std::string_view GetComponentName<Transform>()
    {
        return "Transform";
    }

    template <>
    constexpr std::string_view GetComponentName<Physics::BoxCollider>()
    {
        return "Box Collider";
    }

    template <>
    constexpr std::string_view GetComponentName<Physics::SphereCollider>()
    {
        return "Sphere Collider";
    }

    template <>
    constexpr std::string_view GetComponentName<ResourceRef<Mesh>>()
    {
        return "Mesh Renderer";
    }

    template <typename Component>
    void InspectComponent(Component&)
    {
    }

    template <>
    void InspectComponent(Transform& transform)
    {
        float3 position = transform.GetPosition();
        if (ImGui::DragFloat3("Position", &position.x())) transform.SetPosition(position);

        Quat rotation = transform.GetRotation();
        if (ImGui::DragFloat4("Rotation", &rotation.x())) transform.SetRotation(rotation);

        float3 scale = transform.GetScale();
        if (ImGui::DragFloat3("Scale", &scale.x())) transform.SetScale(scale);
    }

    template <>
    void InspectComponent(Physics::SphereCollider& sphere_collider)
    {
        constexpr float min = std::numeric_limits<float>::epsilon();
        constexpr float max = std::numeric_limits<float>::max();

        float radius = sphere_collider.GetRadius();
        if (ImGui::DragFloat("Radius", &radius, 0.05f, min, max, "%.3f", ImGuiSliderFlags_AlwaysClamp)) sphere_collider.SetRadius(radius);
    }

    template <>
    void InspectComponent(ResourceRef<Mesh>& mesh)
    {
        std::string resource_string = (mesh.Valid() ? mesh.GetUUID().str() : "");

        ImGui::BeginDisabled();
        ImGui::InputText("Mesh", &resource_string);
        ImGui::EndDisabled();

        if (ImGui::BeginDragDropTarget())
        {
            const std::string_view type_id = mesh->GetTypeID();

            const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(type_id.data());
            if (payload != nullptr)
            {
                const UUID uuid{static_cast<const uint8*>(payload->Data)};
                mesh = ResourceManager::Load<Mesh>(uuid);
            }

            ImGui::EndDragDropTarget();
        }
    }

    template <typename Component>
    void DisplayComponent(ECS::Entity& entity)
    {
        if (!entity.Has<Component>()) return;

        constexpr std::string_view component_name = GetComponentName<Component>();
        const bool header_open = ImGui::CollapsingHeader(component_name.data(), ImGuiTreeNodeFlags_DefaultOpen);
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Remove Component")) entity.Remove<Component>();

            ImGui::EndPopup();
        }
        if (header_open)
        {
            Component& component = entity.GetComponent<Component>();

            InspectComponent(component);
        }
    }

    template <typename... Components>
    void DisplayComponents(ECS::Entity& entity)
    {
        (DisplayComponent<Components>(entity), ...);
    }

    template <typename Component>
    void AddComponent(ECS::Entity& entity)
    {
        ImGui::BeginDisabled(entity.Has<Component>());

        constexpr std::string_view component_name = GetComponentName<Component>();
        if (ImGui::MenuItem(component_name.data())) entity.AddComponent<Component>();

        ImGui::EndDisabled();
    }

    template <typename... Components>
    void DisplayAddComponents(ECS::Entity& entity)
    {
        if (ImGui::IsPopupOpen("Add Component"))
        {
            ImGui::SetNextWindowPos(ImVec2{ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y});
            ImGui::SetNextWindowSize(ImVec2{ImGui::GetItemRectSize().x, 0.0f});
        }

        if (!ImGui::BeginPopup("Add Component", ImGuiWindowFlags_NoMove)) return;

        (AddComponent<Components>(entity), ...);

        ImGui::EndPopup();
    }

} // namespace

Inspector::Inspector()
{
    name = "Inspector";
    default_open = true;
}

void Inspector::Display()
{
    if (Editor::selected_entities.Empty()) return;

    ECS::Entity selected_entity = Editor::selected_entities.Front();
    const flecs::world world = selected_entity.GetWorld();

    world.defer_begin();
    DisplayComponents<Transform, Physics::BoxCollider, Physics::SphereCollider, ResourceRef<Mesh>>(selected_entity);
    world.defer_end();

    ImGui::Separator();

    const float half_avail = ImGui::GetContentRegionAvail().x / 2.0f;
    const float half_size = (ImGui::CalcTextSize("Add Component").x + ImGui::GetStyle().FramePadding.x) / 2.0f;

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (half_avail - half_size));

    if (ImGui::Button("Add Component")) ImGui::OpenPopup("Add Component");
    DisplayAddComponents<Transform, Physics::BoxCollider, Physics::SphereCollider, ResourceRef<Mesh>>(selected_entity);
}