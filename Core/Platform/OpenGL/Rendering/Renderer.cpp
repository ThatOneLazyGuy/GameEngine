#include "Renderer.hpp"

#include "Tools/Logging.hpp"
#include "Core/Math.hpp"
#include "Core/Model.hpp"
#include "Core/Window.hpp"
#include "Core/Physics/Physics.hpp"
#include "Core/Rendering/RenderPassInterface.hpp"

#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <glad/glad.h>

#include <map>
#include <filesystem>
#include <string>

namespace
{
    SDL_GLContext context;

    std::map<GLuint, GLuint> uniform_buffers;
    Handle<GraphicsShaderPipeline> active_pipeline;

    void CheckCompileErrors(const uint32 id, const std::string& type = "")
    {
        GLint success;
        char info_log[1024];
        if (type.empty())
        {
            glGetProgramiv(id, GL_LINK_STATUS, &success);
            if (success == GL_FALSE)
            {
                glGetProgramInfoLog(id, 1024, nullptr, info_log);
                Log::Error("Error linking program: {}", info_log);
            }

            return;
        }

        glGetShaderiv(id, GL_COMPILE_STATUS, &success);
        if (success == GL_FALSE)
        {
            glGetShaderInfoLog(id, 1024, nullptr, info_log);
            Log::Error("Error compiling shader of type: {}, {}", type, info_log);
        }
    }

    void CreateUniformBuffer(const GLuint binding, const usize size)
    {
        GLuint& UBO = uniform_buffers[binding];

        glGenBuffers(1, &UBO);

        glBindBuffer(GL_UNIFORM_BUFFER, UBO);
        glBindBufferRange(GL_UNIFORM_BUFFER, binding, UBO, 0, static_cast<GLsizeiptr>(size));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void SetActiveVertexAttributes()
    {
        GLuint location = 0;
        for (const VertexAttribute& attribute : active_pipeline->vertex_attributes)
        {
            GLenum type{};
            switch (attribute.element_type)
            {
            case VertexAttribute::FLOAT:
                type = GL_FLOAT;
                break;

            case VertexAttribute::SINT:
                type = GL_INT;
                break;

            case VertexAttribute::UINT:
                type = GL_UNSIGNED_INT;
                break;

            default:
                Log::Error("Invalid element type");
                break;
            }


            const usize offset = attribute_sizes.at(attribute.semantic_name);
            glVertexAttribPointer(
                location, static_cast<GLint>(attribute.element_count), type, GL_FALSE, sizeof(Vertex), std::bit_cast<void*>(offset)
            );
            glEnableVertexAttribArray(location);
            ++location;
        }
    }

} // namespace

OpenGLRenderer::OpenGLRenderer() : Renderer{}
{
    backend_shader_info = {.file_extension = ".glsl", .binary = false, .profile = "glsl_150", .invert_y = true};
}

void OpenGLRenderer::InitBackend()
{
    auto* window = static_cast<SDL_Window*>(Window::GetHandle());
    context = SDL_GL_CreateContext(window);

    if (context == nullptr) Log::Error("Failed to create GL context: %s", SDL_GetError());

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(&SDL_GL_GetProcAddress))) // NOLINT(clang-diagnostic-cast-function-type-strict)
    {
        Log::Error("Failed to initialize GLAD");
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
}

void OpenGLRenderer::ExitBackend()
{
    if (!SDL_GL_DestroyContext(context)) Log::Error("Failed to destroy GL context: %s", SDL_GetError());
}

void OpenGLRenderer::SwapBuffer()
{
    auto* window = static_cast<SDL_Window*>(Window::GetHandle());
    SDL_GL_SwapWindow(window);
}

void* OpenGLRenderer::GetContext() { return &context; }

void OpenGLRenderer::RenderMesh(const Mesh& mesh)
{
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertices_buffer.id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indices_buffer.id);

    // Explicitly set the vertex attributes for each render call instead of using a VAO, this allows us to use different parameter layouts in shaders.
    SetActiveVertexAttributes();
    glDrawElements(GL_TRIANGLES, static_cast<sint32>(mesh.GetIndicesCount()), GL_UNSIGNED_INT, nullptr);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void OpenGLRenderer::SetTextureSampler(const uint32 slot, const Texture& texture)
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, texture.texture.id);
}

void OpenGLRenderer::SetUniform(const uint32 slot, const void* data, const usize size)
{
    const auto iterator = uniform_buffers.find(static_cast<sint32>(slot));
    if (iterator == uniform_buffers.end())
    {
        Log::Error("Invalid shader uniform buffer");
        return;
    }

    const GLuint UBO = iterator->second;

    glBindBuffer(GL_UNIFORM_BUFFER, UBO);
    glBufferData(GL_UNIFORM_BUFFER, static_cast<GLsizeiptr>(size), data, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void OpenGLRenderer::BeginRenderPass(const RenderPassInterface& render_pass)
{
    const Handle<RenderTarget>& render_target = render_pass.render_target;

    glBindFramebuffer(GL_FRAMEBUFFER, render_target->target_id);
    glViewport(0, 0, render_target->GetWidth(), render_target->GetHeight());

    active_pipeline = render_pass.graphics_pipeline;
    glUseProgram(active_pipeline->shader_pipeline.id);

    std::vector<uint32> draw_buffers;
    draw_buffers.reserve(render_target->render_buffers.size());

    uint32 index = 0;
    for (const RenderBuffer& render_buffer : render_target->render_buffers)
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
    if (render_pass.clear_render_targets && render_target->depth_buffer.GetTexture() != nullptr) { glClear(GL_DEPTH_BUFFER_BIT); }

    glDrawBuffers(static_cast<sint32>(draw_buffers.size()), draw_buffers.data());
}

void OpenGLRenderer::EndRenderPass()
{
    active_pipeline.reset();

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

void OpenGLRenderer::CreateRenderTarget(RenderTarget& target) { glGenFramebuffers(1, &target.target_id); }

void OpenGLRenderer::UpdateRenderBuffer(const RenderTarget& target, const usize index)
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

void OpenGLRenderer::CreateMesh(Mesh& mesh, const std::vector<Vertex>& vertices, const std::vector<uint32>& indices)
{
    glGenBuffers(1, &mesh.vertices_buffer.id);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertices_buffer.id);
    glBufferData(GL_ARRAY_BUFFER, static_cast<sint32>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &mesh.indices_buffer.id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indices_buffer.id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<sint32>(indices.size() * sizeof(uint32)), indices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void OpenGLRenderer::DestroyMesh(Mesh& mesh)
{
    glDeleteBuffers(1, &mesh.vertices_buffer.id);
    glDeleteBuffers(1, &mesh.indices_buffer.id);
}

void OpenGLRenderer::CreateShader(Shader& shader, const void* data, usize)
{
    const GLuint shader_type = (shader.type == Shader::VERTEX ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);

    shader.shader.id = glCreateShader(shader_type);
    const char* code = static_cast<const char*>(data);
    glShaderSource(shader.shader.id, 1, &code, nullptr);
    glCompileShader(shader.shader.id);

    const std::string type_name = (shader.type == Shader::VERTEX ? "vertex" : "fragment");
    CheckCompileErrors(shader.shader.id, type_name);
}
void OpenGLRenderer::DestroyShader(Shader& shader) { glDeleteShader(shader.shader.id); }

void OpenGLRenderer::CreateShaderPipeline(
    GraphicsShaderPipeline& pipeline, const Handle<Shader>& vertex_shader, const Handle<Shader>& fragment_shader
)
{
    pipeline.shader_pipeline.id = glCreateProgram();

    glAttachShader(pipeline.shader_pipeline.id, vertex_shader->shader.id);
    glAttachShader(pipeline.shader_pipeline.id, fragment_shader->shader.id);
    glLinkProgram(pipeline.shader_pipeline.id);

    CheckCompileErrors(pipeline.shader_pipeline.id);

    glUseProgram(pipeline.shader_pipeline.id);
    GLuint i = 0;
    for (const auto& [name, size] : pipeline.uniform_sizes)
    {
        CreateUniformBuffer(i, size);
        ++i;
    }
    glUseProgram(0);
}

void OpenGLRenderer::DestroyShaderPipeline(GraphicsShaderPipeline& pipeline) { glDeleteProgram(pipeline.shader_pipeline.id); }