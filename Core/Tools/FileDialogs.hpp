#pragma once

#include <string>
#include <vector>

namespace FileDialogs
{
    [[nodiscard]] std::string OpenFile(const std::string& title, const std::string& default_path = "./", const std::vector<std::string>& filters = {});
}