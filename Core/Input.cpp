#include "Input.hpp"

#include "Logging.hpp"
#include "Window.hpp"

#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_rect.h>

namespace Input
{
    namespace
    {
        constexpr usize KEY_TYPE_BIT_COUNT{std::numeric_limits<KeyType>::digits};
        constexpr usize KEY_SHIFT_COUNT{static_cast<usize>(std::countr_zero(KEY_TYPE_BIT_COUNT))};

        KeyType previous_key_states[KEY_COUNT >> KEY_SHIFT_COUNT];
        KeyType key_states[KEY_COUNT >> KEY_SHIFT_COUNT];

        constexpr KeyType GetKeyBit(const KeyType key) { return 1 << (key & (KEY_TYPE_BIT_COUNT - 1)); }

        bool GetState(const KeyType states[], const Key key) { return states[key >> KEY_SHIFT_COUNT] & GetKeyBit(key); }

        float2 mouse_pos_delta{};
        float2 mouse_pos{};
    } // namespace

    void SetKey(const Key key, const bool pressed)
    {
        KeyType& state = key_states[key >> KEY_SHIFT_COUNT];
        previous_key_states[key >> KEY_SHIFT_COUNT] = state;

        if (pressed) state |= GetKeyBit(key);
        else state &= ~GetKeyBit(key);
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
            if (!SDL_SetWindowMouseRect(window, &rect)) Log::Error("Failed to set window mouse rect: {}", SDL_GetError());
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