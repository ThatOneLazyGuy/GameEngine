#include "Core/Renderer.hpp"

#include "Implementation/OpenGL/Renderer.hpp"
#include "Implementation/SDL3/Renderer.hpp"

void Renderer::SetupBackend(const std::string& backend_name)
{
    if (backend_name == "OpenGL")
    {
        renderer = new OpenGLRenderer;
        return;
    }

    renderer = new SDL3GPURenderer;
}
