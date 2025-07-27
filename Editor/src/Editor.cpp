#include "Core/ECS.hpp"
#include "Editor.hpp"

#include "ImGuiExtra.hpp"
#include "ImGuiPlatform.hpp"

#include <algorithm>
#include <filesystem>
#include <ranges>

#include <Core/Input.hpp>
#include <Core/Renderer.hpp>
#include <Core/Resource.hpp>
#include <Core/Time.hpp>
#include <Core/Window.hpp>
#include <Core/Physics/Physics.hpp>

#include <SDL3/SDL_mouse.h>

#include <imgui.h>

namespace Editor
{
    void InitFonts()
    {
        const ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();
        io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/Verdana.ttf", 23.0f);
    }
} // namespace Editor

ECS::Entity backpack_entity;
ECS::Entity camera_entity;

int main(int, char* args[])
{
    Renderer::SetupBackend(args[1]);

    Window::Init(&ImGui::PlatformProcessEvent);

    Renderer::Instance().Init();
    Renderer::Instance().OnResize(1, 1);

    Physics::Init();
    Editor::Init();
    ECS::Init();

    auto handle = Resource::Load<Mesh>("Assets/Backpack/backpack.obj", 0);
    backpack_entity = ECS::CreateEntity("Backpack");
    backpack_entity.AddComponent<Handle<Mesh>>(handle);
    backpack_entity.AddComponent<Physics::SphereCollider>();

    camera_entity = ECS::CreateEntity("Camera");
    camera_entity.AddComponent<Camera>();
    camera_entity.GetComponent<Transform>().SetPosition(float3{0.0f, 0.0f, 7.0f});
    handle.reset();

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
    Renderer::Instance().Exit();
    Window::Exit();

    return 0;
}

void Editor::Init()
{
    // Setup Dear ImGui context
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
}

void Editor::Update()
{
    ImGui::PlatformNewFrame();

    ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::ShowDemoWindow();

    static bool open_window = true;
    if (ImGui::Begin("Game viewport", &open_window, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) ImGui::SetWindowFocus();

        if (ImGui::IsWindowFocused())
        {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
            {
                if (!ImGui::IsMouseLocked()) ImGui::LockMouse(true);

                const ImVec2 delta = ImGui::GetIO().MouseDelta;

                static float pitch = 0.0f;
                static float yaw = 0.0f;
                pitch = Math::Clamp(pitch + delta.y * 0.01f, -Math::PI<> / 2.0f, Math::PI<> / 2.0f);
                yaw += delta.x * 0.01f;

                auto& camera_transform = camera_entity.GetComponent<Transform>();
                camera_transform.SetRotation(Eigen::AngleAxisf{pitch, Math::RIGHT} * Eigen::AngleAxisf{yaw, Math::UP});

                const Matrix4 camera_matrix = camera_transform.GetMatrix();

                const float3 forward = Math::TransformVector(Math::FORWARD, camera_matrix);
                const float3 right = Math::TransformVector(Math::RIGHT, camera_matrix);

                const auto forward_move = static_cast<float>(ImGui::IsKeyDown(ImGuiKey_W) - ImGui::IsKeyDown(ImGuiKey_S));
                const auto up_move = static_cast<float>(ImGui::IsKeyDown(ImGuiKey_E) - ImGui::IsKeyDown(ImGuiKey_Q));
                const auto right_move = static_cast<float>(ImGui::IsKeyDown(ImGuiKey_D) - ImGui::IsKeyDown(ImGuiKey_A));
                camera_transform.SetPosition(
                    camera_transform.GetPosition() +
                    (right_move * right + float3{0.0f, up_move, 0.0f} + forward_move * forward) * 40.0f * Time::GetDeltaTime()
                );
            }
            else if (ImGui::IsMouseLocked()) ImGui::LockMouse(false);
        }
        ImVec2 window_content_area = ImGui::GetWindowSize();
        window_content_area.y -= ImGui::GetFrameHeight();

        ImGui::RescaleFramebuffer(window_content_area);
        Renderer::Instance().Update();

        ImGui::SetCursorPos(ImVec2{0.0f, ImGui::GetFrameHeight()});
        ImGui::Image(ImGui::GetFramebuffer(), window_content_area);
    }
    ImGui::End();

    if (ImGui::Begin("Hierarchy", nullptr, ImGuiWindowFlags_NoCollapse))
    {
        const auto query = ECS::GetWorld().query_builder<Transform>().build();

        query.each([](const ECS::Entity entity, const Transform&) {
            constexpr ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Bullet;

            ImGui::TreeNodeEx(entity.Name().data(), flags);
        });
    }
    ImGui::End();

    static std::vector<std::string> selected;
    static const std::vector<std::string> items{"item0", "item1", "item2", "item3", "item4", "item5", "item6", "item7", "item8", "item9"};
    if (ImGui::Begin("Item window", nullptr, ImGuiWindowFlags_NoCollapse))
    {
        auto& transform = camera_entity.GetComponent<Transform>();

        ImGui::Text("Delta time: %f", Time::GetDeltaTime());
        ImGui::NewLine();

        ImGui::Text("Camera");
        float3 position = transform.GetPosition();
        if (ImGui::DragFloat3("Translation", position.data(), 0.1f)) transform.SetPosition(position);

        Quat rotation = transform.GetRotation();
        if (ImGui::DragFloat4("Rotation", &rotation.x())) transform.SetRotation(rotation);

        constexpr ImGuiMultiSelectFlags flags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_ClearOnClickVoid |
                                                ImGuiMultiSelectFlags_BoxSelect1d | ImGuiMultiSelectFlags_SelectOnClickRelease;

        ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(flags, static_cast<int>(selected.size()), static_cast<int>(items.size()));
        ImGui::ApplyRequests(ms_io, selected, items);
        for (int i = 0; i < items.size(); i++)
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