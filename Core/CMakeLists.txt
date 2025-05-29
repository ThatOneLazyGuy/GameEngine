file(GLOB core_sources "Core/*.cpp")

add_library(Core STATIC "${core_sources}"
        Core/Model.cpp
        Core/Model.hpp)

target_sources(Core PRIVATE "Implementation/PC/Renderer.cpp")
target_sources(Core PRIVATE "Implementation/SDL3/Renderer.cpp")
target_sources(Core PRIVATE "Implementation/OpenGL/Renderer.cpp")
target_sources(Core PRIVATE "${EXTERNAL}/glad/src/glad.c")

target_include_directories(Core PUBLIC "${EXTERNAL}/glad/include")
target_include_directories(Core PUBLIC "${EXTERNAL}")
target_include_directories(Core PUBLIC "${EXTERNAL}/SDL/include")
target_include_directories(Core PUBLIC "${EXTERNAL}/assimp/include")
target_include_directories(Core PUBLIC "${EXTERNAL}/glm")
target_include_directories(Core PUBLIC "${CMAKE_SOURCE_DIR}/Core")

set_target_properties(Core PROPERTIES CXX_STANDARD 23)
target_link_libraries(Core SDL3::SDL3)
target_link_libraries(Core assimp::assimp)

add_custom_command(TARGET Core POST_BUILD
        COMMAND slangc.exe "${SHADERS}/TestShader.slang" -matrix-layout-row-major -capability SPIRV_1_3 -target spirv -o "${SHADERS}/Shader.vert.spv" -entry "VertexMain"
)
add_custom_command(TARGET Core POST_BUILD
        COMMAND slangc.exe "${SHADERS}/TestShader.slang" -matrix-layout-row-major -capability SPIRV_1_3 -target spirv -o "${SHADERS}/Shader.frag.spv" -entry "FragmentMain"
)

add_custom_command(TARGET Core POST_BUILD
        COMMAND slangc.exe "${SHADERS}/TestShader.slang" -matrix-layout-row-major -target glsl -o "${SHADERS}/Shader.vert" -entry "VertexMain"
)
add_custom_command(TARGET Core POST_BUILD
        COMMAND slangc.exe "${SHADERS}/TestShader.slang" -matrix-layout-row-major -target glsl -o "${SHADERS}/Shader.frag" -entry "FragmentMain"
)