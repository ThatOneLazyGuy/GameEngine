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

add_custom_command(TARGET Core POST_BUILD
        COMMAND slangc.exe "${SHADERS}/TestShader.slang" -D SDL3GPU -D VERTEX -matrix-layout-row-major -target spirv -profile spirv_1_3 -o "${SHADERS}/Shader.vert.spv" -entry "VertexMain"
        COMMAND slangc.exe "${SHADERS}/TestShader.slang" -D SDL3GPU -D FRAGMENT -matrix-layout-row-major -target spirv -profile spirv_1_3 -o "${SHADERS}/Shader.frag.spv" -entry "FragmentMain"
        COMMAND slangc.exe "${SHADERS}/TestShader.slang" -D OPENGL -D VERTEX -matrix-layout-row-major -fvk-invert-y -target glsl -o "${SHADERS}/Shader.vert.glsl" -entry "VertexMain"
        COMMAND slangc.exe "${SHADERS}/TestShader.slang" -D OPENGL -D FRAGMENT -matrix-layout-row-major -fvk-invert-y -target glsl -o "${SHADERS}/Shader.frag.glsl" -entry "FragmentMain"
        # COMMAND slangc.exe "${SHADERS}/TestShader.slang" -D DX12 -D VERTEX -matrix-layout-row-major -target dxil -profile vs_6_0 -o "${SHADERS}/Shader.vert.dxil" -entry "VertexMain"
        # COMMAND slangc.exe "${SHADERS}/TestShader.slang" -D DX12 -D FRAGMENT -matrix-layout-row-major -target dxil -profile ps_6_0 -o "${SHADERS}/Shader.frag.dxil" -entry "FragmentMain"
)

add_custom_command(TARGET Core POST_BUILD
        COMMAND slangc.exe "${SHADERS}/PhysicsDebug.slang" -D SDL3GPU -D VERTEX -matrix-layout-row-major -target spirv -profile spirv_1_3 -o "${SHADERS}/PhysicsDebug.vert.spv" -entry "VertexMain"
        COMMAND slangc.exe "${SHADERS}/PhysicsDebug.slang" -D SDL3GPU -D FRAGMENT -matrix-layout-row-major -target spirv -profile spirv_1_3 -o "${SHADERS}/PhysicsDebug.frag.spv" -entry "FragmentMain"
        COMMAND slangc.exe "${SHADERS}/PhysicsDebug.slang" -D OPENGL -D VERTEX -matrix-layout-row-major -fvk-invert-y -target glsl -o "${SHADERS}/PhysicsDebug.vert.glsl" -entry "VertexMain"
        COMMAND slangc.exe "${SHADERS}/PhysicsDebug.slang" -D OPENGL -D FRAGMENT -matrix-layout-row-major -fvk-invert-y -target glsl -o "${SHADERS}/PhysicsDebug.frag.glsl" -entry "FragmentMain"
        # COMMAND slangc.exe "${SHADERS}/PhysicsDebug.slang" -D DX12 -D VERTEX -matrix-layout-row-major -target dxil -profile vs_6_0 -o "${SHADERS}/PhysicsDebug.vert.dxil" -entry "VertexMain"
        # COMMAND slangc.exe "${SHADERS}/PhysicsDebug.slang" -D DX12 -D FRAGMENT -matrix-layout-row-major -target dxil -profile ps_6_0 -o "${SHADERS}/PhysicsDebug.frag.dxil" -entry "FragmentMain"
)