file(GLOB core_sources "Core/*.cpp")

add_library(Core STATIC "${core_sources}" "Core/Physics/Physics.cpp" "Core/Physics/DebugRenderer.cpp")

set_target_properties(Core PROPERTIES CXX_STANDARD 20)

target_sources(Core PRIVATE "Implementation/PC/Renderer.cpp")
target_sources(Core PRIVATE "Implementation/SDL3/Renderer.cpp")
target_sources(Core PRIVATE "Implementation/OpenGL/Renderer.cpp")
target_sources(Core PRIVATE "${EXTERNAL}/glad/src/glad.c")

target_include_directories(Core PUBLIC "${CMAKE_SOURCE_DIR}/Core")
target_include_directories(Core PUBLIC "${EXTERNAL}/glad/include")
target_include_directories(Core PUBLIC "${EXTERNAL}")
target_include_directories(Core PUBLIC "${EXTERNAL}/SDL/include")
target_include_directories(Core PUBLIC "${EXTERNAL}/assimp/include")
target_include_directories(Core PUBLIC "${EXTERNAL}/eigen")
target_include_directories(Core PUBLIC "${EXTERNAL}/flecs/include")
target_include_directories(Core PUBLIC "${EXTERNAL}/JoltPhysics")

target_link_libraries(Core SDL3::SDL3)
target_link_libraries(Core assimp::assimp)
target_link_libraries(Core flecs::flecs_static)
target_link_libraries(Core Jolt::Jolt)

add_compile_definitions(FLECS_CUSTOM_BUILD)
add_compile_definitions(FLECS_CPP)
add_compile_definitions(JPH_DEBUG_RENDERER)

add_custom_command(TARGET Core POST_BUILD
        COMMAND slangc.exe "${SHADERS}/TestShader.slang" -D SDL3GPU -D VERTEX -matrix-layout-row-major -target spirv -profile spirv_1_3 -o "${SHADERS}/Shader.vert.spv" -entry "VertexMain"
)
add_custom_command(TARGET Core POST_BUILD
        COMMAND slangc.exe "${SHADERS}/TestShader.slang" -D SDL3GPU -D FRAGMENT -matrix-layout-row-major -target spirv -profile spirv_1_3 -o "${SHADERS}/Shader.frag.spv" -entry "FragmentMain"
)

add_custom_command(TARGET Core POST_BUILD
        COMMAND slangc.exe "${SHADERS}/TestShader.slang" -D OPENGL -D VERTEX -matrix-layout-row-major -fvk-invert-y -target glsl -o "${SHADERS}/Shader.vert" -entry "VertexMain"
)
add_custom_command(TARGET Core POST_BUILD
        COMMAND slangc.exe "${SHADERS}/TestShader.slang" -D OPENGL -D FRAGMENT -matrix-layout-row-major -fvk-invert-y -target glsl -o "${SHADERS}/Shader.frag" -entry "FragmentMain"
)