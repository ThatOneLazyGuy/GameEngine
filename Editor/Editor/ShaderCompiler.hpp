#pragma once

#include <string>

#include <core/Rendering/Renderer.hpp>

namespace ShaderCompiler
{
	void Init();

	GraphicsPipelineSettings CompileGraphicsShaders(const std::string& path);
}