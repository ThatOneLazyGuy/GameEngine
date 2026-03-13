#pragma once

#include "ImporterBase.hpp"

class ObjImporter : public Importer<ObjImporter, "Wavefront Object (*.obj)", "*.obj">
{
  public:
    ObjImporter() = default;

    void ImportAsset(const std::string& path) override;
};