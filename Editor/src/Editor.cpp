#include "Editor.hpp"

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
    Renderer::SetupBackend("SDL3GPU");

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

template <typename ItemType, typename ContainerType>
void ApplyRequests(ImGuiMultiSelectIO* io, std::vector<ItemType>& selection, const ContainerType& items)
{
    for (const auto& request : io->Requests)
    {
        switch (request.Type)
        {
        case ImGuiSelectionRequestType_None:
            break;

        case ImGuiSelectionRequestType_SetAll: // Request app to clear selection (if Selected==false) or select all items (if Selected==true). We cannot set RangeFirstItem/RangeLastItem as its contents is entirely up to user (not necessarily an index)
            selection.clear();
            if (request.Selected) selection.insert(selection.begin(), std::begin(items), std::end(items));
            break;

        case ImGuiSelectionRequestType_SetRange: {
            if (request.Selected)
            {
                for (ImGuiSelectionUserData i = request.RangeFirstItem; i <= request.RangeLastItem; i++)
                {
                    selection.push_back(items.at(i));
                }
            }
            else
            {
                for (ImGuiSelectionUserData i = request.RangeFirstItem; i <= request.RangeLastItem; i++)
                {
                    selection.erase(std::ranges::find(selection, items.at(i)));
                }
            }
        }
        break;
        }
    }
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
    ImGui::PlatformInit("SDL3GPU");
}

void Editor::Update()
{
    ImGui::PlatformNewFrame();

    ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);
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
        ImGui::DragFloat2("test", Renderer::size_test, 0.1f);

        constexpr ImGuiMultiSelectFlags flags =
            ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_ClearOnClickVoid |
            ImGuiMultiSelectFlags_BoxSelect1d | ImGuiMultiSelectFlags_SelectOnClickRelease;

        ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(flags, static_cast<int>(selected.size()), items.size());
        ApplyRequests(ms_io, selected, items);
        for (int i = 0; i < items.size(); i++)
        {
            ImGui::PushID(i);
            const bool item_is_selected = std::ranges::find(selected, items[i]) != selected.end();
            ImGui::SetNextItemSelectionUserData(i);
            ImGui::Selectable(items[i].c_str(), item_is_selected);
            ImGui::PopID();
        }
        ms_io = ImGui::EndMultiSelect();
        ApplyRequests(ms_io, selected, items);
    }
    ImGui::End();

    ImGui::PlatformEndFrame();
}