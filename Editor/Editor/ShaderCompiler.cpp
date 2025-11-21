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
    Slang::ComPtr<ISession> default_session;
} // namespace

namespace ShaderCompiler
{
    void Init()
    {
        SlangGlobalSessionDesc description{.enableGLSL = true};
        createGlobalSession(&description, global_session.writeRef());

        const std::array<TargetDesc, 1> targets{
            {{.format = SLANG_SPIRV, .profile = global_session->findProfile("spirv_1_3")}}
        };
        constexpr std::array<const char*, 1> search_paths{{"Assets/Shaders/"}};
        const std::array<PreprocessorMacroDesc, 3> preprocessor_macros{{{Renderer::GetBackendName().data(), "1"}, {"Sampler", "0"}, {"Uniform", "1"}}};
        //const std::array<PreprocessorMacroDesc, 1> preprocessor_macros{{{Renderer::GetBackendName().data(), "1"}}};

        const SessionDesc default_session_description{
            .targets = targets.data(),
            .targetCount = targets.size(),

            .searchPaths = search_paths.data(),
            .searchPathCount = search_paths.size(),

            .preprocessorMacros = preprocessor_macros.data(),
            .preprocessorMacroCount = preprocessor_macros.size(),
        };
        global_session->createSession(default_session_description, default_session.writeRef());
    }

    void CompileShader(const std::string& path)
    {
        Slang::ComPtr<IBlob> diagnostics;
        Slang::ComPtr<IModule> module{default_session->loadModule(path.c_str(), diagnostics.writeRef())};
        if (diagnostics) Log::Log(static_cast<const char*>(diagnostics->getBufferPointer()));

        Slang::ComPtr<IEntryPoint> vertex_entry_point;
        module->findEntryPointByName("VertexMain", vertex_entry_point.writeRef());

        IComponentType* vertex_components[] = {module, vertex_entry_point};
        Slang::ComPtr<IComponentType> vertex_program;
        default_session->createCompositeComponentType(vertex_components, 2, vertex_program.writeRef(), diagnostics.writeRef());
        if (diagnostics) Log::Log(static_cast<const char*>(diagnostics->getBufferPointer()));

        Slang::ComPtr<IComponentType> vertex_linked_program;
        vertex_program->link(vertex_linked_program.writeRef(), diagnostics.writeRef());
        if (diagnostics) Log::Log(static_cast<const char*>(diagnostics->getBufferPointer()));

        Slang::ComPtr<IBlob> vertex_kernal_blob;
        vertex_linked_program->getEntryPointCode(0, 0, vertex_kernal_blob.writeRef(), diagnostics.writeRef());
        if (diagnostics) Log::Log(static_cast<const char*>(diagnostics->getBufferPointer()));
        
        const std::string new_path = path.substr(0, path.find_last_of('.'));
        const std::string vertex_path = new_path + ".vert" + Renderer::GetBackendShaderInfo().file_extension;
        std::ofstream vertex_file{vertex_path, std::ios::binary | std::ios::trunc};
        vertex_file.write(static_cast<const char*>(vertex_kernal_blob->getBufferPointer()), vertex_kernal_blob->getBufferSize());
        vertex_file.close();



        Slang::ComPtr<IEntryPoint> fragment_entry_point;
        module->findEntryPointByName("FragmentMain", fragment_entry_point.writeRef());

        IComponentType* fragment_components[] = {module, vertex_entry_point};
        Slang::ComPtr<IComponentType> fragment_program;
        default_session->createCompositeComponentType(fragment_components, 2, fragment_program.writeRef(), diagnostics.writeRef());
        if (diagnostics) Log::Log(static_cast<const char*>(diagnostics->getBufferPointer()));

        Slang::ComPtr<IComponentType> fragment_linked_program;
        fragment_program->link(fragment_linked_program.writeRef(), diagnostics.writeRef());
        if (diagnostics) Log::Log(static_cast<const char*>(diagnostics->getBufferPointer()));

        Slang::ComPtr<IBlob> fragment_kernal_blob;
        fragment_linked_program->getEntryPointCode(0, 0, fragment_kernal_blob.writeRef(), diagnostics.writeRef());
        if (diagnostics) Log::Log(static_cast<const char*>(diagnostics->getBufferPointer()));

        const std::string fragment_path = new_path + ".frag" + Renderer::GetBackendShaderInfo().file_extension;
        std::ofstream fragment_file{fragment_path, std::ios::binary | std::ios::trunc};
        fragment_file.write(static_cast<const char*>(fragment_kernal_blob->getBufferPointer()), fragment_kernal_blob->getBufferSize());
        fragment_file.close();
    }
} // namespace ShaderCompiler