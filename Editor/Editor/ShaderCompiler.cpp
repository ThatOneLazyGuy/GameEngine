#include "ShaderCompiler.hpp"

#include <Core/Rendering/Renderer.hpp>
#include <Tools/Logging.hpp>
#include <Tools/Files.hpp>
#include <slang.h>
#include <slang-com-ptr.h>

#include <array>

using namespace slang;

namespace
{
    Slang::ComPtr<IGlobalSession> global_session;
    Slang::ComPtr<ISession> vertex_session;
    Slang::ComPtr<ISession> fragment_session;

    bool TryLog(const Slang::ComPtr<IBlob>& diagnostic)
    {
        if (diagnostic) Log::Log("ShaderCompiler: ", static_cast<const char*>(diagnostic->getBufferPointer()));

        return diagnostic;
    }

    bool CompareShaderHash(const std::string& path, const Slang::ComPtr<IBlob>& hash)
    {
        const std::vector<uint8>& stored_hash = Files::ReadBinary(path, false);
        if (stored_hash.size() != hash->getBufferSize()) return false;

        return std::memcmp(stored_hash.data(), hash->getBufferPointer(), stored_hash.size()) == 0;
    }

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

        std::array<PreprocessorMacroDesc, 2> preprocessor_macros{
            {{backend_name.data(), ""}, {"VERTEX", ""}}
        };

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

        Slang::ComPtr module{vertex_session->loadModule(path.c_str(), diagnostics.writeRef())};
        if (TryLog(diagnostics)) return;

        const Renderer::BackendShaderInfo& backend_shader_info = Renderer::GetBackendShaderInfo();
        const std::string new_path = path.substr(0, path.find_last_of('.'));

        const std::string vertex_path = new_path + ".vert" + Renderer::GetBackendShaderInfo().file_extension;
        const SlangInt32 vertex_session_entry_points = module->getDefinedEntryPointCount();
        for (SlangInt32 i = 0; i < vertex_session_entry_points; i++)
        {
            // Get the entry point.
            Slang::ComPtr<IEntryPoint> entry_point;
            module->getDefinedEntryPoint(i, entry_point.writeRef());

            EntryPointReflection* entry_point_reflection = entry_point->getLayout()->getEntryPointByIndex(0);
            if (entry_point_reflection->getStage() != SLANG_STAGE_VERTEX) continue;

            const std::string hash_file_path = vertex_path + ".hash";

            // Create a composite type to correctly compute the hash later.
            Slang::ComPtr<IComponentType> composite;
            IComponentType* components[2]{module, entry_point};
            vertex_session->createCompositeComponentType(components, 2, composite.writeRef());

            // Compute hash.
            Slang::ComPtr<IBlob> hash;
            composite->getEntryPointHash(
                0, 0, hash.writeRef()
            ); // We use entry point index 0 because the composite was only made with 1 entry point.
            if (CompareShaderHash(hash_file_path, hash)) break;

            Log::Log("Recompiling shader: {}", vertex_path);

            const uint8* hash_data = static_cast<const uint8*>(hash->getBufferPointer());
            Files::WriteBinary(hash_file_path, {hash_data, hash->getBufferSize()});

            // Link/compile the shader.
            Slang::ComPtr<IComponentType> linked_entry_point;
            entry_point->link(linked_entry_point.writeRef(), diagnostics.writeRef());
            if (TryLog(diagnostics)) return;

            // Get the shader data.
            Slang::ComPtr<IBlob> shader_stage_data;
            linked_entry_point->getEntryPointCode(0, 0, shader_stage_data.writeRef(), diagnostics.writeRef());
            if (TryLog(diagnostics)) return;

            // Write the shader stage data to the file.
            if (backend_shader_info.binary)
            {
                const uint8* shader_data = static_cast<const uint8*>(shader_stage_data->getBufferPointer());
                Files::WriteBinary(vertex_path, {shader_data, shader_stage_data->getBufferSize()});
            }
            else
            {
                const char* shader_text = static_cast<const char*>(shader_stage_data->getBufferPointer());
                Files::WriteText(vertex_path, {shader_text, shader_stage_data->getBufferSize()});
            }

            break;
        }

        module = Slang::ComPtr{fragment_session->loadModule(path.c_str(), diagnostics.writeRef())};
        if (diagnostics) Log::Log(static_cast<const char*>(diagnostics->getBufferPointer()));

        const std::string fragment_path = new_path + ".frag" + Renderer::GetBackendShaderInfo().file_extension;
        const SlangInt32 fragment_session_entry_points = module->getDefinedEntryPointCount();
        for (SlangInt32 i = 0; i < fragment_session_entry_points; i++)
        {
            // Get the entry point.
            Slang::ComPtr<IEntryPoint> entry_point;
            module->getDefinedEntryPoint(i, entry_point.writeRef());

            EntryPointReflection* entry_point_reflection = entry_point->getLayout()->getEntryPointByIndex(0);
            if (entry_point_reflection->getStage() != SLANG_STAGE_FRAGMENT) continue;

            const std::string hash_file_path = fragment_path + ".hash";

            // Create a composite type to correctly compute the hash later.
            Slang::ComPtr<IComponentType> composite;
            IComponentType* components[2]{module, entry_point};
            vertex_session->createCompositeComponentType(components, 2, composite.writeRef());

            // Compute hash.
            Slang::ComPtr<IBlob> hash;
            composite->getEntryPointHash(
                0, 0, hash.writeRef()
            ); // We use entry point index 0 because the composite was only made with 1 entry point.
            if (CompareShaderHash(hash_file_path, hash)) break;

            Log::Log("Recompiling shader: {}", fragment_path);

            const uint8* hash_data = static_cast<const uint8*>(hash->getBufferPointer());
            Files::WriteBinary(hash_file_path, {hash_data, hash->getBufferSize()});

            // Link/compile the shader.
            Slang::ComPtr<IComponentType> linked_entry_point;
            entry_point->link(linked_entry_point.writeRef(), diagnostics.writeRef());
            if (TryLog(diagnostics)) return;

            // Get the shader data.
            Slang::ComPtr<IBlob> shader_stage_data;
            linked_entry_point->getEntryPointCode(0, 0, shader_stage_data.writeRef(), diagnostics.writeRef());
            if (TryLog(diagnostics)) return;

            // Write the shader stage data to the file.
            if (backend_shader_info.binary)
            {
                const uint8* shader_data = static_cast<const uint8*>(shader_stage_data->getBufferPointer());
                Files::WriteBinary(fragment_path, {shader_data, shader_stage_data->getBufferSize()});
            }
            else
            {
                const char* shader_text = static_cast<const char*>(shader_stage_data->getBufferPointer());
                Files::WriteText(fragment_path, {shader_text, shader_stage_data->getBufferSize()});
            }

            break;
        }
    }
} // namespace ShaderCompiler