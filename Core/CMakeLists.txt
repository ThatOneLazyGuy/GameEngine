add_library(
        Core
        STATIC
        "Core/Physics/Physics.cpp"
        "Core/Physics/DebugRenderer.cpp"
        "Core/ECS.cpp"
        "Core/Input.cpp"
        "Core/Model.cpp"
        "Core/Time.cpp"
        "Core/Window.cpp"

        "Core/Rendering/Renderer.cpp"
        "Core/Rendering/RenderPassInterface.cpp"
        "Platform/PC/SDL3GPU/Rendering/Renderer.cpp"
        "Platform/OpenGL/Rendering/Renderer.cpp"
        "${EXTERNAL}/glad/src/glad.c"
)

set_target_properties(Core PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

target_include_directories(
        Core
        PUBLIC
        "${CMAKE_SOURCE_DIR}/Core"
        "${EXTERNAL}/glad/include"
        "${EXTERNAL}"
        "${EXTERNAL}/SDL/include"
        "${EXTERNAL}/assimp/include"
        "${EXTERNAL}/eigen"
        "${EXTERNAL}/flecs/include"
        "${EXTERNAL}/JoltPhysics"
)

target_link_libraries(
        Core
        "SDL3::SDL3"
        "assimp::assimp"
        "flecs::flecs"
        "Jolt::Jolt"
)

target_compile_definitions(Core
        PUBLIC
        "FLECS_CUSTOM_BUILD"
        "FLECS_CPP"
        "JPH_DEBUG_RENDERER"
        "EIGEN_MAX_ALIGN_BYTES=0"
)