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
    class BinaryReadStream : protected std::ifstream
    {
      public:
        BinaryReadStream() = default;
        BinaryReadStream(const std::string& file) : std::ifstream{file, std::ios::binary} {}

        [[nodiscard]] bool IsOpen() const { return is_open(); }

        template <typename Type>
        requires(std::is_arithmetic_v<Type>)
        void operator>>(Type& value)
        {
            read(reinterpret_cast<char*>(&value), sizeof(Type));
        }

        template <typename Type>
        requires std::is_trivially_copyable_v<Type>
        void operator>>(std::vector<Type>& value)
        {
            usize size = 0;
            read(reinterpret_cast<char*>(&size), sizeof(usize));

            value.resize(size);
            read(reinterpret_cast<char*>(value.data()), size * sizeof(Type));
        }

        template <typename Type>
        void operator>>(std::vector<Type>& value)
        {
            usize size = 0;
            read(reinterpret_cast<char*>(&size), sizeof(usize));

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
            read(reinterpret_cast<char*>(&size), sizeof(usize));

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

    class OutputStream
    {
    };

    std::vector<uint8> ReadBinary(const std::string& path, bool log_failure = true);
    std::string ReadText(const std::string& path, bool log_failure = true);

    bool WriteBinary(const std::string& path, const std::span<const uint8>& data, bool append = false, bool log_failure = true);
    bool WriteText(const std::string& path, const std::string_view& text, bool append = false, bool log_failure = true);
} // namespace Files