#include "AssetBrowser.hpp"

#include "Editor/Editor.hpp"
#include "Editor/Importers/ImporterBase.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

#include <filesystem>

AssetBrowser::AssetBrowser()
{
    name = "AssetBrowser";
    default_open = false;
}

void AssetBrowser::Display()
{
    static std::string path_string;
    ImGui::InputText("Path", &path_string);

    ImGui::BeginDisabled(path_string.empty() || !std::filesystem::exists(path_string));
    if (ImGui::Button("Import Object")) ImportObject(path_string);
    ImGui::EndDisabled();

    const auto& asset_mapping = Editor::asset_registry->GetAssetFileMapping();

    for (const auto& [path, UUID] : asset_mapping)
    {
        ImGui::Selectable(path.data());
    }
}