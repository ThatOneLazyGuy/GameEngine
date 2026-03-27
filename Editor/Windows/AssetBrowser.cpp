#include "AssetBrowser.hpp"

#include "Editor.hpp"
#include "Importers/ImporterBase.hpp"

#include <Tools/FileDialogs.hpp>

#include <imgui.h>

#include <filesystem>

namespace
{
    void DragResource(const std::string_view& path, const UUID& uuid)
    {
        if (!ImGui::BeginDragDropSource()) return;

        const std::string_view type_hash_string = ResourceManager::GetResourceTypeID(uuid);
        const std::string uuid_bytes = uuid.bytes();

        ImGui::SetDragDropPayload(type_hash_string.data(), uuid_bytes.data(), uuid_bytes.size());
        ImGui::Text("%s", path.data());

        ImGui::EndDragDropSource();
    }
} // namespace

AssetBrowser::AssetBrowser()
{
    name = "AssetBrowser";
    default_open = false;
}

void AssetBrowser::Display()
{
    if (ImGui::Button("Import Object +"))
    {
        const auto& importer_infos = ImporterBase::GetImporterInfos();

        std::vector<std::string> filters{"All Files", "*"};
        filters.reserve(filters.size() + (importer_infos.size() * 2));

        for (const ImporterInfo& info : importer_infos)
        {
            filters.emplace_back(info.description);
            filters.emplace_back(info.file_types);
        }

        const std::string path = FileDialogs::OpenFile("Import Asset", "./Assets", filters);
        if (!path.empty()) Editor::asset_registry->Import(path);
    }

    const auto& import_infos = Editor::asset_registry->GetImportInfos();
    for (const auto& import_info : import_infos)
    {
        constexpr ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanFullWidth;
        if (ImGui::TreeNodeEx(import_info.path.data(), node_flags))
        {
            for (const UUID& uuid : import_info.derived_assets)
            {
                const std::string& asset_path = Editor::asset_registry->GetAssetPath(uuid);
                ImGui::TreeNodeEx(asset_path.c_str(), node_flags | ImGuiTreeNodeFlags_Leaf);
                DragResource(asset_path, uuid);
            }
        }
    }
}