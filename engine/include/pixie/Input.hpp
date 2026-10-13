#pragma once

#include <pixie/math/pxVec.hpp>
#include <pixie/pxPoint.hpp>

#include <array>
#include <cstdint>

namespace pixie {

// Key codes. The values are GLFW's, so a host built on GLFW passes its codes through unchanged.
enum class Key : int {
    Space = 32,
    Apostrophe = 39,
    Comma = 44,
    Minus = 45,
    Period = 46,
    Slash = 47,
    Num0 = 48, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    Semicolon = 59,
    Equal = 61,
    A = 65, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    LeftBracket = 91,
    Backslash = 92,
    RightBracket = 93,
    GraveAccent = 96,
    Escape = 256,
    Enter = 257,
    Tab = 258,
    Backspace = 259,
    Insert = 260,
    Delete = 261,
    Right = 262,
    Left = 263,
    Down = 264,
    Up = 265,
    PageUp = 266,
    PageDown = 267,
    Home = 268,
    End = 269,
    F1 = 290, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    KeypadSubtract = 333,
    KeypadAdd = 334,
    KeypadEnter = 335,
    LeftShift = 340,
    LeftControl = 341,
    LeftAlt = 342,
    RightShift = 344,
    RightControl = 345,
    RightAlt = 346,
};

// Mouse buttons, with GLFW's values.
enum class MouseButton : int { Left = 0, Right = 1, Middle = 2 };

// The state of the keyboard and the mouse, fed by a host's events and read by an App.
//
// A press or release is kept until the next fixed update has seen it, so one that happens between
// two updates is not lost, and one that happens while several updates run in a frame is seen by the
// first of them only. Scroll and cursor movement accumulate the same way.
class Input {
public:
    static constexpr int kKeyCount = 512;
    static constexpr int kButtonCount = 8;

    // Host side: report events as they arrive. Codes outside the known range are ignored.
    void SetKey(int code, bool down);
    void SetMouseButton(int button, bool down);
    // The cursor in framebuffer pixels, origin at the top-left corner, Y down.
    void SetCursor(pxPoint position);
    void AddScroll(pxVec2 offset);

    // Called after each fixed update: forgets the presses, releases, scroll, and movement it saw.
    void EndUpdate();

    bool IsDown(Key key) const;
    // Pressed at least once since the previous update, even if released again.
    bool WasPressed(Key key) const;
    bool WasReleased(Key key) const;

    bool IsDown(MouseButton button) const;
    bool WasPressed(MouseButton button) const;
    bool WasReleased(MouseButton button) const;

    // Where the cursor is now, in framebuffer pixels.
    pxPoint Cursor() const { return cursor_; }
    // How far the cursor moved since the previous update, in framebuffer pixels.
    pxVec2 CursorMoved() const { return cursor_ - cursorAtLastUpdate_; }
    // Scroll since the previous update; positive Y is away from the user.
    pxVec2 Scroll() const { return scroll_; }

private:
    enum : std::uint8_t { kDown = 1, kPressed = 2, kReleased = 4 };

    static void Set(std::uint8_t& state, bool down);

    std::array<std::uint8_t, kKeyCount> keys_{};
    std::array<std::uint8_t, kButtonCount> buttons_{};
    pxPoint cursor_;
    pxPoint cursorAtLastUpdate_;
    pxVec2 scroll_;
};

} // namespace pixie
