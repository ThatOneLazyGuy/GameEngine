#include "Renderer.hpp"

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>

#include "Core/Model.hpp"
#include "Core/Window.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <filesystem>
#include <fstream>
#include <glad/glad.h>
#include <iostream>
#include <map>
#include <string>

#include "Core/Math.hpp"

namespace
{
    SDL_GLContext context;
    Shader default_shader;

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
                std::cout << "Error compiling shader of type: " << type << "\n"
                          << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(id, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(id, 1024, nullptr, infoLog);
                std::cout << "Error linking program of type: " << type << "\n"
                          << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }

    template <typename Type>
    void CreateUniformBuffer(const Shader shader, const int binding)
    {
        unsigned int& UBO = uniformBuffers[binding];

        glGenBuffers(1, &UBO);

        glBindBuffer(GL_UNIFORM_BUFFER, UBO);
        //glBufferData(GL_UNIFORM_BUFFER, sizeof(Type), nullptr, GL_STATIC_DRAW);
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
    void SetUniform(const int binding, const Mat4& value)
    {
        const auto iterator = uniformBuffers.find(binding);
        if (iterator == uniformBuffers.end())
        {
            SDL_Log("Invalid shader uniform buffer");
            return;
        }

        const unsigned int UBO = iterator->second;

        glBindBuffer(GL_UNIFORM_BUFFER, UBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(Mat4), value.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    template <size_t TextureType = GL_TEXTURE_2D>
    void SetTexture(const unsigned int texture, const int binding)
    {
        glActiveTexture(GL_TEXTURE0 + binding);
        glBindTexture(TextureType, texture);
    }
} // namespace

void OpenGLRenderer::Init(void* window_handle)
{
    auto* window = static_cast<SDL_Window*>(window_handle);
    context = SDL_GL_CreateContext(window);

    if (context == nullptr) SDL_Log("Failed to create GL context: %s", SDL_GetError());

    if (!gladLoadGLLoader(
            reinterpret_cast<GLADloadproc>(&SDL_GL_GetProcAddress)
        )) // NOLINT(clang-diagnostic-cast-function-type-strict)
    {
        SDL_Log("Failed to initialize GLAD");
    }

    SDL_GL_SetSwapInterval(1);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);

    default_shader = CreateShader("Assets/Shaders/Shader.vert", "Assets/Shaders/Shader.frag");

    glUseProgram(default_shader.program);
    CreateUniformBuffer<Mat4>(default_shader, 0);
    CreateUniformBuffer<Mat4>(default_shader, 1);
    CreateUniformBuffer<Mat4>(default_shader, 2);
}

void OpenGLRenderer::Exit() {}

void OpenGLRenderer::Update()
{
    glClearColor(0.75f, 0.81f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto model = Identity<Mat4>();

    const size_t time = SDL_GetTicks();
    model *= Rotation(static_cast<float>(time) / 600.0f, Vec3{0.0f, 1.0f, 0.0f});
    model *= Translation(Vec3{0.5f, -0.5f, -2.5f});

    const auto width = static_cast<float>(Window::GetWidth());
    const auto height = static_cast<float>(Window::GetHeight());

    const Vec3 cameraPos{position};
    const Mat4 view = LookAt(cameraPos, Vec3{0.0f, 0.0f, -1.0f}, Vec3{0.0f, 1.0f, 0.0f});
    const Mat4 projection = PerspectiveNO(ToRadians(fov), width / height, 0.1f, 100.0f);

    glUseProgram(default_shader.program);
    SetUniform<Mat4>(0, model);
    SetUniform<Mat4>(1, view);
    SetUniform<Mat4>(2, projection);

    for (const auto& mesh_handle : Resource::GetResources<Mesh>())
    {
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

            glBindTexture(GL_TEXTURE_2D, texture->id);
        }

        glBindVertexArray(mesh_handle->bind);
        glDrawElements(GL_TRIANGLES, static_cast<sint32>(mesh_handle->indices.size()), GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }
}

void OpenGLRenderer::SwapBuffer()
{
    auto* window = static_cast<SDL_Window*>(Window::GetHandle());
    SDL_GL_SwapWindow(window);
}

void* OpenGLRenderer::GetContext() { return &context; }

Mesh OpenGLRenderer::CreateMesh(
    const std::vector<Vertex>& vertices, const std::vector<uint32>& indices,
    const std::vector<Handle<Texture>>& textures
)
{
    Mesh mesh;
    mesh.vertices = vertices;
    mesh.indices = indices;
    mesh.textures = textures;

    glGenVertexArrays(1, &mesh.bind);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);

    ReloadMesh(mesh);

    return std::move(mesh);
}

void OpenGLRenderer::DeleteMesh(Mesh& mesh)
{
    glDeleteBuffers(1, &mesh.VBO);
    glDeleteBuffers(1, &mesh.EBO);
    glDeleteVertexArrays(1, &mesh.bind);
}

void OpenGLRenderer::ReloadMesh(Mesh& mesh)
{
    glBindVertexArray(mesh.bind);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<sint32>(mesh.vertices.size() * sizeof(Vertex)),
        mesh.vertices.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<sint32>(mesh.indices.size() * sizeof(uint32)),
        mesh.indices.data(),
        GL_STATIC_DRAW
    );

    const void* offset = nullptr;
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), offset);
    glEnableVertexAttribArray(0);
    offset += sizeof(Vec3);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), offset);
    glEnableVertexAttribArray(1);
    offset += sizeof(Vec3);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), offset);
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    for (const auto& texture : mesh.textures)
    {
        ReloadTexture(*texture);
    }
}

Texture OpenGLRenderer::CreateTexture(const std::string& texture_path, const Texture::Type type)
{
    sint32 width, height, component_count;
    void* data = stbi_load(("Assets/Backpack/" + texture_path).c_str(), &width, &height, &component_count, 4);

    const uint32 pixel_count = width * height;
    if (data != nullptr)
    {
        const auto* pixel_data = static_cast<const uint32*>(data);
        const std::vector colors(pixel_data, pixel_data + pixel_count);
        stbi_image_free(data);

        return std::move(CreateTexture(width, height, colors, type));
    }

    SDL_Log("Failed to load OpenGL Texture: %s", stbi_failure_reason());
    return Texture{};
}

Texture OpenGLRenderer::CreateTexture(
    const uint32 width, const uint32 height, const std::vector<uint32>& colors, const Texture::Type type
)
{
    Texture texture;
    texture.type = type;
    texture.width = width;
    texture.height = height;
    texture.colors = colors;

    glGenTextures(1, &texture.id);
    ReloadTexture(texture);

    return std::move(texture);
}


void OpenGLRenderer::ReloadTexture(Texture& texture)
{
    glBindTexture(GL_TEXTURE_2D, texture.id);
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

void OpenGLRenderer::DeleteTexture(Texture& mesh) { glDeleteTextures(1, &mesh.id); }

// Taken from the LearnOpenGL shader class: https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/shader_m.h
Shader OpenGLRenderer::CreateShader(const std::string& vertex_path, const std::string& fragment_path)
{
    // 1. retrieve the vertex/fragment source code from filePath
    std::string vertex_code;
    std::string fragment_code;
    std::ifstream vertex_file;
    std::ifstream fragment_file;

    // ensure ifstream objects can throw exceptions:
    vertex_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fragment_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        // open files
        vertex_file.open(vertex_path);
        fragment_file.open(fragment_path);
        std::stringstream vShaderStream, fShaderStream;
        // read file's buffer contents into streams
        vShaderStream << vertex_file.rdbuf();
        fShaderStream << fragment_file.rdbuf();
        // close file handlers
        vertex_file.close();
        fragment_file.close();
        // convert stream into string
        vertex_code = vShaderStream.str();
        fragment_code = fShaderStream.str();
    }
    catch (std::ifstream::failure& e)
    {
        SDL_Log("Error reading shader file: %s", e.what());
    }
    const char* vertex_shader_code = vertex_code.c_str();
    const char* fragment_shader_code = fragment_code.c_str();

    // 2. compile shaders
    uint32 vertex, fragment;

    // vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertex_shader_code, nullptr);
    glCompileShader(vertex);
    CheckCompileErrors(vertex, "vertex");

    // fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragment_shader_code, nullptr);
    glCompileShader(fragment);
    CheckCompileErrors(fragment, "fragment");

    // shader Program
    uint32 opengl_id;
    opengl_id = glCreateProgram();

    glAttachShader(opengl_id, vertex);
    glAttachShader(opengl_id, fragment);
    glLinkProgram(opengl_id);
    CheckCompileErrors(opengl_id, "program");

    // delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return Shader{.program = opengl_id};
}

void OpenGLRenderer::DeleteShader(Shader shader) { glDeleteProgram(shader.program); }