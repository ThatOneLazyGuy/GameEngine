#include "GraphicsPipelineImporter.hpp"

#include "Editor.hpp"
#include "ShaderCompiler.hpp"

#include <Rendering/Renderer.hpp>

#include <string>

bool GraphicsPipelineImporter::ImportAsset(const std::string& path)
{
    ShaderCompiler::ShaderData vertex_data;
    ShaderCompiler::ShaderData fragment_data;

    const GraphicsPipelineSettings settings = ShaderCompiler::CompileGraphicsShaders(path, &vertex_data, &fragment_data);

    const usize extension_position = path.find_last_of('.');
    const std::string base_path = path.substr(0, extension_position);

    const std::string shader_path = base_path + ".shader";
    Files::BinaryWriteStream file{shader_path};
    if (!file.IsOpen())
    {
        Log::Error("Failed to import graphics pipeline, couldn't open file: {}", path);
        return false;
    }

    Editor::asset_registry->RegisterPath<GraphicsShaderPipeline>(shader_path);

    const UUID vertex_uuid = Editor::asset_registry->RegisterPath<Shader>(vertex_data.shader_path);

    Files::BinaryWriteStream vertex_file{vertex_data.shader_path};
    if (vertex_file.IsOpen())
    {
        vertex_file << settings.vertex_info.type;
        vertex_file << settings.vertex_info.sampler_count;
        vertex_file << settings.vertex_info.storage_count;
        vertex_file << settings.vertex_info.uniform_count;

        vertex_file << vertex_data.data;
    }

    const UUID fragment_uuid = Editor::asset_registry->RegisterPath<Shader>(fragment_data.shader_path);

    Files::BinaryWriteStream fragment_file{fragment_data.shader_path};
    if (fragment_file.IsOpen())
    {
        fragment_file << settings.fragment_info.type;
        fragment_file << settings.fragment_info.sampler_count;
        fragment_file << settings.fragment_info.storage_count;
        fragment_file << settings.fragment_info.uniform_count;

        fragment_file << fragment_data.data;
    }

    file << settings.vertex_attributes.size();
    for (const VertexAttribute& attribute : settings.vertex_attributes)
    {
        file << attribute.semantic_name;

        file << attribute.element_type;
        file << attribute.element_count;
    }

    file << settings.uniform_sizes.size();
    for (const auto& [name, size] : settings.uniform_sizes)
    {
        file << name;
        file << size;
    }

    file << vertex_uuid;
    file << fragment_uuid;

    return true;
}