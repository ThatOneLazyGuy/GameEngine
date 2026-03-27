#include "Editor.hpp"


#include "ImGuiExtra.hpp"
#include "ImGuiPlatform.hpp"
#include "ShaderCompiler.hpp"

#include "Windows/WindowBase.hpp"
#include "Importers/ImporterBase.hpp"

#include <ECS.hpp>
#include <Time.hpp>
#include <Window.hpp>
#include <Physics/Physics.hpp>
#include <ResourceManager.hpp>
#include <Rendering/Renderer.hpp>
#include <Rendering/RenderPassInterface.hpp>
#include <Files.hpp>

#include <SDL3/SDL_mouse.h>

#include <imgui.h>

#include <filesystem>
#include <numeric>

namespace
{
    void InitFonts()
    {
        const ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();
        io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/Verdana.ttf", 23.0f);
    }

    std::vector<EditorWindowBase*> windows;
} // namespace

// NOLINTBEGIN(misc-use-anonymous-namespace)
namespace Editor
{
    static void Init()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigWindowsMoveFromTitleBarOnly = true;
        io.ConfigDockingAlwaysTabBar = true;
        io.ConfigViewportsNoDecoration = false;
        io.ConfigViewportsNoAutoMerge = true;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        InitFonts();

        ImGui::StyleColorsDark();
        ImGui::PlatformInit(Renderer::GetBackendName());

        const auto& window_creators = EditorWindowBase::GetWindowCreators();
        windows.reserve(window_creators.size());
        for (auto& creator : window_creators)
        {
            windows.push_back(creator());
        }
    }

    static void MainMenuBar()
    {
        if (!ImGui::BeginMainMenuBar()) return;

        if (ImGui::BeginMenu("Window"))
        {
            for (EditorWindowBase* window : windows)
            {
                if (ImGui::MenuItem(window->GetName().data(), nullptr, window->is_open)) window->is_open = !window->is_open;
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    static void DisplayWindows()
    {
        for (EditorWindowBase* window : windows)
        {
            if (!window->is_open) continue;

            if (ImGui::Begin(window->GetName().data(), &window->is_open, window->GetWindowFlags()))
            {
                if (ImGui::BeginMenuBar())
                {
                    window->DisplayMenuBar();
                    ImGui::EndMenuBar();
                }

                window->Display();
            }
            ImGui::End();
        }
    }

    static void Update()
    {
        ImGui::PlatformNewFrame();

        MainMenuBar();
        ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

        DisplayWindows();

        static std::vector<std::string> selected;
        static const std::vector<std::string> items{
            "item0", "item1", "item2", "item3", "item4", "item5", "item6", "item7", "item8", "item9"
        };
        if (ImGui::Begin("Item window", nullptr, ImGuiWindowFlags_NoCollapse))
        {

            ImGui::Text("Delta time: %f", Time::GetDeltaTime());
            const sint32 frame_rate = static_cast<int>(1.0f / Time::GetDeltaTime());
            ImGui::Text("Frame rate: %i", frame_rate);
            ImGui::NewLine();

            constexpr ImGuiMultiSelectFlags flags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_ClearOnClickVoid |
                                                    ImGuiMultiSelectFlags_BoxSelect1d | ImGuiMultiSelectFlags_SelectOnClickRelease;

            ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(flags, static_cast<int>(selected.size()), static_cast<int>(items.size()));
            ImGui::ApplyRequests(ms_io, selected, items);
            const int item_count = static_cast<int>(items.size());
            for (int i = 0; i < item_count; i++)
            {
                ImGui::PushID(i);
                const bool item_is_selected = std::ranges::find(selected, items[i]) != selected.end();
                ImGui::SetNextItemSelectionUserData(i);
                ImGui::Selectable(items[i].c_str(), item_is_selected);
                ImGui::PopID();
            }
            ms_io = ImGui::EndMultiSelect();
            ImGui::ApplyRequests(ms_io, selected, items);
        }
        ImGui::End();

        ImGui::PlatformEndFrame();
    }

    ECS::Entity CreateEntity(std::string name)
    {
        ECS::Entity editor_entity = ECS::CreateEntity(std::move(name));
        editor_entity.AddTag<ECS::IgnoreTag, EditorOnly>();

        return editor_entity;
    }

} // namespace Editor
// NOLINTEND(misc-use-anonymous-namespace)

int main(int, char* args[])
{
    Log::Log("Current working directory: {}", std::filesystem::current_path().generic_string());

    Editor::asset_registry = ResourceManager::Init<AssetRegistry>();

    Renderer::SetupBackend(args[1]);
    Window::Init(&ImGui::PlatformProcessEvent);
    ShaderCompiler::Init();

    Renderer::Init();


    // Import the default shaders if they haven't been imported yet.
    if (Editor::asset_registry->GetAssetUUID("Assets/Shaders/DefaultShader.shader") == NULL_UUID)
        Editor::asset_registry->Import("Assets/Shaders/DefaultShader.slang");

    if (Editor::asset_registry->GetAssetUUID("Assets/Shaders/PhysicsDebug.shader") == NULL_UUID)
        Editor::asset_registry->Import("Assets/Shaders/PhysicsDebug.slang");

    ResourceRef graphics_pipeline =
        ResourceManager::Load<GraphicsShaderPipeline>(Editor::asset_registry->GetAssetUUID("Assets/Shaders/DefaultShader.shader"));

    const ResourceRef physics_shader =
        ResourceManager::Load<GraphicsShaderPipeline>(Editor::asset_registry->GetAssetUUID("Assets/Shaders/PhysicsDebug.shader"));

    ECS::Init();

    Editor::Init();
    Renderer::render_passes.emplace_back(std::make_shared<DefaultRenderPass>(graphics_pipeline, Renderer::main_target));
    graphics_pipeline.Clear();

    Physics::Init(physics_shader.GetUUID());

    while (!Window::PollEvents())
    {
        Time::Update();

        Physics::Update(Time::GetDeltaTime());

        Editor::Update();
        Renderer::Instance().SwapBuffer();

        ResourceManager::Update();
    }

    ECS::Exit();
    Physics::Exit();

    ResourceManager::Exit();

    ImGui::PlatformExit();
    Renderer::Exit();
    Window::Exit();

    return 0;
}

AssetRegistry::ImportInfo::ImportInfo(std::string path) :
    path{std::move(path)}, watcher{FileWatcher::CreateWatcher(this->path, &ImportedFileChanged)}
{
}

AssetRegistry::ImportInfo::ImportInfo(std::string path, std::vector<UUID> assets) :
    path{std::move(path)}, derived_assets{std::move(assets)}, watcher{FileWatcher::CreateWatcher(this->path, &ImportedFileChanged)}
{
}

AssetRegistry::AssetRegistry()
{
    Files::BinaryReadStream stream{"AssetMapping.bin"};
    while (stream.GetReadPosition() < stream.Size())
    {
        std::string imported_path;
        stream >> imported_path;

        if (!std::filesystem::exists(imported_path))
        {
            Log::Warning("Import path no longer exists: {}", imported_path);
            continue;
        }

        ImportInfo& info = imported_files.emplace_back(std::move(imported_path));

        usize asset_count;
        stream >> asset_count;
        for (usize i = 0; i < asset_count; i++)
        {
            UUID asset;
            stream >> asset;

            std::string asset_path;
            stream >> asset_path;

            if (!std::filesystem::exists(asset_path))
            {
                Log::Warning("Asset path no longer exists: {}", asset_path);
                continue;
            }

            std::string type_id;
            stream >> type_id;

            mapping.emplace(asset, AssetInfo{asset_path, type_id});
            info.derived_assets.push_back(asset);

            Log::Log("Path: {}, UUID: {}", asset_path, asset.str());
        }
    }
}

AssetRegistry::~AssetRegistry()
{
    Files::BinaryWriteStream stream{"AssetMapping.bin"};
    for (const auto& import_info : imported_files)
    {
        stream << import_info.path;

        stream << import_info.derived_assets.size();
        for (const UUID& asset : import_info.derived_assets)
        {
            const auto& [asset_path, type_id] = mapping.at(asset);
            stream << asset;

            stream << asset_path;
            stream << type_id;
        }
    }
}

void AssetRegistry::Import(const std::string& path)
{
    const std::string extension = path.substr(path.find_last_of('.'));
    if (extension.empty())
    {
        Log::Error("Failed to import file, invalid extension: {}", path);
        return;
    }

    const auto& importer_infos = ImporterBase::GetImporterInfos();
    const auto iterator = std::ranges::find_if(importer_infos, [&extension](const ImporterInfo& element) {
        return element.file_types.find(extension) != std::string::npos;
    });

    if (iterator == importer_infos.end())
    {
        Log::Error("Failed to import file, no importer for file extension: {}", extension);
        return;
    }

    ImporterBase* importer = iterator->creator_function();

    std::vector<UUID> imported_assets = importer->ImportAsset(path);
    if (!imported_assets.empty()) RegisterImportedFile(path, std::move(imported_assets));

    delete importer;
}

const UUID& AssetRegistry::GetAssetUUID(const std::string& path) const
{
    for (const auto& [uuid, info] : mapping)
    {
        if (info.path == path) return uuid;
    }

    return NULL_UUID;
}

std::string_view AssetRegistry::GetAssetTypeID(const UUID& uuid) const { return mapping.at(uuid).type_id; }

std::ifstream AssetRegistry::GetAssetTextStream(const UUID& uuid) const { return std::ifstream{mapping.at(uuid).path}; }

Files::BinaryReadStream AssetRegistry::GetAssetDataStream(const UUID& uuid) const { return Files::BinaryReadStream{mapping.at(uuid).path}; }

std::string AssetRegistry::GetAssetText(const UUID& uuid) const { return Files::ReadText(mapping.at(uuid).path); }

std::vector<uint8> AssetRegistry::GetAssetData(const UUID& uuid) const { return Files::ReadBinary(mapping.at(uuid).path); }

void AssetRegistry::RegisterImportedFile(const std::string& path, std::vector<UUID>&& assets)
{
    const auto iterator = std::ranges::lower_bound(imported_files, path, {}, &ImportInfo::path);
    if (iterator != imported_files.end() && iterator->path == path) return; // Early return if the imported file is already registered.

    ImportInfo info{path, std::move(assets)};

    // Do a binary search to find the place for the file watcher, this ensures a sorted list.
    imported_files.insert(iterator, std::move(info));
}

std::vector<UUID> AssetRegistry::UnregisterImportedFile(const std::string& path)
{
    const auto iterator = std::ranges::lower_bound(imported_files, path, {}, &ImportInfo::path);
    if (iterator == imported_files.end()) return {};

    std::vector<UUID> assets = std::move(iterator->derived_assets);
    imported_files.erase(iterator);

    return assets;
}

void AssetRegistry::ImportedFileChanged(const std::string& path, const FileWatcher::Event event)
{
    static std::vector<UUID> previous_assets;

    switch (event)
    {
    case FileWatcher::REMOVED:
        Editor::asset_registry->UnregisterImportedFile(path);
        break;

    case FileWatcher::MODIFIED:
        Editor::asset_registry->Import(path);
        break;

    case FileWatcher::RENAMED_OLD:
        previous_assets = Editor::asset_registry->UnregisterImportedFile(path);
        break;

    case FileWatcher::RENAMED_NEW:
        Editor::asset_registry->RegisterImportedFile(path, std::move(previous_assets));
        break;

    default:
        break;
    }
}