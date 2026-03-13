#include "AssetBrowser.hpp"

#include "Editor/Editor.hpp"
#include "Editor/Importers/ImporterBase.hpp"

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

        std::vector<std::string> filters;
        filters.reserve(importer_infos.size() * 2);

        for (const ImporterInfo& info : importer_infos)
        {
            filters.emplace_back(info.description);
            filters.emplace_back(info.file_types);
        }

        const std::string path = FileDialogs::OpenFile("Import Asset", "./Assets", filters);
        if (!path.empty()) ImporterBase::Import(path);
    }

    const auto& asset_mapping = Editor::asset_registry->GetAssetFileMapping();
    for (const auto& [path, uuid] : asset_mapping)
    {
        ImGui::Selectable(path.data());
        DragResource(path, uuid);
    }
}