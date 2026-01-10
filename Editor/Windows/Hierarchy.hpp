#pragma once

#include <imgui/imgui.h>

#include "Core/ECS.hpp"
#include "Core/Components/Transform.hpp"

inline void EntityDrag(const ECS::Entity& entity)
{
    if (!ImGui::BeginDragDropSource()) return;

    ImGui::SetDragDropPayload("Entity", &entity, sizeof(ECS::Entity));
    ImGui::Text("%s", entity.Name().c_str());

    ImGui::EndDragDropSource();
}

inline void EntityDrop(const ECS::Entity& entity)
{
    if (!ImGui::BeginDragDropTarget()) return;

    const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Entity");
    if (payload != nullptr) { static_cast<ECS::Entity*>(payload->Data)->SetParent(entity); }

    ImGui::EndDragDropTarget();
}

inline void RecurseHierarchy(const ECS::Entity& entity)
{
    constexpr ImGuiTreeNodeFlags default_flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow |
                                                 ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                                 ImGuiTreeNodeFlags_FramePadding;

    const uint64 id = entity.GetID();
    ImGui::PushID(static_cast<int>(id & 0xFFFFFFFF));
    ImGui::PushID(static_cast<int>(id >> 32));

    const std::vector<ECS::Entity> children = entity.GetChildren();

    ImGuiTreeNodeFlags flags = default_flags;
    if (children.empty()) flags |= ImGuiTreeNodeFlags_Leaf;

    const bool node_open = ImGui::TreeNodeEx(entity.Name().c_str(), flags);
    EntityDrag(entity);
    EntityDrop(entity);
    if (node_open)
    {
        for (const ECS::Entity& child : children)
        {
            RecurseHierarchy(child);
        }

        ImGui::TreePop();
    }

    ImGui::PopID();
    ImGui::PopID();
}

inline void Hierarchy()
{
    if (!ImGui::Begin("Hierarchy", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar))
    {
        ImGui::End();
        return;
    }

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::MenuItem("Add")) ECS::CreateEntity();

        ImGui::EndMenuBar();
    }


    const float line_height = ImGui::GetFrameHeight();
    const float content_width = ImGui::GetContentRegionAvail().x;

    usize i = 0;
    ImVec2 start_pos = ImGui::GetCursorScreenPos();

    const ImGuiWindow* current_window = ImGui::GetCurrentWindow();
    while (current_window->Rect().Max.y > start_pos.y)
    {
        ++i;
        if (i & 0b1) continue;

        const ImColor color = ImGui::GetStyle().Colors[ImGuiCol_MenuBarBg];
        current_window->DrawList->AddRectFilled(start_pos, start_pos + ImVec2{content_width, line_height}, color, 2.0f);

        start_pos += ImVec2{0.0f, line_height * 2.0f};
    }

    const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
    if (ImGui::BeginDragDropTargetCustom(ImRect{cursor_pos, cursor_pos + ImGui::GetContentRegionAvail()}, ImGui::GetID("DropRegion")))
    {

        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Entity");
        if (payload != nullptr) { static_cast<ECS::Entity*>(payload->Data)->RemoveParent(); }

        ImGui::EndDragDropTarget();
    }

    const flecs::world& world = ECS::GetWorld();
    const flecs::query query = world.query_builder().with<Transform>().without(flecs::ChildOf, flecs::Wildcard).build();

    ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, 0.0f);
    world.defer_begin();
    query.each(&RecurseHierarchy);
    world.defer_end();
    ImGui::PopStyleVar();


    ImGui::End();
}