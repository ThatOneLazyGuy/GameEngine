#include "Hierarchy.hpp"

#include "Editor.hpp"

#include <ECS.hpp>
#include <Transform.hpp>

#include <imgui.h>
#include <imgui_internal.h>

namespace
{
    // Custom entity selection ApplyRequests function for ImGui, works with ImGui's built in multi-select system.
    bool ApplyRequests(
        ImGuiMultiSelectIO* io, UniqueVector<ECS::Entity, ECS::EntityHasher>& selection, const std::vector<ECS::Entity>& entities
    )
    {
        bool set_all = false;

        for (const auto& request : io->Requests)
        {
            switch (request.Type)
            {
            case ImGuiSelectionRequestType_None:
                break;

            case ImGuiSelectionRequestType_SetAll:
                selection.Clear();
                // "Entities" will only be populated after looping through entities, but the SetAll (ctrl + A) call can happen before the loop.
                set_all = request.Selected;
                break;

            case ImGuiSelectionRequestType_SetRange:
            {
                if (request.Selected)
                {
                    for (ImGuiSelectionUserData i = request.RangeFirstItem; i <= request.RangeLastItem; i++)
                    {
                        selection.PushBack(entities.at(static_cast<usize>(i)));
                    }
                }
                else
                {
                    for (ImGuiSelectionUserData i = request.RangeFirstItem; i <= request.RangeLastItem; i++)
                    {
                        selection.Erase(entities.at(static_cast<usize>(i)));
                    }
                }
            }
            break;
            }
        }

        return set_all;
    }

    void EntityDrag(const ECS::Entity& entity)
    {
        if (!ImGui::BeginDragDropSource()) return;

        ImGui::SetDragDropPayload("Entity", &entity, sizeof(ECS::Entity));
        ImGui::Text("%s", entity.Name().c_str());

        ImGui::EndDragDropSource();
    }

    void EntityDrop(const ECS::Entity& entity)
    {
        if (!ImGui::BeginDragDropTarget()) return;

        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Entity");
        if (payload != nullptr) { static_cast<ECS::Entity*>(payload->Data)->SetParent(entity); }

        ImGui::EndDragDropTarget();
    }

    void RecurseHierarchy(const ECS::Entity& entity, std::vector<ECS::Entity>& entities)
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
        if (Editor::selected_entities.Contains(entity)) flags |= ImGuiTreeNodeFlags_Selected;

        ImGui::SetNextItemSelectionUserData(static_cast<ImGuiSelectionUserData>(entities.size()));
        entities.push_back(entity);

        const bool node_open = ImGui::TreeNodeEx(entity.Name().c_str(), flags);

        EntityDrag(entity);
        EntityDrop(entity);
        if (node_open)
        {
            for (const ECS::Entity& child : children)
            {
                RecurseHierarchy(child, entities);
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
        ImGui::PopID();
    }

} // namespace

Hierarchy::Hierarchy()
{
    name = "Hierarchy";
    window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar;
    default_open = true;
}

void Hierarchy::DisplayMenuBar()
{
    if (ImGui::MenuItem("Add")) ECS::CreateEntity();
}

void Hierarchy::Display()
{
    const float line_height = ImGui::GetFrameHeight();
    const ImVec2 content_region = ImGui::GetContentRegionAvail();

    usize i = 0;
    ImVec2 start_pos = ImGui::GetCursorScreenPos();

    const ImGuiWindow* current_window = ImGui::GetCurrentWindow();
    while (current_window->Rect().Max.y > start_pos.y)
    {
        ++i;
        if (i & 0b1) continue;

        const ImColor color = ImGui::GetStyle().Colors[ImGuiCol_MenuBarBg];
        current_window->DrawList->AddRectFilled(start_pos, start_pos + ImVec2{content_region.x, line_height}, color, 2.0f);

        start_pos += ImVec2{0.0f, line_height * 2.0f};
    }

    const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
    if (ImGui::BeginDragDropTargetCustom(ImRect{cursor_pos, cursor_pos + content_region}, ImGui::GetID("DropRegion")))
    {

        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Entity");
        if (payload != nullptr) { static_cast<ECS::Entity*>(payload->Data)->RemoveParent(); }

        ImGui::EndDragDropTarget();
    }

    const flecs::world& world = ECS::GetWorld();
    const flecs::query query =
        world.query_builder().with<Transform>().without(flecs::ChildOf, flecs::Wildcard).without<ECS::IgnoreTag>().build();


    ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, 0.0f);


    constexpr ImGuiMultiSelectFlags flags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_ClearOnClickVoid |
                                            ImGuiMultiSelectFlags_BoxSelect1d | ImGuiMultiSelectFlags_SelectOnClickRelease;


    std::vector<ECS::Entity> entities;
    ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(flags, static_cast<int>(Editor::selected_entities.Size()));
    const bool select_all = ApplyRequests(ms_io, Editor::selected_entities, entities);

    world.defer_begin();
    query.each([&entities](const ECS::Entity& entity) { RecurseHierarchy(entity, entities); });
    world.defer_end();

    if (select_all) Editor::selected_entities.PushBack(entities);

    ms_io = ImGui::EndMultiSelect();
    ApplyRequests(ms_io, Editor::selected_entities, entities);

    ImGui::PopStyleVar();
}