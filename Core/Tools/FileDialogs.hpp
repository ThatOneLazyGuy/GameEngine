#pragma once

#include <string>
#include <vector>

namespace FileDialogs
{
    // Open a file dialog to select a single file.
    [[nodiscard]] std::string OpenFile(const std::string& title, const std::string& default_path = "./", const std::vector<std::string>& filters = {});

    // Open a file dialog with the ability to select multiple files.
    [[nodiscard]] std::vector<std::string> OpenFiles(const std::string& title, const std::string& default_path = "./", const std::vector<std::string>& filters = {});
}