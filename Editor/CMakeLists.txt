file(GLOB editor_source "src/*.cpp")
file(GLOB imgui_source "${EXTERNAL}/imgui/*.cpp")

add_executable(
        Editor
        "${editor_source}"
        "${imgui_source}"
        "Implementation/OpenGL/ImGuiPlatform.cpp"
        "Implementation/SDL3GPU/ImGuiPlatform.cpp"
        "${EXTERNAL}/imgui/backends/imgui_impl_sdl3.cpp"
        "${EXTERNAL}/imgui/backends/imgui_impl_opengl3.cpp"
        "${EXTERNAL}/imgui/backends/imgui_impl_sdlgpu3.cpp"
        "${EXTERNAL}/imgui/misc/cpp/imgui_stdlib.cpp"
)

message("${editor_source}")

set_target_properties(Editor PROPERTIES CXX_STANDARD 20)

target_link_libraries(Editor Core)
target_include_directories(Editor PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(Editor PUBLIC "${EXTERNAL}/imgui")
target_include_directories(Editor PUBLIC "${EXTERNAL}/imgui/misc/cpp")
