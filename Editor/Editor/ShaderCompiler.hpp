#pragma once

#include <string>

namespace ShaderCompiler
{
	void Init();

	void CompileShader(const std::string& path);
}