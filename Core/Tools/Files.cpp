#include "Files.hpp"

#include "Core/Rendering/Renderer.hpp"
#include "Tools/Logging.hpp"
#include "Tools/UUID.hpp"

#include <vector>
#include <fstream>
#include <string>

namespace Files
{
    BinaryReadStream::BinaryReadStream(const std::string& path)
    {
        data = ReadBinary(path);

        if (!data.empty()) is_initialized = true;
    }

    BinaryReadStream::BinaryReadStream(std::ifstream& input_stream, const usize count)
    {
        if (!input_stream.is_open())
        {
            Log::Error("Tried to create BinaryReadStream from un-opened file stream!");
            return;
        }

        if (!(input_stream.flags() | std::ios::binary))
        {
            Log::Error("Tried to create BinaryReadStream from non binary file stream!");
            return;
        }

        data.resize(count);
        input_stream.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(count));

        if (!data.empty()) is_initialized = true;
    }

    void BinaryReadStream::operator>>(std::string& value)
    {
        usize size = 0;
        Read(&size);

        value.resize(size);
        Read(value.data(), size);
    }

    void BinaryReadStream::operator>>(UUID& value)
    {
        std::string bytes;
        bytes.resize(sizeof(UUID));

        Read(bytes.data(), sizeof(UUID));

        value = UUID{bytes};
    }

    void BinaryReadStream::operator>>(float2& value)
    {
        Read(value.data());
        Read(value.data() + 1);
    }

    void BinaryReadStream::operator>>(float3& value)
    {
        Read(value.data());
        Read(value.data() + 1);
        Read(value.data() + 2);
    }

    void BinaryReadStream::operator>>(Vertex& value)
    {
        *this >> value.position;
        *this >> value.color;
        *this >> value.tex_coord;
    }


    void BinaryWriteStream::operator<<(const std::string& value)
    {
        const usize size = value.size();
        write(reinterpret_cast<const char*>(&size), sizeof(usize));

        write(value.data(), static_cast<std::streamsize>(size));
    }

    void BinaryWriteStream::operator<<(const UUID& value)
    {
        const std::string bytes = value.bytes();
        write(bytes.data(), sizeof(UUID));
    }

    void BinaryWriteStream::operator<<(const float2& value)
    {
        write(reinterpret_cast<const char*>(value.data()), sizeof(float3::Scalar));
        write(reinterpret_cast<const char*>(value.data() + 1), sizeof(float3::Scalar));
    }

    void BinaryWriteStream::operator<<(const float3& value)
    {
        write(reinterpret_cast<const char*>(value.data()), sizeof(float3::Scalar));
        write(reinterpret_cast<const char*>(value.data() + 1), sizeof(float3::Scalar));
        write(reinterpret_cast<const char*>(value.data() + 2), sizeof(float3::Scalar));
    }

    void BinaryWriteStream::operator<<(const Vertex& value)
    {
        *this << value.position;
        *this << value.color;
        *this << value.tex_coord;
    }

    void BinaryWriteStream::Write(const void* data, const usize count) { write(static_cast<const char*>(data), count); }

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