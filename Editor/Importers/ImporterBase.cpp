#include "ImporterBase.hpp"

#include "Editor.hpp"

ImporterInfo& ImporterBase::AddImporter(
    const ImporterCreator creator_function, const std::string_view& description, const std::string_view& file_types
)
{
    const ImporterInfo info{
        .creator_function = creator_function,
        .description = description,
        .file_types = file_types,
    };

    const auto iterator = std::ranges::lower_bound(importer_infos, info.file_types, {}, &ImporterInfo::file_types);

    return *importer_infos.insert(iterator, info);
}