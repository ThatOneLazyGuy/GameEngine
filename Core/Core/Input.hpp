#pragma once

#include "Math.hpp"
#include "Tools/Types.hpp"

namespace Input
{
    using KeyType = uint64;

    // Scancodes taken from SDL3 scancodes, for documentation look here: https://wiki.libsdl.org/SDL3/SDL_Scancode
    enum Key : KeyType
    {
        UNKNOWN = 0,

        A = 4,
        B = 5,
        C = 6,
        D = 7,
        E = 8,
        F = 9,
        G = 10,
        H = 11,
        I = 12,
        J = 13,
        K = 14,
        L = 15,
        M = 16,
        N = 17,
        O = 18,
        P = 19,
        Q = 20,
        R = 21,
        S = 22,
        T = 23,
        U = 24,
        V = 25,
        W = 26,
        X = 27,
        Y = 28,
        Z = 29,

        NUM_1 = 30,
        NUM_2 = 31,
        NUM_3 = 32,
        NUM_4 = 33,
        NUM_5 = 34,
        NUM_6 = 35,
        NUM_7 = 36,
        NUM_8 = 37,
        NUM_9 = 38,
        NUM_0 = 39,

        RETURN = 40,
        ESCAPE = 41,
        BACKSPACE = 42,
        TAB = 43,
        SPACE = 44,

        MINUS = 45,
        EQUALS = 46,
        LEFTBRACKET = 47,
        RIGHTBRACKET = 48,
        BACKSLASH = 49,

        NONUSHASH = 50,
        SEMICOLON = 51,
        APOSTROPHE = 52,
        GRAVE = 53,
        COMMA = 54,
        PERIOD = 55,
        SLASH = 56,

        CAPSLOCK = 57,

        F1 = 58,
        F2 = 59,
        F3 = 60,
        F4 = 61,
        F5 = 62,
        F6 = 63,
        F7 = 64,
        F8 = 65,
        F9 = 66,
        F10 = 67,
        F11 = 68,
        F12 = 69,

        PRINTSCREEN = 70,
        SCROLLLOCK = 71,
        PAUSE = 72,
        INSERT = 73,
        HOME = 74,
        PAGEUP = 75,
        DELETE = 76,
        END = 77,
        PAGEDOWN = 78,
        RIGHT = 79,
        LEFT = 80,
        DOWN = 81,
        UP = 82,

        NUMLOCKCLEAR = 83,

        KEY_PAD_DIVIDE = 84,
        KEY_PAD_MULTIPLY = 85,
        KEY_PAD_MINUS = 86,
        KEY_PAD_PLUS = 87,
        KEY_PAD_ENTER = 88,
        KEY_PAD_1 = 89,
        KEY_PAD_2 = 90,
        KEY_PAD_3 = 91,
        KEY_PAD_4 = 92,
        KEY_PAD_5 = 93,
        KEY_PAD_6 = 94,
        KEY_PAD_7 = 95,
        KEY_PAD_8 = 96,
        KEY_PAD_9 = 97,
        KEY_PAD_0 = 98,
        KEY_PAD_PERIOD = 99,

        NONUSBACKSLASH = 100,
        APPLICATION = 101,
        POWER = 102,

        KEY_PAD_EQUALS = 103,
        F13 = 104,
        F14 = 105,
        F15 = 106,
        F16 = 107,
        F17 = 108,
        F18 = 109,
        F19 = 110,
        F20 = 111,
        F21 = 112,
        F22 = 113,
        F23 = 114,
        F24 = 115,
        EXECUTE = 116,
        HELP = 117,
        MENU = 118,
        SELECT = 119,
        STOP = 120,
        AGAIN = 121,
        UNDO = 122,
        CUT = 123,
        COPY = 124,
        PASTE = 125,
        FIND = 126,
        MUTE = 127,
        VOLUMEUP = 128,
        VOLUMEDOWN = 129,

        KEY_PAD_COMMA = 133,
        KEY_PAD_EQUALSAS400 = 134,

        INTERNATIONAL1 = 135,
        INTERNATIONAL2 = 136,
        INTERNATIONAL3 = 137,
        INTERNATIONAL4 = 138,
        INTERNATIONAL5 = 139,
        INTERNATIONAL6 = 140,
        INTERNATIONAL7 = 141,
        INTERNATIONAL8 = 142,
        INTERNATIONAL9 = 143,
        LANG1 = 144,
        LANG2 = 145,
        LANG3 = 146,
        LANG4 = 147,
        LANG5 = 148,
        LANG6 = 149,
        LANG7 = 150,
        LANG8 = 151,
        LANG9 = 152,

        ALTERASE = 153,
        SYSREQ = 154,
        CANCEL = 155,
        CLEAR = 156,
        PRIOR = 157,
        RETURN2 = 158,
        SEPARATOR = 159,
        OUT = 160,
        OPER = 161,
        CLEARAGAIN = 162,
        CRSEL = 163,
        EXSEL = 164,

        KEY_PAD_00 = 176,
        KEY_PAD_000 = 177,
        THOUSANDSSEPARATOR = 178,
        DECIMALSEPARATOR = 179,
        CURRENCYUNIT = 180,
        CURRENCYSUBUNIT = 181,
        KEY_PAD_LEFTPAREN = 182,
        KEY_PAD_RIGHTPAREN = 183,
        KEY_PAD_LEFTBRACE = 184,
        KEY_PAD_RIGHTBRACE = 185,
        KEY_PAD_TAB = 186,
        KEY_PAD_BACKSPACE = 187,
        KEY_PAD_A = 188,
        KEY_PAD_B = 189,
        KEY_PAD_C = 190,
        KEY_PAD_D = 191,
        KEY_PAD_E = 192,
        KEY_PAD_F = 193,
        KEY_PAD_XOR = 194,
        KEY_PAD_POWER = 195,
        KEY_PAD_PERCENT = 196,
        KEY_PAD_LESS = 197,
        KEY_PAD_GREATER = 198,
        KEY_PAD_AMPERSAND = 199,
        KEY_PAD_DBLAMPERSAND = 200,
        KEY_PAD_VERTICALBAR = 201,
        KEY_PAD_DBLVERTICALBAR = 202,
        KEY_PAD_COLON = 203,
        KEY_PAD_HASH = 204,
        KEY_PAD_SPACE = 205,
        KEY_PAD_AT = 206,
        KEY_PAD_EXCLAM = 207,
        KEY_PAD_MEMSTORE = 208,
        KEY_PAD_MEMRECALL = 209,
        KEY_PAD_MEMCLEAR = 210,
        KEY_PAD_MEMADD = 211,
        KEY_PAD_MEMSUBTRACT = 212,
        KEY_PAD_MEMMULTIPLY = 213,
        KEY_PAD_MEMDIVIDE = 214,
        KEY_PAD_PLUSMINUS = 215,
        KEY_PAD_CLEAR = 216,
        KEY_PAD_CLEARENTRY = 217,
        KEY_PAD_BINARY = 218,
        KEY_PAD_OCTAL = 219,
        KEY_PAD_DECIMAL = 220,
        KEY_PAD_HEXADECIMAL = 221,

        LCTRL = 224,
        LSHIFT = 225,
        LALT = 226,
        LGUI = 227,
        RCTRL = 228,
        RSHIFT = 229,
        RALT = 230,
        RGUI = 231,

        MODE = 257,

        SLEEP = 258,
        WAKE = 259,

        CHANNEL_INCREMENT = 260,
        CHANNEL_DECREMENT = 261,

        MEDIA_PLAY = 262,
        MEDIA_PAUSE = 263,
        MEDIA_RECORD = 264,
        MEDIA_FAST_FORWARD = 265,
        MEDIA_REWIND = 266,
        MEDIA_NEXT_TRACK = 267,
        MEDIA_PREVIOUS_TRACK = 268,
        MEDIA_STOP = 269,
        MEDIA_EJECT = 270,
        MEDIA_PLAY_PAUSE = 271,
        MEDIA_SELECT = 272,

        AC_NEW = 273,
        AC_OPEN = 274,
        AC_CLOSE = 275,
        AC_EXIT = 276,
        AC_SAVE = 277,
        AC_PRINT = 278,
        AC_PROPERTIES = 279,

        AC_SEARCH = 280,
        AC_HOME = 281,
        AC_BACK = 282,
        AC_FORWARD = 283,
        AC_STOP = 284,
        AC_REFRESH = 285,
        AC_BOOKMARKS = 286,

        SOFTLEFT = 287,
        SOFTRIGHT = 288,

        CALL = 289,
        ENDCALL = 290,

        RESERVED = 400,

        KEY_COUNT = 512
    };


    void SetKey(Key key, bool pressed);

    bool GetKeyPressed(Key key);
    bool GetKey(Key key);
    bool GetKeyReleased(Key key);

    void ClearKeys();


    void SetMousePos(float x, float y);
    void SetMousePos(const Vec2& pos);

    void SetMouseDelta(float x, float y);
    void SetMouseDelta(const Vec2& pos_delta);

    // Makes the mouse invisible and locks it in place, only mouse delta functions will be updated.
    void LockMouse(bool lock);
    bool IsMouseLocked();

    Vec2 GetMousePos();
    float GetMouseX();
    float GetMouseY();

    Vec2 GetMouseDeltaPos();
    float GetMouseDeltaX();
    float GetMouseDeltaY();

} // namespace Input