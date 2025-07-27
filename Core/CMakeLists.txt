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

        "Implementation/PC/Renderer.cpp"
        "Implementation/SDL3/Renderer.cpp"
        "Implementation/OpenGL/Renderer.cpp"
        "${EXTERNAL}/glad/src/glad.c"
)

set_target_properties(Core PROPERTIES CXX_STANDARD 20)

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
        "flecs::flecs_static"
        "Jolt::Jolt"
)

target_compile_definitions(Core
        PUBLIC
        "FLECS_CUSTOM_BUILD"
        "FLECS_CPP"
        "JPH_DEBUG_RENDERER"
)

add_custom_command(TARGET Core POST_BUILD
        COMMAND slangc.exe "${SHADERS}/TestShader.slang" -D SDL3GPU -D VERTEX -matrix-layout-row-major -target spirv -profile spirv_1_3 -o "${SHADERS}/Shader.vert.spv" -entry "VertexMain"
        COMMAND slangc.exe "${SHADERS}/TestShader.slang" -D SDL3GPU -D FRAGMENT -matrix-layout-row-major -target spirv -profile spirv_1_3 -o "${SHADERS}/Shader.frag.spv" -entry "FragmentMain"
        COMMAND slangc.exe "${SHADERS}/TestShader.slang" -D OPENGL -D VERTEX -matrix-layout-row-major -fvk-invert-y -target glsl -o "${SHADERS}/Shader.vert" -entry "VertexMain"
        COMMAND slangc.exe "${SHADERS}/TestShader.slang" -D OPENGL -D FRAGMENT -matrix-layout-row-major -fvk-invert-y -target glsl -o "${SHADERS}/Shader.frag" -entry "FragmentMain"
)