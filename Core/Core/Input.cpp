#include "Input.hpp"

#include "Window.hpp"

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_rect.h>

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

        float2 mouse_pos_delta{};
        float2 mouse_pos{};
    } // namespace

    void SetKey(const Key key, const bool pressed)
    {
        KeyType& state = key_states[key >> key_shift_count];
        previous_key_states[key >> key_shift_count] = state;

        if (pressed) state |= (1ull << (key & key_lower_mask));
        else state &= ~(1ull << (key & key_lower_mask));
    }

    bool GetKeyPressed(const Key key) { return GetState(key_states, key) && !GetState(previous_key_states, key); }
    bool GetKey(const Key key) { return GetState(key_states, key); }
    bool GetKeyReleased(const Key key) { return !GetState(key_states, key) && GetState(previous_key_states, key); }

    void ClearKeys()
    {
        memset(previous_key_states, 0, sizeof(previous_key_states));
        memset(key_states, 0, sizeof(key_states));
    }


    void SetMousePos(const float x, const float y) { mouse_pos = float2{x, y}; }
    void SetMousePos(const float2& pos) { mouse_pos = pos; }

    void SetMouseDelta(const float x, const float y) { mouse_pos_delta = float2{x, y}; }
    void SetMouseDelta(const float2& pos_delta) { mouse_pos_delta = pos_delta; }

    void LockMouse(const bool lock)
    {
        SDL_Window* window = static_cast<SDL_Window*>(Window::GetHandle());

        if (lock)
        {
            const SDL_Rect rect{static_cast<sint32>(mouse_pos.x()), static_cast<sint32>(mouse_pos.y()), 1, 1};
            if (!SDL_SetWindowMouseRect(window, &rect)) SDL_Log("Failed to set window mouse rect", SDL_GetError());
        }
        else SDL_SetWindowMouseRect(window, nullptr);

        SDL_SetWindowRelativeMouseMode(window, lock);
    }
    bool IsMouseLocked()
    {
        SDL_Window* window = static_cast<SDL_Window*>(Window::GetHandle());
        return SDL_GetWindowRelativeMouseMode(window);
    }

    float2 GetMousePos() { return mouse_pos; }
    float GetMouseX() { return mouse_pos.x(); }
    float GetMouseY() { return mouse_pos.y(); }

    float2 GetMouseDeltaPos() { return mouse_pos_delta; }
    float GetMouseDeltaX() { return mouse_pos_delta.x(); }
    float GetMouseDeltaY() { return mouse_pos_delta.y(); }
} // namespace Input