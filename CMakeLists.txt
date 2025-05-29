cmake_minimum_required(VERSION 3.10.0)
project(Engine VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Project definitions
set(EXTERNAL "${CMAKE_SOURCE_DIR}/External")
set(ASSETS "${CMAKE_SOURCE_DIR}/Assets")
set(SHADERS "${ASSETS}/Shaders")
set(PLATFORM "PC")
set(RENDER_BACKEND "SDL3GPU")

add_compile_definitions(RENDER_BACKEND_${RENDER_BACKEND})
add_compile_definitions($<$<CONFIG:Debug>:DEBUG>)
add_compile_definitions(GLM_FORCE_LEFT_HANDED)

add_subdirectory("${EXTERNAL}/SDL" EXCLUDE_FROM_ALL)

option(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT "" OFF)
option(ASSIMP_BUILD_TESTS "" OFF)
option(ASSIMP_INSTALL "" OFF)
add_subdirectory("${EXTERNAL}/assimp" EXCLUDE_FROM_ALL)

add_subdirectory(Core)
add_subdirectory(Editor)