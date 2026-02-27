#pragma once

#include <uuid_v4/endianness.h>
#include <uuid_v4/uuid_v4.h>

using UUID = UUIDv4::UUID;

inline const UUID NULL_UUID{0, 0};

class UUIDGenerator : public UUIDv4::UUIDGenerator<std::mt19937_64> {
   public:
    UUIDGenerator() = default;
    UUIDGenerator(const uint64_t seed) : UUIDv4::UUIDGenerator<std::mt19937_64>(seed) {}

    static UUID Generate() {
        static UUIDGenerator generator;
        return generator.getUUID();
    }
};
