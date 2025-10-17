#include "Renderer.hpp"

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>

#include "Core/ECS.hpp"
#include "Core/Math.hpp"
#include "Core/Model.hpp"
#include "Core/Window.hpp"
#include "Core/Physics/DebugRenderer.hpp"
#include "Core/Physics/Physics.hpp"

#include <filesystem>
#include <fstream>
#include <glad/glad.h>
#include <map>
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

    // utility uniform functions
    template <typename Type>
    void SetUniform(const int binding, const Type& value)
    {
        const auto iterator = uniformBuffers.find(binding);
        if (iterator == uniformBuffers.end())
        {
            SDL_Log("Invalid shader uniform buffer");
            return;
        }

        const unsigned int UBO = iterator->second;

        glBindBuffer(GL_UNIFORM_BUFFER, UBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(Type), &value, GL_STATIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    template <typename>
    void SetUniform(const int binding, const Matrix4& value)
    {
        const auto iterator = uniformBuffers.find(binding);
        if (iterator == uniformBuffers.end())
        {
            SDL_Log("Invalid shader uniform buffer");
            return;
        }

        const unsigned int UBO = iterator->second;

        glBindBuffer(GL_UNIFORM_BUFFER, UBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(Matrix4), value.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    template <size TextureType = GL_TEXTURE_2D>
    void SetTexture(const unsigned int texture, const int binding)
    {
        glActiveTexture(GL_TEXTURE0 + binding);
        glBindTexture(TextureType, texture);
    }

    void RenderMesh(const Transform& transform, const Handle<Mesh>& mesh_handle)
    {
        const auto model = transform.GetMatrix();
        SetUniform<Matrix4>(0, model);

        glUseProgram(Resource::GetResources<ShaderPipeline>()[0]->shader_pipeline.opengl);

        uint32 diffuse_count = 0;
        uint32 specular_count = 0;
        for (const auto& texture : mesh_handle->textures)
        {
            switch (texture->type)
            {
            case Texture::Type::DIFFUSE:
                glActiveTexture(GL_TEXTURE0 + diffuse_count++);
                break;

            case Texture::Type::SPECULAR:
                glActiveTexture(GL_TEXTURE3 + specular_count++);
                break;
            }

            glBindTexture(GL_TEXTURE_2D, texture->texture.opengl);
        }

        glBindVertexArray(mesh_handle->bind);
        glDrawElements(GL_TRIANGLES, static_cast<sint32>(mesh_handle->indices.size()), GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }

    void RenderCamera(const Transform& camera_transform, const Camera& camera)
    {
        const Handle<RenderTarget>& render_target = camera.render_target;

        glBindFramebuffer(GL_FRAMEBUFFER, render_target->sampler.opengl);
        constexpr unsigned int buffers[2] = {GL_COLOR_ATTACHMENT0, GL_DEPTH_STENCIL_ATTACHMENT};
        glDrawBuffers(2, buffers);

        glViewport(0, 0, render_target->GetWidth(), render_target->GetHeight());

        if (render_target->clear)
        {
            const float4 color = render_target->clear_color;
            glClearColor(color.x(), color.y(), color.z(), color.w());
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        const Matrix4 view = Math::Inverse(camera_transform.GetMatrix());
        SetUniform<Matrix4>(1, view);

        const Matrix4 projection = camera.GetProjection(*render_target);
        SetUniform<Matrix4>(2, projection);
        
        const auto mesh_query = ECS::GetWorld().query_builder<const Transform, const Handle<Mesh>>().build();
        mesh_query.each(&RenderMesh);

        for (const auto& [model, mesh] : Physics::RenderDebug(camera_transform.GetPosition()))
        {
            SetUniform<Matrix4>(0, model);

            uint32 diffuse_count = 0;
            uint32 specular_count = 0;
            for (const auto& texture : mesh->textures)
            {
                switch (texture->type)
                {
                case Texture::Type::DIFFUSE:
                    glActiveTexture(GL_TEXTURE0 + diffuse_count++);
                    break;

                case Texture::Type::SPECULAR:
                    glActiveTexture(GL_TEXTURE3 + specular_count++);
                    break;
                }

                glBindTexture(GL_TEXTURE_2D, texture->texture.opengl);
            }

            glBindVertexArray(mesh->bind);
            glDrawElements(GL_TRIANGLES, static_cast<sint32>(mesh->indices.size()), GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
} // namespace

void OpenGLRenderer::Init()
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

    Resource::Load<ShaderPipeline>("Assets/Shaders/Shader.vert", "Assets/Shaders/Shader.frag");

    glUseProgram(Resource::GetResources<ShaderPipeline>()[0]->shader_pipeline.opengl);
    CreateUniformBuffer<Matrix4>(0);
    CreateUniformBuffer<Matrix4>(1);
    CreateUniformBuffer<Matrix4>(2);
}

void OpenGLRenderer::Exit() 
{
    if (!SDL_GL_DestroyContext(context)) SDL_Log("Failed to destroy GL context: %s", SDL_GetError());
}

void OpenGLRenderer::Update()
{
    const auto camera_query = ECS::GetWorld().query_builder<const Transform, const Camera>().build();
    camera_query.each(&RenderCamera);
}

void OpenGLRenderer::SwapBuffer()
{
    auto* window = static_cast<SDL_Window*>(Window::GetHandle());
    SDL_GL_SwapWindow(window);
}

void* OpenGLRenderer::GetContext() { return &context; }

void OpenGLRenderer::CreateRenderTarget(RenderTarget& target)
{
    glGenFramebuffers(1, &target.sampler.opengl);
    glGenTextures(1, &target.render_texture.opengl);
    glGenRenderbuffers(1, &target.depth_texture.opengl);

    RecreateRenderTarget(target);
}

void OpenGLRenderer::RecreateRenderTarget(RenderTarget& target)
{
    if (target.sampler.opengl == 0) return;

    glBindFramebuffer(GL_FRAMEBUFFER, target.sampler.opengl);
    glBindTexture(GL_TEXTURE_2D, target.render_texture.opengl);
    glBindRenderbuffer(GL_RENDERBUFFER, target.depth_texture.opengl);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, target.GetWidth(), target.GetHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.render_texture.opengl, 0);

    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, target.GetWidth(), target.GetHeight());
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, target.depth_texture.opengl);

    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void OpenGLRenderer::DestroyRenderTarget(RenderTarget& target)
{
    glDeleteRenderbuffers(1, &target.depth_texture.opengl);
    glDeleteTextures(1, &target.render_texture.opengl);
    glDeleteFramebuffers(1, &target.sampler.opengl);
}

void OpenGLRenderer::CreateTexture(Texture& texture)
{
    glGenTextures(1, &texture.texture.opengl);
    ReloadTexture(texture);
}


void OpenGLRenderer::ReloadTexture(Texture& texture)
{
    glBindTexture(GL_TEXTURE_2D, texture.texture.opengl);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        static_cast<sint32>(texture.width),
        static_cast<sint32>(texture.height),
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        texture.colors.data()
    );
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLRenderer::DestroyTexture(Texture& mesh) { glDeleteTextures(1, &mesh.texture.opengl); }

void OpenGLRenderer::CreateMesh(Mesh& mesh)
{
    glGenVertexArrays(1, &mesh.bind);
    glGenBuffers(1, &mesh.vertices_buffer.opengl);
    glGenBuffers(1, &mesh.indices_buffer.opengl);

    ReloadMesh(mesh);
}

void OpenGLRenderer::DestroyMesh(Mesh& mesh)
{
    glDeleteBuffers(1, &mesh.vertices_buffer.opengl);
    glDeleteBuffers(1, &mesh.indices_buffer.opengl);
    glDeleteVertexArrays(1, &mesh.bind);
}

void OpenGLRenderer::ReloadMesh(Mesh& mesh)
{
    glBindVertexArray(mesh.bind);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertices_buffer.opengl);
    glBufferData(GL_ARRAY_BUFFER, static_cast<sint32>(mesh.vertices.size() * sizeof(Vertex)), mesh.vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indices_buffer.opengl);
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

    for (const auto& texture : mesh.textures)
    {
        ReloadTexture(*texture);
    }
}

void OpenGLRenderer::CreateShader(Shader& shader)
{
    const GLuint shader_type = (shader.type == Shader::VERTEX ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);

    shader.shader.opengl = glCreateShader(shader_type);
    const char* code = shader.code.data();
    glShaderSource(shader.shader.opengl, 1, &code, nullptr);
    glCompileShader(shader.shader.opengl);

    const std::string type_name = (shader.type == Shader::VERTEX ? "vertex" : "fragment");
    CheckCompileErrors(shader.shader.opengl, type_name);
}
void OpenGLRenderer::DestroyShader(Shader& shader) { glDeleteShader(shader.shader.opengl); }

// Taken from the LearnOpenGL shader class: https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/shader_m.h
void OpenGLRenderer::CreateShaderPipeline(
    ShaderPipeline& pipeline, const Handle<Shader>& vertex_shader, const Handle<Shader>& fragment_shader
)
{
    pipeline.shader_pipeline.opengl = glCreateProgram();

    glAttachShader(pipeline.shader_pipeline.opengl, vertex_shader->shader.opengl);
    glAttachShader(pipeline.shader_pipeline.opengl, fragment_shader->shader.opengl);
    glLinkProgram(pipeline.shader_pipeline.opengl);
    CheckCompileErrors(pipeline.shader_pipeline.opengl, "program");
}

void OpenGLRenderer::DestroyShaderPipeline(ShaderPipeline& pipeline) { glDeleteProgram(pipeline.shader_pipeline.opengl); }