#pragma once

#include <string>

#include <Rendering/Renderer.hpp>

namespace ShaderCompiler
{
    struct ShaderData
    {
        std::string shader_path;
        std::vector<uint8> data;
    };

    void Init();

    GraphicsPipelineSettings CompileGraphicsShaders(const std::string& path, ShaderData* vertex = nullptr, ShaderData* fragment = nullptr);
} // namespace ShaderCompiler