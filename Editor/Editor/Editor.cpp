#include "Editor.hpp"


#include "ImGuiExtra.hpp"
#include "ImGuiPlatform.hpp"
#include "ShaderCompiler.hpp"

#include "Windows/WindowBase.hpp"
#include "Importers/ImporterBase.hpp"

#include <Core/ECS.hpp>
#include <Core/Time.hpp>
#include <Core/Window.hpp>
#include <Core/Physics/Physics.hpp>
#include <Core/ResourceManager.hpp>
#include <Core/Rendering/Renderer.hpp>
#include <Core/Rendering/RenderPassInterface.hpp>
#include <Tools/Files.hpp>

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

    ECS::Entity backpack_entity;

    //void CreateDefaultEntities()
    //{
    //    const ResourceRef<Mesh> handle = ResourceManager::Load<Mesh>("Assets/Backpack/backpack.obj", 0);
    //    backpack_entity = ECS::CreateEntity("Backpack");
    //    auto&& [handle_component, collider] = backpack_entity.AddComponent<ResourceRef<Mesh>, Physics::SphereCollider>();
    //    handle_component = handle;
    //}

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
    Editor::asset_registry = ResourceManager::Init<AssetRegistry>();

    Renderer::SetupBackend(args[1]);
    Window::Init(&ImGui::PlatformProcessEvent);
    ShaderCompiler::Init();

    Renderer::Init();

    Log::Log(std::filesystem::current_path().generic_string());

    const GraphicsPipelineSettings default_pipeline_settings = ShaderCompiler::CompileGraphicsShaders("Assets/Shaders/DefaultShader.slang");
    ResourceRef graphics_pipeline =
        ResourceManager::Create<GraphicsShaderPipeline>("Assets/Shaders/DefaultShader.slang", default_pipeline_settings);

    const GraphicsPipelineSettings physics_pipeline_settings = ShaderCompiler::CompileGraphicsShaders("Assets/Shaders/PhysicsDebug.slang");
    const ResourceRef physics_shader =
        ResourceManager::Create<GraphicsShaderPipeline>("Assets/Shaders/PhysicsDebug.slang", physics_pipeline_settings);

    ECS::Init();

    Editor::Init();
    Renderer::render_passes.emplace_back(std::make_shared<DefaultRenderPass>(graphics_pipeline, Renderer::main_target));
    graphics_pipeline.Clear();

    Physics::Init(physics_shader.GetUUID());

    //CreateDefaultEntities();

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

AssetRegistry::AssetRegistry()
{
    usize read_index = 0;
    const std::vector<uint8> data = Files::ReadBinary("ResourceMapping.bin");
    while (read_index < data.size())
    {
        const usize uuid_size = *reinterpret_cast<const usize*>(data.data() + read_index);
        read_index += sizeof(usize);

        std::span uuid_bytes{data.data() + read_index, data.data() + read_index + uuid_size};
        read_index += uuid_size;

        const usize path_size = *reinterpret_cast<const usize*>(data.data() + read_index);
        read_index += sizeof(usize);

        std::string path{data.data() + read_index, data.data() + read_index + path_size};
        read_index += path_size;

        UUID uuid{uuid_bytes.data()};

        Log::Log("Path: {}, UUID: {}", path, uuid.str());

        if (!std::filesystem::exists(path))
        {
            Log::Warning("Asset path no longer exists: {}", path);
            continue;
        }

        const auto& iterator = mapping.emplace(uuid, path);
        reverse_mapping.emplace(iterator.first->second, uuid);
    }
}

AssetRegistry::~AssetRegistry()
{
    usize write_index = 0;
    std::vector<uint8> data;
    for (const auto& [uuid, path] : mapping)
    {
        const std::string uuid_bytes = uuid.bytes();
        const usize uuid_size = uuid_bytes.size();
        const usize uuid_data_size = uuid_size + sizeof(usize);

        const usize path_size = path.size();
        const usize path_data_size = path_size + sizeof(usize);

        data.resize(data.size() + uuid_data_size + path_data_size);

        std::memcpy(data.data() + write_index, &uuid_size, sizeof(usize));
        write_index += sizeof(usize);

        std::memcpy(data.data() + write_index, uuid_bytes.data(), uuid_size);
        write_index += uuid_size;

        std::memcpy(data.data() + write_index, &path_size, sizeof(usize));
        write_index += sizeof(usize);

        std::memcpy(data.data() + write_index, path.data(), path_size);
        write_index += path_size;
    }

    Files::WriteBinary("ResourceMapping.bin", data);

    mapping.clear();
}

usize AssetRegistry::GetAssetMetadata(const UUID& uuid) const
{
    const std::vector<uint8> metadata = Files::ReadBinary(mapping.at(uuid) + ".meta");
    return *reinterpret_cast<const usize*>(metadata.data());
}

std::string AssetRegistry::GetAssetText(const UUID& uuid) const { return Files::ReadText(mapping.at(uuid)); }

std::vector<uint8> AssetRegistry::GetAssetData(const UUID& uuid) const { return Files::ReadBinary(mapping.at(uuid)); }