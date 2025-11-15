#include "Renderer.hpp"

#include "Core/Math.hpp"
#include "Core/Model.hpp"
#include "Core/Window.hpp"
#include "Core/Physics/Physics.hpp"
#include "Core/Rendering/RenderPassInterface.hpp"

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <glad/glad.h>

#include <map>
#include <filesystem>
#include <string>

namespace
{
    SDL_GLContext context;

    std::map<int, unsigned int> uniformBuffers;

    void CheckCompileErrors(const uint32 id, const std::string& type)
    {
        int success;
        char infoLog[1024];
        if (type != "program")
        {
            glGetShaderiv(id, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(id, 1024, nullptr, infoLog);
                SDL_Log("Error compiling shader of type: %s", type.c_str());
            }
        }
        else
        {
            glGetProgramiv(id, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(id, 1024, nullptr, infoLog);
                SDL_Log("Error linking program of type: %s", type.c_str());
            }
        }
    }

    template <typename Type>
    void CreateUniformBuffer(const int binding)
    {
        unsigned int& UBO = uniformBuffers[binding];

        glGenBuffers(1, &UBO);

        glBindBuffer(GL_UNIFORM_BUFFER, UBO);
        glBindBufferRange(GL_UNIFORM_BUFFER, binding, UBO, 0, sizeof(Type));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

} // namespace

void OpenGLRenderer::InitBackend()
{
    auto* window = static_cast<SDL_Window*>(Window::GetHandle());
    context = SDL_GL_CreateContext(window);

    if (context == nullptr) SDL_Log("Failed to create GL context: %s", SDL_GetError());

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(&SDL_GL_GetProcAddress))) // NOLINT(clang-diagnostic-cast-function-type-strict)
    {
        SDL_Log("Failed to initialize GLAD");
    }

    if (!SDL_GL_SetSwapInterval(-1))
    {
        // If we fail to set adaptive v-sync we use regular v-sync.
        SDL_GL_SetSwapInterval(1);
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);

    const Handle<Shader> vertex_shader = FileResource::Load<Shader>("Assets/Shaders/Shader.vert", ShaderSettings{Shader::VERTEX, 0, 0, 3});
    const Handle<Shader> fragment_shader = FileResource::Load<Shader>("Assets/Shaders/Shader.frag", ShaderSettings{Shader::FRAGMENT, 1, 0, 0});
    const auto graphics_pipeline = Resource::Load<GraphicsShaderPipeline>(vertex_shader, fragment_shader);

    glUseProgram(graphics_pipeline->shader_pipeline.id);
    CreateUniformBuffer<Matrix4>(0);
    CreateUniformBuffer<Matrix4>(1);
    CreateUniformBuffer<Matrix4>(2);
    glUseProgram(0);
}

void OpenGLRenderer::ExitBackend()
{
    if (!SDL_GL_DestroyContext(context)) SDL_Log("Failed to destroy GL context: %s", SDL_GetError());
}

void OpenGLRenderer::Update() {}

void OpenGLRenderer::SwapBuffer()
{
    auto* window = static_cast<SDL_Window*>(Window::GetHandle());
    SDL_GL_SwapWindow(window);
}

void* OpenGLRenderer::GetContext() { return &context; }

void OpenGLRenderer::RenderMesh(const Mesh& mesh)
{
    glBindVertexArray(mesh.bind);
    glDrawElements(GL_TRIANGLES, static_cast<sint32>(mesh.indices.size()), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void OpenGLRenderer::SetTextureSampler(const uint32 slot, const Texture& texture)
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, texture.texture.id);
}

void OpenGLRenderer::SetUniform(const uint32 slot, const void* data, const size size)
{
    const auto iterator = uniformBuffers.find(static_cast<sint32>(slot));
    if (iterator == uniformBuffers.end())
    {
        SDL_Log("Invalid shader uniform buffer");
        return;
    }

    const unsigned int UBO = iterator->second;

    glBindBuffer(GL_UNIFORM_BUFFER, UBO);
    glBufferData(GL_UNIFORM_BUFFER, static_cast<GLsizeiptr>(size), data, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void OpenGLRenderer::BeginRenderPass(const RenderPassInterface& render_pass)
{
    const Handle<RenderTarget>& render_target = render_pass.render_target;

    glBindFramebuffer(GL_FRAMEBUFFER, render_target->target_id);
    glViewport(0, 0, render_target->GetWidth(), render_target->GetHeight());

    glUseProgram(render_pass.graphics_pipeline->shader_pipeline.id);

    std::vector<uint32> draw_buffers;
    draw_buffers.reserve(render_target->render_buffers.size());

    uint32 index = 0;
    for (RenderBuffer& render_buffer : render_target->render_buffers)
    {
        const uint32 attachment = GL_COLOR_ATTACHMENT0 + index;
        draw_buffers.push_back(attachment);

        index++;

        if (!render_pass.clear_render_targets) continue;

        const float4 color = render_buffer.clear_color;
        glClearColor(color.x(), color.y(), color.z(), color.w());

        glDrawBuffers(1, &attachment);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    // If the target has a valid depth buffer clear the frame buffer's depth bit (you shouldn't set draw buffers to GL_DEPTH_ATTACHMENT for some reason).
    if (render_pass.clear_render_targets && render_target->depth_buffer.GetTexture() != nullptr)
    {
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    glDrawBuffers(static_cast<sint32>(draw_buffers.size()), draw_buffers.data());
}

void OpenGLRenderer::EndRenderPass()
{
    glUseProgram(0);
    glDrawBuffers(0, nullptr);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLRenderer::CreateTexture(Texture& texture, const uint8* data, const SamplerSettings& sampler_settings)
{
    glGenTextures(1, &texture.texture.id);

    const bool is_color_texture = texture.GetFormat() == Texture::COLOR_RGBA_32;
    const sint32 format = is_color_texture ? GL_RGBA : GL_DEPTH_COMPONENT;

    glBindTexture(GL_TEXTURE_2D, texture.texture.id);
    glTexImage2D(GL_TEXTURE_2D, 0, format, texture.GetWidth(), texture.GetHeight(), 0, format, GL_UNSIGNED_BYTE, data);

    if (is_color_texture)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLRenderer::ResizeTexture(Texture& texture, sint32 new_width, sint32 new_height)
{
    const bool is_color_texture = texture.GetFormat() == Texture::COLOR_RGBA_32;
    const sint32 format = is_color_texture ? GL_RGBA : GL_DEPTH_COMPONENT;

    glBindTexture(GL_TEXTURE_2D, texture.texture.id);
    glTexImage2D(GL_TEXTURE_2D, 0, format, new_width, new_height, 0, format, GL_UNSIGNED_BYTE, nullptr);

    if (is_color_texture)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLRenderer::DestroyTexture(Texture& texture) { glDeleteTextures(1, &texture.texture.id); }

void OpenGLRenderer::CreateRenderTarget(RenderTarget& target)
{
    glGenFramebuffers(1, &target.target_id);
}

void OpenGLRenderer::UpdateRenderBuffer(const RenderTarget& target, const size index)
{
    glBindFramebuffer(GL_FRAMEBUFFER, target.target_id);

    const uint32 texture_id = target.render_buffers[index].GetTexture()->texture.id;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + static_cast<uint32>(index), GL_TEXTURE_2D, texture_id, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLRenderer::UpdateDepthBuffer(const RenderTarget& target)
{
    glBindFramebuffer(GL_FRAMEBUFFER, target.target_id);

    const uint32 texture_id = target.depth_buffer.GetTexture()->texture.id;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture_id, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLRenderer::DestroyRenderTarget(RenderTarget& target) { glDeleteFramebuffers(1, &target.target_id); }

void OpenGLRenderer::CreateMesh(Mesh& mesh)
{
    glGenVertexArrays(1, &mesh.bind);
    glGenBuffers(1, &mesh.vertices_buffer.id);
    glGenBuffers(1, &mesh.indices_buffer.id);

    ReloadMesh(mesh);
}

void OpenGLRenderer::DestroyMesh(Mesh& mesh)
{
    glDeleteBuffers(1, &mesh.vertices_buffer.id);
    glDeleteBuffers(1, &mesh.indices_buffer.id);
    glDeleteVertexArrays(1, &mesh.bind);
}

void OpenGLRenderer::ReloadMesh(Mesh& mesh)
{
    glBindVertexArray(mesh.bind);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertices_buffer.id);
    glBufferData(GL_ARRAY_BUFFER, static_cast<sint32>(mesh.vertices.size() * sizeof(Vertex)), mesh.vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indices_buffer.id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<sint32>(mesh.indices.size() * sizeof(uint32)), mesh.indices.data(), GL_STATIC_DRAW);

    uint64 offset = 0;
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(offset));
    glEnableVertexAttribArray(0);
    offset += sizeof(float3);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(offset));
    glEnableVertexAttribArray(1);
    offset += sizeof(float3);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(offset));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void OpenGLRenderer::CreateShader(Shader& shader)
{
    const GLuint shader_type = (shader.type == Shader::VERTEX ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);

    shader.shader.id = glCreateShader(shader_type);
    const char* code = shader.code.data();
    glShaderSource(shader.shader.id, 1, &code, nullptr);
    glCompileShader(shader.shader.id);

    const std::string type_name = (shader.type == Shader::VERTEX ? "vertex" : "fragment");
    CheckCompileErrors(shader.shader.id, type_name);
}
void OpenGLRenderer::DestroyShader(Shader& shader) { glDeleteShader(shader.shader.id); }

// Taken from the LearnOpenGL shader class: https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/shader_m.h
void OpenGLRenderer::CreateShaderPipeline(
    GraphicsShaderPipeline& pipeline, const Handle<Shader>& vertex_shader, const Handle<Shader>& fragment_shader
)
{
    pipeline.shader_pipeline.id = glCreateProgram();

    glAttachShader(pipeline.shader_pipeline.id, vertex_shader->shader.id);
    glAttachShader(pipeline.shader_pipeline.id, fragment_shader->shader.id);
    glLinkProgram(pipeline.shader_pipeline.id);
    CheckCompileErrors(pipeline.shader_pipeline.id, "program");
}

void OpenGLRenderer::DestroyShaderPipeline(GraphicsShaderPipeline& pipeline) { glDeleteProgram(pipeline.shader_pipeline.id); }