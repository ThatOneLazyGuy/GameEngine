#include "Editor.hpp"

#include "ImGuiExtra.hpp"

#include <algorithm>
#include <ranges>

#include <Core/Renderer.hpp>
#include <Core/Window.hpp>

#include "ImGuiPlatform.hpp"

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

int main(int, char*[])
{
    //Renderer::SetupBackend("SDL3GPU");
    Renderer::SetupBackend("OpenGL");

    Window::Init(&ImGui::PlatformProcessEvent);
    Renderer::Instance().Init(Window::GetHandle());
    Editor::Init();

    bool done = false;
    while (!done)
    {
        done = Window::PollEvents();
        Editor::Update();
        Renderer::Instance().SwapBuffer();
    }

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
    //ImGui::PlatformInit("SDL3GPU");
    ImGui::PlatformInit("OpenGL");
}

void Editor::Update()
{
    ImGui::PlatformNewFrame();

    ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::ShowStyleEditor();
    ImGui::ShowDemoWindow();

    static bool open_window = true;
    if (ImGui::Begin(
            "Hello window", &open_window, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse

        ))
    {
        ImVec2 window_content_area = ImGui::GetWindowSize();
        window_content_area.y -= ImGui::GetFrameHeight();

        ImGui::RescaleFramebuffer(window_content_area);
        Renderer::Instance().Update();

        ImGui::SetCursorPos(ImVec2{0.0f, ImGui::GetFrameHeight()});
        ImGui::Image(ImGui::GetFramebuffer(), window_content_area);
    }
    ImGui::End();

    static std::vector<std::string> selected;
    static const std::vector<std::string> items{
        "item0", "item1", "item2", "item3", "item4", "item5", "item6", "item7", "item8", "item9"
    };
    if (ImGui::Begin("Item window", nullptr, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::DragFloat3("test", Renderer::position, 0.1f);
        ImGui::DragFloat("test", &Renderer::fov, 0.1f);

        constexpr ImGuiMultiSelectFlags flags =
            ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_ClearOnClickVoid |
            ImGuiMultiSelectFlags_BoxSelect1d | ImGuiMultiSelectFlags_SelectOnClickRelease;

        ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(flags, static_cast<int>(selected.size()), items.size());
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