#include "ImporterBase.hpp"

void ImporterBase::Import(const std::string& path)
{
    const std::string extension = path.substr(path.find_last_of('.'));
    if (extension.empty())
    {
        Log::Error("Failed to import file, invalid extension: {}", path);
        return;
    }

    const auto iterator = std::ranges::find_if(importer_infos, [&extension](const ImporterInfo& element) {
        return element.file_types.find(extension) != std::string::npos;
    });

    if (iterator == importer_infos.end())
    {
        Log::Error("Failed to import file, no importer for file extension: {}", extension);
        return;
    }

    ImporterBase* importer = iterator->creator_function();

    importer->ImportAsset(path);

    delete importer;
}

ImporterInfo& ImporterBase::AddImporter(
    const ImporterCreator creator_function, const std::string_view& description, const std::string_view& file_types
)
{
    const ImporterInfo info{
        .creator_function = creator_function,
        .description = description,
        .file_types = file_types,
    };

    const auto iterator = std::ranges::lower_bound(importer_infos, info, [](const ImporterInfo& first, const ImporterInfo& second) {
        return first.file_types < second.file_types;
    });

    return *importer_infos.insert(iterator, info);
}