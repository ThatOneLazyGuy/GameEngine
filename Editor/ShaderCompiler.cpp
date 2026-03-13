#include "ShaderCompiler.hpp"

#include <Rendering/Renderer.hpp>
#include <Logging.hpp>
#include <Files.hpp>

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
        if (diagnostic) Log::Log("ShaderCompiler: {}", static_cast<const char*>(diagnostic->getBufferPointer()));

        return diagnostic;
    }

    bool CompareShaderHash(const std::string& path, const Slang::ComPtr<IBlob>& hash)
    {
        const std::vector<uint8>& stored_hash = Files::ReadBinary(path, false);
        if (stored_hash.size() != hash->getBufferSize()) return false;

        return std::memcmp(stored_hash.data(), hash->getBufferPointer(), stored_hash.size()) == 0;
    }

    void ReflectGlobalParameters(
        ProgramLayout* shader_layout, const Slang::ComPtr<IMetadata>& metadata, ShaderSettings& settings,
        GraphicsPipelineSettings& pipeline_settings
    )
    {
        Slang::ComPtr<IBlob> diagnostic{};

        VariableLayoutReflection* global_params = shader_layout->getGlobalParamsVarLayout();
        if (global_params == nullptr) return;

        TypeLayoutReflection* type_layout = global_params->getTypeLayout();
        for (uint32 i = 0; i < type_layout->getFieldCount(); i++)
        {
            VariableLayoutReflection* param_layout = type_layout->getFieldByIndex(i);
            TypeLayoutReflection* param_type_layout = param_layout->getTypeLayout();

            bool is_used = false;
            const auto category = static_cast<SlangParameterCategory>(param_type_layout->getParameterCategory());
            metadata->isParameterLocationUsed(category, param_layout->getBindingSpace(), param_layout->getOffset(), is_used);

            if (!is_used) continue;

            TypeReflection* parameter_type = param_layout->getVariable()->getType();
            TypeReflection::Kind parameter_kind = parameter_type->getKind();
            switch (parameter_kind)
            {
            case TypeReflection::Kind::ConstantBuffer:
            {
                // Get the TypeLayout of the constant buffer, then get the TypeLayout of the type it holds, then get the size of THAT.
                const usize type_size = param_type_layout->getElementTypeLayout()->getSize();
                const std::string& param_name = param_layout->getName();
                if (!pipeline_settings.uniform_sizes.contains(param_name)) pipeline_settings.uniform_sizes[param_name] = type_size;

                settings.uniform_count++;
                break;
            }

            case TypeReflection::Kind::Resource:
            {
                if (parameter_type->getResourceShape() ^ (SLANG_TEXTURE_2D | SLANG_TEXTURE_COMBINED_FLAG)) break;
                settings.sampler_count++;
                break;
            }

            default:
            {
                Log::Log("ShaderCompiler: Unknown global parameter kind: ", static_cast<uint32>(parameter_kind));
                break;
            }
            }
        }
    }

    void RecurseVertexAttributes(
        VariableLayoutReflection* param_layout, const Slang::ComPtr<IMetadata>& metadata, GraphicsPipelineSettings& pipeline_settings
    )
    {
        TypeLayoutReflection* param_type_layout = param_layout->getTypeLayout();
        const uint32 field_count = param_type_layout->getFieldCount();
        if (field_count != 0)
        {
            for (uint32 i = 0; i < field_count; i++)
            {
                RecurseVertexAttributes(param_type_layout->getFieldByIndex(i), metadata, pipeline_settings);
            }
            return;
        }

        if (param_type_layout->getParameterCategory() == VaryingOutput) return;

        const char* semantic_name = param_layout->getSemanticName();
        pipeline_settings.vertex_attributes.emplace_back(
            (semantic_name != nullptr ? semantic_name : ""), VertexAttribute::FLOAT, param_type_layout->getElementCount()
        );
    }

    void ReflectCompositeShader(const Slang::ComPtr<IComponentType>& composite, GraphicsPipelineSettings& pipeline_settings)
    {
        Slang::ComPtr<IBlob> diagnostic{};

        Slang::ComPtr<IMetadata> metadata;
        composite->getEntryPointMetadata(0, 0, metadata.writeRef(), diagnostic.writeRef());

        ProgramLayout* shader_layout = composite->getLayout();

        EntryPointReflection* entry_point_layout = shader_layout->getEntryPointByIndex(0);
        switch (entry_point_layout->getStage())
        {
        case SLANG_STAGE_VERTEX:
        {
            pipeline_settings.vertex_info.type = Shader::VERTEX;
            ReflectGlobalParameters(shader_layout, metadata, pipeline_settings.vertex_info, pipeline_settings);

            VariableLayoutReflection* entry_point_params = entry_point_layout->getVarLayout();
            if (entry_point_params == nullptr) break;

            RecurseVertexAttributes(entry_point_params, metadata, pipeline_settings);
            break;
        }

        case SLANG_STAGE_FRAGMENT:
        {
            pipeline_settings.fragment_info.type = Shader::FRAGMENT;
            ReflectGlobalParameters(shader_layout, metadata, pipeline_settings.fragment_info, pipeline_settings);
            break;
        }

        default:
            break;
        }
    }

    ShaderCompiler::ShaderData CompileStage(
        const Slang::ComPtr<IModule>& module, const std::string& base_path, const SlangStage stage,
        GraphicsPipelineSettings& pipeline_settings
    )
    {
        ShaderCompiler::ShaderData out_info{};

        Slang::ComPtr<IBlob> diagnostics{};

        const Renderer::BackendShaderInfo& backend_shader_info = Renderer::GetBackendShaderInfo();

        const std::string shader_path = base_path + backend_shader_info.file_extension;
        const SlangInt32 shader_session_entry_points = module->getDefinedEntryPointCount();
        for (SlangInt32 i = 0; i < shader_session_entry_points; i++)
        {
            // Get the entry point.
            Slang::ComPtr<IEntryPoint> entry_point;
            module->getDefinedEntryPoint(i, entry_point.writeRef());

            EntryPointReflection* entry_point_reflection = entry_point->getLayout()->getEntryPointByIndex(0);
            if (entry_point_reflection->getStage() != stage) continue;

            const std::string hash_file_path = shader_path + ".hash";

            // Create a composite type to correctly compute the hash later.
            Slang::ComPtr<IComponentType> composite;
            IComponentType* components[2]{module, entry_point};
            vertex_session->createCompositeComponentType(components, 2, composite.writeRef(), diagnostics.writeRef());
            if (TryLog(diagnostics)) break;

            ReflectCompositeShader(composite, pipeline_settings);

            // Compute hash.
            Slang::ComPtr<IBlob> hash;
            composite->getEntryPointHash(
                0, 0, hash.writeRef()
            ); // We use entry point index 0 because the composite was only made with 1 entry point.
            // if (CompareShaderHash(hash_file_path, hash)) break;

            Log::Log("Recompiling shader: {}", shader_path);

            const auto* hash_data = static_cast<const uint8*>(hash->getBufferPointer());
            Files::WriteBinary(hash_file_path, {hash_data, hash->getBufferSize()});

            // Link/compile the shader.
            Slang::ComPtr<IComponentType> linked_entry_point;
            entry_point->link(linked_entry_point.writeRef(), diagnostics.writeRef());
            if (TryLog(diagnostics)) break;

            // Get the shader data.
            Slang::ComPtr<IBlob> shader_stage_data;
            linked_entry_point->getEntryPointCode(0, 0, shader_stage_data.writeRef(), diagnostics.writeRef());
            if (TryLog(diagnostics)) break;

            out_info.shader_path = shader_path;

            // Write the shader stage data to the file.
            const auto* shader_data = static_cast<const uint8*>(shader_stage_data->getBufferPointer());
            out_info.data = std::vector<uint8>{shader_data, shader_data + shader_stage_data->getBufferSize()};

            break;
        }

        return out_info;
    }

} // namespace

namespace ShaderCompiler
{
    void Init()
    {
        constexpr SlangGlobalSessionDesc description{};
        createGlobalSession(&description, global_session.writeRef());

        const Renderer::BackendShaderInfo& backend_shader_info = Renderer::GetBackendShaderInfo();
        const std::string& backend_name = Renderer::GetBackendName();

        std::array<CompilerOptionEntry, 2> compiler_options{
            {{CompilerOptionName::VulkanInvertY, {.intValue0 = backend_shader_info.invert_y}},
             {CompilerOptionName::NoMangle, {.intValue0 = true}}}
        };
        const SlangProfileID profile = global_session->findProfile(backend_shader_info.profile);
        const std::array<TargetDesc, 1> targets{
            {{.format = (backend_name == "SDL3GPU" ? SLANG_SPIRV : SLANG_GLSL),
              .profile = profile,
              .flags = 0,
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

    GraphicsPipelineSettings CompileGraphicsShaders(const std::string& path, ShaderData* vertex, ShaderData* fragment)
    {
        Slang::ComPtr<IBlob> diagnostics;
        GraphicsPipelineSettings info;

        const std::string base_path = path.substr(0, path.find_last_of('.'));

        Slang::ComPtr module{vertex_session->loadModule(path.c_str(), diagnostics.writeRef())};
        if (TryLog(diagnostics)) return info;

        const ShaderData vertex_stage = CompileStage(module, base_path + ".vert", SLANG_STAGE_VERTEX, info);
        if (vertex != nullptr) *vertex = vertex_stage;

        module = Slang::ComPtr{fragment_session->loadModule(path.c_str(), diagnostics.writeRef())};
        if (TryLog(diagnostics)) return info;

        const ShaderData fragment_stage = CompileStage(module, base_path + ".frag", SLANG_STAGE_FRAGMENT, info);
        if (fragment != nullptr) *fragment = fragment_stage;

        return info;
    }
} // namespace ShaderCompiler