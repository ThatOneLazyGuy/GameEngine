#pragma once

#include <string>
#include <vector>

struct ShaderSettings;

namespace ShaderCompiler
{
	struct ShaderInfo
	{

	};

	void Init();

	std::vector<ShaderSettings> CompileShaders(const std::string& path);
}