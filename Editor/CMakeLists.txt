add_executable(
        Editor
        "src/Editor.cpp"
        "src/ImGuiExtra.cpp"
        "src/ImGuiPlatform.cpp"
        "Implementation/OpenGL/ImGuiPlatform.cpp"
        "Implementation/SDL3GPU/ImGuiPlatform.cpp"

        "${EXTERNAL}/imgui/imgui.cpp"
        "${EXTERNAL}/imgui/imgui_demo.cpp"
        "${EXTERNAL}/imgui/imgui_draw.cpp"
        "${EXTERNAL}/imgui/imgui_tables.cpp"
        "${EXTERNAL}/imgui/imgui_widgets.cpp"
        "${EXTERNAL}/imgui/backends/imgui_impl_sdl3.cpp"
        "${EXTERNAL}/imgui/backends/imgui_impl_opengl3.cpp"
        "${EXTERNAL}/imgui/backends/imgui_impl_sdlgpu3.cpp"
        "${EXTERNAL}/imgui/misc/cpp/imgui_stdlib.cpp"
)

set_target_properties(Editor PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

target_include_directories(
        Editor
        PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${EXTERNAL}/imgui"
        "${EXTERNAL}/imgui/misc/cpp"
)

target_link_libraries(Editor "Core")


