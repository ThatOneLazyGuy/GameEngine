#include "ShaderCompiler.hpp"

#include <slang.h>
#include <slang-com-ptr.h>
#include <Tools/Logging.hpp>
#include <Core/Rendering/Renderer.hpp>

#include <array>
#include <fstream>

using namespace slang;

namespace
{
    Slang::ComPtr<IGlobalSession> global_session;
    Slang::ComPtr<ISession> vertex_session;
    Slang::ComPtr<ISession> fragment_session;
} // namespace

namespace ShaderCompiler
{
    void Init()
    {
        SlangGlobalSessionDesc description{.enableGLSL = true};
        createGlobalSession(&description, global_session.writeRef());

        const Renderer::BackendShaderInfo& backend_shader_info = Renderer::GetBackendShaderInfo();
        const std::string& backend_name = Renderer::GetBackendName();

        std::array<CompilerOptionEntry, 1> compiler_options{
            {{CompilerOptionName::VulkanInvertY, {.intValue0 = backend_shader_info.invert_y}}}
        };
        const SlangProfileID profile = global_session->findProfile(backend_shader_info.profile);
        const std::array<TargetDesc, 1> targets{
            {{.format = (backend_name == "SDL3GPU" ? SLANG_SPIRV : SLANG_GLSL),
              .profile = profile,
              .compilerOptionEntries = compiler_options.data(),
              .compilerOptionEntryCount = compiler_options.size()}}
        };

        constexpr std::array<const char*, 1> search_paths{{"Assets/Shaders/"}};

        std::array<PreprocessorMacroDesc, 2> preprocessor_macros{{{backend_name.data(), ""}, {"VERTEX", ""}}};

        const SessionDesc default_session_description{
            .targets = targets.data(),
            .targetCount = targets.size(),

            .searchPaths = search_paths.data(),
            .searchPathCount = search_paths.size(),

            .preprocessorMacros = preprocessor_macros.data(),
            .preprocessorMacroCount = preprocessor_macros.size(),
        };
        global_session->createSession(default_session_description, vertex_session.writeRef());

        preprocessor_macros[1] = {"FRAGMENT", ""};
        global_session->createSession(default_session_description, fragment_session.writeRef());
    }

    void CompileShader(const std::string& path)
    {
        Slang::ComPtr<IBlob> diagnostics;

        Slang::ComPtr<IModule> module{vertex_session->loadModule(path.c_str(), diagnostics.writeRef())};
        if (diagnostics) Log::Log(static_cast<const char*>(diagnostics->getBufferPointer()));

        Slang::ComPtr<IEntryPoint> vertex_entry_point;
        module->findEntryPointByName("VertexMain", vertex_entry_point.writeRef());

        IComponentType* vertex_components[] = {module, vertex_entry_point};
        Slang::ComPtr<IComponentType> vertex_program;
        vertex_session->createCompositeComponentType(vertex_components, 2, vertex_program.writeRef(), diagnostics.writeRef());
        if (diagnostics) Log::Log(static_cast<const char*>(diagnostics->getBufferPointer()));

        Slang::ComPtr<IComponentType> vertex_linked_program;
        vertex_program->link(vertex_linked_program.writeRef(), diagnostics.writeRef());
        if (diagnostics) Log::Log(static_cast<const char*>(diagnostics->getBufferPointer()));

        Slang::ComPtr<IBlob> vertex_kernal_blob;
        vertex_linked_program->getEntryPointCode(0, 0, vertex_kernal_blob.writeRef(), diagnostics.writeRef());
        if (diagnostics) Log::Log(static_cast<const char*>(diagnostics->getBufferPointer()));

        const Renderer::BackendShaderInfo& backend_shader_info = Renderer::GetBackendShaderInfo();
        const std::string new_path = path.substr(0, path.find_last_of('.'));
        const std::string vertex_path = new_path + ".vert" + Renderer::GetBackendShaderInfo().file_extension;
        std::ofstream vertex_file{vertex_path, std::ios::trunc | (backend_shader_info.binary ? std::ios::binary : 0)};
        vertex_file.write(static_cast<const char*>(vertex_kernal_blob->getBufferPointer()), vertex_kernal_blob->getBufferSize());
        vertex_file.close();


        module = Slang::ComPtr<IModule>{fragment_session->loadModule(path.c_str(), diagnostics.writeRef())};
        if (diagnostics) Log::Log(static_cast<const char*>(diagnostics->getBufferPointer()));

        Slang::ComPtr<IEntryPoint> fragment_entry_point;
        module->findEntryPointByName("FragmentMain", fragment_entry_point.writeRef());

        IComponentType* fragment_components[] = {module, fragment_entry_point};
        Slang::ComPtr<IComponentType> fragment_program;
        fragment_session->createCompositeComponentType(fragment_components, 2, fragment_program.writeRef(), diagnostics.writeRef());
        if (diagnostics) Log::Log(static_cast<const char*>(diagnostics->getBufferPointer()));

        Slang::ComPtr<IComponentType> fragment_linked_program;
        fragment_program->link(fragment_linked_program.writeRef(), diagnostics.writeRef());
        if (diagnostics) Log::Log(static_cast<const char*>(diagnostics->getBufferPointer()));

        Slang::ComPtr<IBlob> fragment_kernal_blob;
        fragment_linked_program->getEntryPointCode(0, 0, fragment_kernal_blob.writeRef(), diagnostics.writeRef());
        if (diagnostics) Log::Log(static_cast<const char*>(diagnostics->getBufferPointer()));

        const std::string fragment_path = new_path + ".frag" + Renderer::GetBackendShaderInfo().file_extension;
        std::ofstream fragment_file{fragment_path, std::ios::trunc | (backend_shader_info.binary ? std::ios::binary : 0)};
        fragment_file.write(static_cast<const char*>(fragment_kernal_blob->getBufferPointer()), fragment_kernal_blob->getBufferSize());
        fragment_file.close();
    }
} // namespace ShaderCompiler