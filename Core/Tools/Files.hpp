#pragma once

#include "Types.hpp"

#include <vector>
#include <string>
#include <span>

namespace Files
{
    std::vector<uint8> ReadBinary(const std::string& path, bool log_failure = true);
    std::string ReadText(const std::string& path, bool log_failure = true);

    bool WriteBinary(const std::string& path, const std::span<const uint8>& data, bool append = false, bool log_failure = true);
    bool WriteText(const std::string& path, const std::string_view& text, bool append = false, bool log_failure = true);
} // namespace Files