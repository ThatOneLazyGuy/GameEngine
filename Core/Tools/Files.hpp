#pragma once

#include "Tools/Types.hpp"
#include "Core/Math.hpp"

#include <fstream>
#include <vector>
#include <string>
#include <span>
#include <map>


namespace UUIDv4
{
    class UUID;
}

using UUID = UUIDv4::UUID;
struct Vertex;

namespace Files
{
    class BinaryReadStream
    {
      public:
        BinaryReadStream() = default;
        BinaryReadStream(const std::string& path);
        BinaryReadStream(std::ifstream& input_stream, usize count);

        template <typename Type>
        requires(std::is_arithmetic_v<Type>)
        void operator>>(Type& value)
        {
            Read(&value);
        }

        template <typename Type>
        requires std::is_trivially_copyable_v<Type>
        void operator>>(std::vector<Type>& value)
        {
            usize size = 0;
            Read(&size);

            value.resize(size);
            Read(value.data(), size * sizeof(Type));
        }

        template <typename Type>
        void operator>>(std::vector<Type>& value)
        {
            usize size = 0;
            Read(&size);

            value.resize(size);
            for (usize i = 0; i < size; i++)
            {
                *this >> value[i];
            }
        }

        template <typename Key, typename Type>
        void operator>>(std::map<Key, Type>& value)
        {
            usize size = 0;
            Read(&size);

            for (usize i = 0; i < size; i++)
            {
                Key key;
                *this >> key;

                Type type;
                *this >> type;

                value.emplace(std::move(key), std::move(type));
            }
        }

        void operator>>(std::string& value);
        void operator>>(UUID& value);

        void operator>>(float2& value);
        void operator>>(float3& value);
        void operator>>(Vertex& value);

        [[nodiscard]] bool IsInitialized() const { return is_initialized; }

        [[nodiscard]] usize Size() const { return data.size(); }
        [[nodiscard]] const uint8* Data() const { return data.data(); }

        [[nodiscard]] usize GetReadPosition() const { return read_position; }
        void SetReadPosition(const usize position) { read_position = position; }

      private:
        void Read(void* destination, const usize count)
        {
            std::memcpy(destination, data.data() + read_position, count);
            read_position += count;
        }

        template <typename Type>
        void Read(Type* destination)
        {
            std::memcpy(destination, data.data() + read_position, sizeof(Type));
            read_position += sizeof(Type);
        }

        std::vector<uint8> data;
        usize read_position{0};

        bool is_initialized{false};
    };

    class BinaryWriteStream : protected std::ofstream
    {
      public:
        BinaryWriteStream() = default;
        BinaryWriteStream(const std::string& file, const bool truncate = true) :
            std::ofstream{file, std::ios::binary | (truncate ? std::ios::trunc : 0)}
        {
        }

        [[nodiscard]] bool IsOpen() const { return is_open(); }

        template <typename Type>
        requires(std::is_arithmetic_v<Type>)
        void operator<<(const Type& value)
        {
            write(reinterpret_cast<const char*>(&value), sizeof(Type));
        }

        template <typename Type>
        requires(std::is_trivially_copyable_v<Type>)
        void operator<<(const std::vector<Type>& value)
        {
            const usize size = value.size();
            write(reinterpret_cast<const char*>(&size), sizeof(usize));

            write(reinterpret_cast<const char*>(value.data()), size * sizeof(Type));
        }

        template <typename Type>
        void operator<<(const std::vector<Type>& value)
        {
            const usize size = value.size();
            write(reinterpret_cast<const char*>(&size), sizeof(usize));

            for (const Type& type : value)
            {
                *this << type;
            }
        }

        template <typename Key, typename Type>
        void operator<<(const std::map<Key, Type>& value)
        {
            const usize size = value.size();
            write(reinterpret_cast<const char*>(&size), sizeof(usize));

            for (const auto&& [key, type] : value)
            {
                *this << key;
                *this << type;
            }
        }

        void operator<<(const std::string& value);
        void operator<<(const UUID& value);

        void operator<<(const float2& value);
        void operator<<(const float3& value);
        void operator<<(const Vertex& value);
    };

    std::vector<uint8> ReadBinary(const std::string& path, bool log_failure = true);
    std::string ReadText(const std::string& path, bool log_failure = true);

    bool WriteBinary(const std::string& path, const std::span<const uint8>& data, bool append = false, bool log_failure = true);
    bool WriteText(const std::string& path, const std::string_view& text, bool append = false, bool log_failure = true);
} // namespace Files