#include "AssetBrowser.hpp"

#include "Editor/Editor.hpp"
#include "Editor/Importers/ImporterBase.hpp"

#include <Tools/FileDialogs.hpp>

#include <imgui.h>

#include <filesystem>

AssetBrowser::AssetBrowser()
{
    name = "AssetBrowser";
    default_open = false;
}

void AssetBrowser::Display()
{
    if (ImGui::Button("Import Object +"))
    {
        const std::string path = FileDialogs::OpenFile("Import Obj", "./Assets", {"Wavefront Object (*.obj)", "*.obj"});
        if (!path.empty()) ImportObject(path);
    }

    const auto& asset_mapping = Editor::asset_registry->GetAssetFileMapping();

    for (const auto& [path, UUID] : asset_mapping)
    {
        ImGui::Selectable(path.data());
    }
}