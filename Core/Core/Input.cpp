#include "Input.hpp"

#include "Math.hpp"
#include "Tools/Types.hpp"

namespace Input
{
    namespace
    {
        constexpr size key_type_bit_count{std::numeric_limits<KeyType>::digits};
        constexpr size key_lower_mask{key_type_bit_count - 1};
        constexpr size key_shift_count{6ull}; // How many bits to shift to divide by key_type_bit_count.

        KeyType previous_key_states[KEY_COUNT >> key_shift_count];
        KeyType key_states[KEY_COUNT >> key_shift_count];

        bool GetState(const KeyType* key_states, const Key key)
        {
            return key_states[key >> key_shift_count] & (1ull << (key & key_lower_mask));
        }
    } // namespace

    void SetKey(const Key key, const bool pressed)
    {
        uint64& state = key_states[key >> key_shift_count];
        previous_key_states[key >> key_shift_count] = state;

        if (pressed) state |= (1ull << (key & key_lower_mask));
        else state &= ~(1ull << (key & key_lower_mask));
    }

    bool GetKeyPressed(const Key key) { return GetState(key_states, key) && !GetState(previous_key_states, key); }
    bool GetKey(const Key key) { return GetState(key_states, key); }
    bool GetKeyReleased(const Key key) { return !GetState(key_states, key) && GetState(previous_key_states, key); }
} // namespace Input