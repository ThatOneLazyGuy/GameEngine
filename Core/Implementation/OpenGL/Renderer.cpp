#include "Renderer.hpp"

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_video.h>

#define STB_IMAGE_IMPLEMENTATION
#include "Core/Model.hpp"
#include "Core/Window.hpp"
#include "SDL3/SDL_timer.h"

#include <stb_image.h>

#include <glm/fwd.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
#include <fstream>
#include <glad/glad.h>
#include <iostream>
#include <map>
#include <string>

Mesh OpenGLRenderer::CreateMesh(const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices)
{
    Mesh mesh{.vertices = std::move(vertices), .indices = std::move(indices)};
    std::uint32_t VBO, EBO;

    glGenVertexArrays(1, &mesh.bind);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    mesh.render_vertices = reinterpret_cast<void*>(static_cast<std::uint64_t>(VBO));
    mesh.render_indices = reinterpret_cast<void*>(static_cast<std::uint64_t>(EBO));

    this->ReloadMesh(mesh);

    return std::move(mesh);
}

void OpenGLRenderer::DeleteMesh(Mesh& mesh)
{
    const auto VBO = static_cast<std::uint32_t>(reinterpret_cast<std::uint64_t>(mesh.render_indices));
    const auto EBO = static_cast<std::uint32_t>(reinterpret_cast<std::uint64_t>(mesh.render_vertices));
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &mesh.bind);
}

void OpenGLRenderer::ReloadMesh(Mesh& mesh)
{
    glBindVertexArray(mesh.bind);

    glBindBuffer(GL_ARRAY_BUFFER, static_cast<std::uint32_t>(reinterpret_cast<std::uint64_t>(mesh.render_vertices)));
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex), mesh.vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(
        GL_ELEMENT_ARRAY_BUFFER, static_cast<std::uint32_t>(reinterpret_cast<std::uint64_t>(mesh.render_indices))
    );
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(std::uint32_t), mesh.indices.data(), GL_STATIC_DRAW
    );

    const void* offset = nullptr;
    glVertexAttribPointer(0, glm::vec3::length(), GL_FLOAT, GL_FALSE, 8 * sizeof(float), offset);
    glEnableVertexAttribArray(0);
    offset += sizeof(glm::vec3);

    glVertexAttribPointer(1, glm::vec3::length(), GL_FLOAT, GL_FALSE, 8 * sizeof(float), offset);
    glEnableVertexAttribArray(1);
    offset += sizeof(glm::vec3);

    glVertexAttribPointer(2, glm::vec2::length(), GL_FLOAT, GL_FALSE, 8 * sizeof(float), offset);
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

class Camera
{
  public:
    [[nodiscard]] glm::mat4 GetViewMatrix() const { return glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp); }

    glm::vec3 cameraPos{0.0f, 0.0f, 0.0f};

  private:
    glm::vec3 cameraFront{0.0f, 0.0f, 1.0f};
    glm::vec3 cameraUp{0.0f, 1.0f, 0.0f};
} camera;

class Shader
{
  public:
    unsigned int ID;

    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    Shader(const char* vertexPath, const char* fragmentPath)
    {
        // 1. retrieve the vertex/fragment source code from filePath
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;

        // ensure ifstream objects can throw exceptions:
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try
        {
            // open files
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;
            // read file's buffer contents into streams
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();
            // close file handlers
            vShaderFile.close();
            fShaderFile.close();
            // convert stream into string
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch (std::ifstream::failure& e)
        {
            SDL_Log("ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: %s", e.what());
        }
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        // 2. compile shaders
        unsigned int vertex, fragment;

        // vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, nullptr);
        glCompileShader(vertex);
        CheckCompileErrors(vertex, "VERTEX");

        // fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, nullptr);
        glCompileShader(fragment);
        CheckCompileErrors(fragment, "FRAGMENT");

        // shader Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        CheckCompileErrors(ID, "PROGRAM");

        // delete the shaders as they're linked into our program now and no longer necessary
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    // activate the shader
    void Use() const { glUseProgram(ID); }

    template <typename Type> void CreateUniformBuffer(const int binding)
    {
        unsigned int& UBO = uniformBuffers[binding];

        glUniformBlockBinding(ID, 0, binding);
        glGenBuffers(1, &UBO);

        glBindBuffer(GL_UNIFORM_BUFFER, UBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(Type), nullptr, GL_STATIC_DRAW);
        glBindBufferRange(GL_UNIFORM_BUFFER, binding, UBO, 0, sizeof(Type));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    // utility uniform functions
    template <typename Type> void SetUniform(const int binding, const Type& value) const
    {
        const auto iterator = uniformBuffers.find(binding);
        if (iterator == uniformBuffers.end())
        {
            SDL_Log("Invalid shader uniform buffer");
            return;
        }

        const unsigned int UBO = iterator->second;

        glBindBuffer(GL_UNIFORM_BUFFER, UBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Type), &value);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    template <typename> void SetUniform(const int binding, const glm::mat4& value) const
    {
        const auto iterator = uniformBuffers.find(binding);
        if (iterator == uniformBuffers.end())
        {
            SDL_Log("Invalid shader uniform buffer");
            return;
        }

        const unsigned int UBO = iterator->second;

        glBindBuffer(GL_UNIFORM_BUFFER, UBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(value));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    template <size_t TextureType = GL_TEXTURE_2D> static void SetTexture(const unsigned int texture, const int binding)
    {
        glActiveTexture(GL_TEXTURE0 + binding);
        glBindTexture(TextureType, texture);
    }

  private:
    std::map<int, unsigned int> uniformBuffers;

    // utility function for checking shader compilation/linking errors.
    // ------------------------------------------------------------------------
    static void CheckCompileErrors(unsigned int shader, const std::string& type)
    {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
                          << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n"
                          << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};

namespace
{
    SDL_GLContext context;
    std::unique_ptr<Shader> shader = nullptr;
} // namespace

unsigned int LoadTexture(const char* path)
{
    unsigned int textureID;

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(true);

    // load and generate the texture
    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data)
    {
        const GLenum format = nrChannels == 4 ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else { std::cout << "Failed to load texture" << '\n'; }
    stbi_image_free(data);

    return textureID;
}

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

    shader = std::make_unique<Shader>("Assets/Shaders/Shader.vert", "Assets/Shaders/Shader.frag");

    shader->Use();
    shader->CreateUniformBuffer<glm::mat4>(0);

    // containerTexture = LoadTexture("Assets/container.jpg");
    // SDL_Log("Error: %i", glGetError());
    // glUniform1i(0, 0);
    // SDL_Log("Error: %i", glGetError());
    // faceTexture = LoadTexture("Assets/awesomeface.png");
    // SDL_Log("Error: %i", glGetError());
    // glUniform1i(1, 1);
    // SDL_Log("Error: %i", glGetError());
}

void OpenGLRenderer::Exit() {}

void OpenGLRenderer::Update()
{
    static Model loaded_model{"Assets/cube_with_vertexcolors.obj"};

    glClearColor(0.75f, 0.81f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto model = glm::mat4(1.0f);

    const size_t time = SDL_GetTicks();
    model = glm::translate(model, glm::vec3{0.5f, -0.5f, 1.5f});
    model = glm::rotate(model, static_cast<float>(time) / 600.0f, glm::vec3{0.0f, 1.0f, 0.0f});

    const float width = static_cast<float>(Window::GetWidth());
    const float height = static_cast<float>(Window::GetHeight());

    camera.cameraPos = glm::vec3(0.0f, 0.0f, size_test[0]);
    const glm::mat4 view = camera.GetViewMatrix();
    const glm::mat4 projection = glm::perspective(glm::radians(45.0f), width / height, 0.1f, 100.0f);

    shader->Use();

    shader->SetUniform<glm::mat4>(0, projection * view * model);

    glBindVertexArray(loaded_model.meshes[0]->bind);
    glDrawElements(GL_TRIANGLES, loaded_model.meshes[0]->indices.size(), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void OpenGLRenderer::SwapBuffer()
{
    auto* window = static_cast<SDL_Window*>(Window::GetHandle());
    SDL_GL_SwapWindow(window);
}

void* OpenGLRenderer::GetContext() { return &context; }
void* OpenGLRenderer::GetTexture() { return nullptr; }
