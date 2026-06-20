#include "Viewport.hpp"

#include "Editor.hpp"
#include "ImGuiPlatform.hpp"

#include <Math.hpp>
#include <Input.hpp>
#include <Time.hpp>
#include <Transform.hpp>
#include <Rendering/Renderer.hpp>

#include <imgui.h>

namespace
{
    constexpr float PAN_SPEED = 0.0025f;
    constexpr float MOVE_SPEED = 20.0f;

    ECS::Entity camera_entity;

    void CameraInput()
    {
        const bool using_camera = ImGui::IsMouseDown(ImGuiMouseButton_Right);

        // Change mouse lock state.
        if (ImGui::IsMouseLocked() != using_camera) ImGui::LockMouse(using_camera);
        if (!using_camera) return; // Early out if the right mouse button isn't being held down (no need to move the viewport camera).

        if (!ImGui::IsWindowFocused()) ImGui::SetWindowFocus();
        ImGui::SetNextFrameWantCaptureMouse(false);

        const float2 mouse_delta = Input::GetMouseDeltaPos();

        static float pitch = 0.0f;
        static float yaw = 0.0f;
        pitch = Math::Clamp(pitch + mouse_delta.y() * PAN_SPEED, -Math::PI<> / 2.0f, Math::PI<> / 2.0f);
        yaw += mouse_delta.x() * PAN_SPEED;

        auto& camera_transform = camera_entity.GetComponent<Transform>();
        camera_transform.SetRotation(Eigen::AngleAxisf{pitch, Math::RIGHT} * Eigen::AngleAxisf{yaw, Math::UP});

        const Matrix4 camera_matrix = Transform::GetMatrix(camera_entity);

        const float3 forward = Math::TransformVector(Math::FORWARD, camera_matrix);
        const float3 right = Math::TransformVector(Math::RIGHT, camera_matrix);

        const auto forward_move = static_cast<float>(ImGui::IsKeyDown(ImGuiKey_W) - ImGui::IsKeyDown(ImGuiKey_S));
        const auto up_move = static_cast<float>(ImGui::IsKeyDown(ImGuiKey_E) - ImGui::IsKeyDown(ImGuiKey_Q));
        const auto right_move = static_cast<float>(ImGui::IsKeyDown(ImGuiKey_D) - ImGui::IsKeyDown(ImGuiKey_A));
        camera_transform.SetPosition(
            camera_transform.GetPosition() +
            (right_move * right + float3{0.0f, up_move, 0.0f} + forward_move * forward) * MOVE_SPEED * Time::GetDeltaTime()
        );
    }
} // namespace

Viewport::Viewport()
{
    name = "Game viewport";
    window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    default_open = true;

    camera_entity = Editor::CreateEntity("Camera");
    auto&& [transform, camera] = camera_entity.AddComponent<Transform, Camera>();
    transform.SetPosition(float3{0.0f, 0.0f, 7.0f});
}

void Viewport::Display()
{
    if (ImGui::IsWindowHovered()) CameraInput();

    ImVec2 window_content_area = ImGui::GetWindowSize();
    window_content_area.y -= ImGui::GetFrameHeight();

    ImGui::PlatformRescaleGameWindow(window_content_area);
    Renderer::Render(camera_entity);

    ImGui::SetCursorPos(ImVec2{0.0f, ImGui::GetFrameHeight()});
    ImGui::Image(ImGui::GetPlatformTextureID(*Renderer::main_target), window_content_area);
}