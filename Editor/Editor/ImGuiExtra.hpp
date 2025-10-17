#pragma once

#include <vector>
#include <imgui.h>

namespace ImGui
{
    template <typename ItemType, typename ContainerType>
    void ApplyRequests(ImGuiMultiSelectIO* io, std::vector<ItemType>& selection, const ContainerType& items)
    {
        for (const auto& request : io->Requests)
        {
            switch (request.Type)
            {
            case ImGuiSelectionRequestType_None:
                break;

            case ImGuiSelectionRequestType_SetAll:
                selection.clear();
                if (request.Selected) selection.insert(selection.begin(), std::begin(items), std::end(items));
                break;

            case ImGuiSelectionRequestType_SetRange: {
                if (request.Selected)
                {
                    for (ImGuiSelectionUserData i = request.RangeFirstItem; i <= request.RangeLastItem; i++)
                    {
                        selection.push_back(items.at(i));
                    }
                }
                else
                {
                    for (ImGuiSelectionUserData i = request.RangeFirstItem; i <= request.RangeLastItem; i++)
                    {
                        selection.erase(std::ranges::find(selection, items.at(i)));
                    }
                }
            }
            break;
            }
        }
    }
} // namespace ImGui