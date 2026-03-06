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

    std::vector<std::string> OpenFiles(const std::string& title, const std::string& default_path, const std::vector<std::string>& filters)
    {
        const std::string full_default_path = std::filesystem::absolute(default_path).generic_string();
        auto result = pfd::open_file{title, full_default_path, filters, pfd::opt::multiselect}.result();

        if (result.empty()) return {};

        // Make each selected file path relative.
        for (std::string& path : result)
        {
            path = std::filesystem::relative(path).generic_string();
        }

        return result;
    }
} // namespace FileDialogs