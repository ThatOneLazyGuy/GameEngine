#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "Core/ResourceManager.hpp"

class ImporterBase;
using ImporterCreator = ImporterBase* (*)();

struct ImporterInfo
{
    ImporterCreator creator_function;
    std::string_view description;
    std::string_view file_types;
};

class ImporterBase
{
  public:
    virtual ~ImporterBase() = default;

    [[nodiscard]] static const std::vector<ImporterInfo>& GetImporterInfos() { return importer_infos; }
    static void Import(const std::string& path);

    virtual void ImportAsset(const std::string& path) = 0;

  private:
    template <typename Derived, TemplateString Description, TemplateString FileTypes>
    friend class Importer;

    ImporterBase() = default;

    static ImporterInfo& AddImporter(
        ImporterCreator creator_function, const std::string_view& description, const std::string_view& file_types
    );


    inline static std::vector<ImporterInfo> importer_infos;
};

template <typename Derived, TemplateString Description, TemplateString FileTypes>
class Importer : public ImporterBase
{
    static ImporterBase* ImporterCreator() { return new Derived{}; }

  public:
    ~Importer() override = default;

    inline static const ImporterInfo& info = ImporterBase::AddImporter(&ImporterCreator, Description, FileTypes);

  private:
    friend Derived;
    friend class ImporterBase;

    Importer() = default;
};

class ObjImporter : public Importer<ObjImporter, "Wavefront Object (*.obj)", "*.obj">
{
  public:
    ObjImporter() = default;

    void ImportAsset(const std::string& path) override;
};

class GraphicsPipelineImporter : public Importer<GraphicsPipelineImporter, "Graphics Shader Pipeline (*.slang)", "*.slang">
{
  public:
    GraphicsPipelineImporter() = default;

    void ImportAsset(const std::string& path) override;
};