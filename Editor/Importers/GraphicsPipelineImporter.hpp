#pragma once

#include "ImporterBase.hpp"

class GraphicsPipelineImporter : public Importer<GraphicsPipelineImporter, "Graphics Shader Pipeline (*.slang)", "*.slang">
{
  public:
    GraphicsPipelineImporter() = default;

    std::vector<UUID> ImportAssets(const std::string& path) override;
};