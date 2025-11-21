add_executable(
        Editor
        "Editor/Editor.cpp"
        "Editor/ImGuiExtra.cpp"
        "Editor/ImGuiPlatform.cpp"
        "Editor/ShaderCompiler.cpp"
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
        "${EXTERNAL}/slang/include"
)

# Setup slang by downloading the library, CMake code stolen from: https://github.com/JoseMiguelPizarro/PuduRenderer/blob/master/CMakeLists.txt
set(SLANG_VERSION "2025.14.3")
if (WIN32)
    set(SLANG_OS "windows-x86_64")
elseif (APPLE)
    set(SLANG_OS "macos-aarch64")
elseif (UNIX)
    set(SLANG_OS "linux-x86_64")
endif ()

message(STATUS "Fetching Slang ver: ${SLANG_VERSION}")
include(FetchContent)
FetchContent_Populate(slang_zip URL "https://github.com/shader-slang/slang/releases/download/v${SLANG_VERSION}/slang-${SLANG_VERSION}-${SLANG_OS}.zip" QUIET)

message(STATUS "Slang directory: ${slang_zip_SOURCE_DIR}")
add_library(slang SHARED IMPORTED GLOBAL)
if (WIN32)
    set_target_properties(slang PROPERTIES
            SLANG_DLL "${slang_zip_SOURCE_DIR}/bin/slang.dll"
            GLSLANG_DLL "${slang_zip_SOURCE_DIR}/bin/slang-glslang.dll"
            IMPORTED_IMPLIB "${slang_zip_SOURCE_DIR}/lib/slang.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${slang_zip_SOURCE_DIR}/include"
    )
elseif (APPLE)
    set_target_properties(slang PROPERTIES
            IMPORTED_LOCATION "${slang_zip_SOURCE_DIR}/lib/libslang.dylib"
            IMPORTED_LOCATION "${slang_zip_SOURCE_DIR}/lib/libslang-glslang.dylib"
            INTERFACE_INCLUDE_DIRECTORIES "${slang_zip_SOURCE_DIR}/include"
    )
elseif (UNIX)
    set_target_properties(slang PROPERTIES
            IMPORTED_LOCATION "${slang_zip_SOURCE_DIR}/lib/libslang.so"
            IMPORTED_LOCATION "${slang_zip_SOURCE_DIR}/lib/libslang-glslang.so"
            INTERFACE_INCLUDE_DIRECTORIES "${slang_zip_SOURCE_DIR}/include"
    )
endif ()

get_target_property(SlangDLL slang SLANG_DLL)
get_target_property(GlslangDLL slang GLSLANG_DLL)
set(BuildDirectory "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/")

file(MAKE_DIRECTORY ${BuildDirectory})

configure_file(${SlangDLL} "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/" COPYONLY)
configure_file(${GlslangDLL} "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/" COPYONLY)

target_link_libraries(
    Editor 
    PUBLIC 
    "Core"
    "slang"
)


