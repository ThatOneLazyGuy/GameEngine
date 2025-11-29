#include "Files.hpp"

#include "Logging.hpp"

#include <vector>
#include <fstream>
#include <string>

namespace Files
{
    std::vector<uint8> ReadBinary(const std::string& path, bool log_failure)
    {
        std::vector<uint8> data;

        std::ifstream file{path, std::ios::ate | std::ios::binary};
        if (!file.is_open())
        {
            if (log_failure) Log::Error("Failed to open binary file for read: {}", path);
            return data;
        }

        const std::streamsize size = file.tellg();
        data.resize(size);

        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(data.data()), size);
        file.close();

        return data;
    }

    std::string ReadText(const std::string& path, bool log_failure)
    {
        std::string text;

        std::ifstream file{path, std::ios::ate};
        if (!file.is_open())
        {
            if (log_failure) Log::Error("Failed to open text file for read: {}", path);
            return text;
        }

        const std::streamsize size = file.tellg();
        text.resize(size);

        file.seekg(0, std::ios::beg);
        file.read(text.data(), size);
        file.close();

        return text;
    }

    bool WriteBinary(const std::string& path, const std::span<const uint8>& data, bool append, bool log_failure)
    {
        std::ofstream file{path, std::ios::binary | (append ? 0 : std::ios::trunc)};
        if (!file.is_open())
        {
            if (log_failure) Log::Error("Failed to open binary file for write: {}", path);
            return false;
        }

        file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));

        return true;
    }

    bool WriteText(const std::string& path, const std::string_view& text, bool append, bool log_failure)
    {
        std::ofstream file{path, (append ? 0 : std::ios::trunc)};
        if (!file.is_open())
        {
            if (log_failure) Log::Error("Failed to open text file for write: {}", path);
            return false;
        }

        file.write(text.data(), static_cast<std::streamsize>(text.size()));

        return true;
    }
} // namespace Files