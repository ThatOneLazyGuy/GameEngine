cmake_minimum_required(VERSION 3.10.0)
project(Engine VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>")

set(EXTERNAL "${CMAKE_SOURCE_DIR}/External")
set(ASSETS "${CMAKE_SOURCE_DIR}/Assets")
set(SHADERS "${ASSETS}/Shaders")
set(PLATFORM "PC")
set(RENDER_BACKEND "SDL3GPU")

add_subdirectory("${EXTERNAL}/SDL" EXCLUDE_FROM_ALL)
add_subdirectory("${EXTERNAL}/assimp" EXCLUDE_FROM_ALL)
add_subdirectory("${EXTERNAL}/flecs" EXCLUDE_FROM_ALL)
add_subdirectory("${EXTERNAL}/JoltPhysics/Build" EXCLUDE_FROM_ALL)

add_compile_definitions(
        "RENDER_BACKEND_${RENDER_BACKEND}"
        "$<$<CONFIG:Debug>:DEBUG>"
)

add_subdirectory(Core)
add_subdirectory(Editor)

