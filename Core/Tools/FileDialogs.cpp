#include "FileDialogs.hpp"

#include <portable-file-dialogs/portable-file-dialogs.h>

#include <filesystem>

namespace FileDialogs
{
    std::string OpenFile(const std::string& title, const std::string& default_path, const std::vector<std::string>& filters)
    {
        const std::string full_default_path = std::filesystem::absolute(default_path).generic_string();
        const auto result = pfd::open_file{title, full_default_path, filters}.result();

        if (result.empty()) return {};

        return std::filesystem::relative(result.front()).generic_string();
    }
} // namespace FileDialogs