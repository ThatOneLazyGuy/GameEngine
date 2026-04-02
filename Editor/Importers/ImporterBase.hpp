#pragma once

#include "Editor.hpp"

#include <ResourceManager.hpp>

#include <string>
#include <string_view>
#include <vector>

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

    // The import function that generates the asset files that will be loaded by the engine, should return uuids of the imported assets or an empty array on failure.
    [[nodiscard]] virtual std::vector<UUID> ImportAssets(const std::string& path) = 0;

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