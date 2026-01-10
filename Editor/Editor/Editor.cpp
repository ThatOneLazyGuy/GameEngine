#include "Core/ECS.hpp"
#include "Editor.hpp"

#include "ImGuiExtra.hpp"
#include "ImGuiPlatform.hpp"
#include "ShaderCompiler.hpp"

#include "Windows/WindowBase.hpp"

#include <Core/Input.hpp>
#include <Core/Rendering/Renderer.hpp>
#include <Core/Rendering/RenderPassInterface.hpp>
#include <Core/Resource.hpp>
#include <Core/Time.hpp>
#include <Core/Window.hpp>
#include <Core/Physics/Physics.hpp>

#include <SDL3/SDL_mouse.h>

#include <imgui.h>
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

    void CreateDefaultEntities()
    {
        const Handle<Mesh> handle = Resource::Load<Mesh>("Assets/Backpack/backpack.obj", 0);
        backpack_entity = ECS::CreateEntity("Backpack");
        auto&& [handle_component, collider] = backpack_entity.AddComponent<Handle<Mesh>, Physics::SphereCollider>();
        handle_component = handle;
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
    Renderer::SetupBackend(args[1]);
    Window::Init(&ImGui::PlatformProcessEvent);
    ShaderCompiler::Init();

    Renderer::Init();
    const GraphicsPipelineSettings default_pipeline_settings = ShaderCompiler::CompileGraphicsShaders("Assets/Shaders/DefaultShader.slang");
    Handle graphics_pipeline = Resource::Load<GraphicsShaderPipeline>("Assets/Shaders/DefaultShader.slang", default_pipeline_settings);
    const GraphicsPipelineSettings physics_pipeline_settings = ShaderCompiler::CompileGraphicsShaders("Assets/Shaders/PhysicsDebug.slang");
    Resource::Load<GraphicsShaderPipeline>("Assets/Shaders/PhysicsDebug.slang", physics_pipeline_settings);

    ECS::Init();

    Editor::Init();
    Renderer::render_passes.emplace_back(std::make_shared<DefaultRenderPass>(graphics_pipeline, Renderer::main_target));
    graphics_pipeline.reset();

    Physics::Init();

    CreateDefaultEntities();

    while (!Window::PollEvents())
    {
        Time::Update();

        Physics::Update(Time::GetDeltaTime());

        Editor::Update();
        Renderer::Instance().SwapBuffer();
    }

    ECS::Exit();
    Physics::Exit();

    Resource::CleanResources(true);

    ImGui::PlatformExit();
    Renderer::Exit();
    Window::Exit();

    return 0;
}