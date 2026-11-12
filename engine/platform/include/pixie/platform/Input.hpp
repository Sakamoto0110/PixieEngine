#pragma once

#include "pixie/platform/Export.hpp"

#include <pixie/math/pxVec.hpp>
#include <pixie/pxPoint.hpp>

#include <array>
#include <cstdint>
#include <span>
#include <variant>
#include <vector>

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
    LeftSuper = 343,
    RightShift = 344,
    RightControl = 345,
    RightAlt = 346,
    RightSuper = 347,
};

// Mouse buttons, with GLFW's values.
enum class MouseButton : int { Left = 0, Right = 1, Middle = 2, Button4 = 3, Button5, Button6, Button7, Button8 };

// Values match GLFW actions/modifier bits; no game actions or coordinate-space inference.
enum class InputAction : int { Release = 0, Press = 1, Repeat = 2 };
enum class InputModifiers : std::uint32_t { None = 0, Shift = 1, Control = 2, Alt = 4, Super = 8, CapsLock = 16, NumLock = 32 };
constexpr InputModifiers operator|(InputModifiers a, InputModifiers b) {
    return static_cast<InputModifiers>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}
constexpr bool HasModifier(InputModifiers value, InputModifiers flag) {
    return (static_cast<std::uint32_t>(value) & static_cast<std::uint32_t>(flag)) != 0;
}
struct KeyEvent {
    Key key;
    InputAction action;
    InputModifiers modifiers;
    int scanCode = 0;
    bool Pressed() const { return action == InputAction::Press; }
    bool Released() const { return action == InputAction::Release; }
    bool Repeat() const { return action == InputAction::Repeat; }
};
struct MouseButtonEvent {
    MouseButton button;
    InputAction action;
    InputModifiers modifiers;
    pxPoint position;
    bool Pressed() const { return action == InputAction::Press; }
    bool Released() const { return action == InputAction::Release; }
};
struct MouseMoveEvent { pxPoint position; pxVec2 delta; InputModifiers modifiers; };
struct MouseScrollEvent { pxVec2 offset; pxPoint position; InputModifiers modifiers; };
using InputEvent = std::variant<KeyEvent, MouseButtonEvent, MouseMoveEvent, MouseScrollEvent>;

/// Keyboard and pointer state owned by the driving host thread.
/// Edges, movement and scroll accumulate until the host consumes them with EndUpdate.
class Input {
public:
    static constexpr int kKeyCount = 512;
    static constexpr int kButtonCount = 8;

    /// Feed events in arrival order. Codes outside the storage range are ignored.
    /// Bool overloads infer modifiers. Action overloads preserve host modifiers.
    PIXIE_PLATFORM_API void SetKey(int code, bool down);
    PIXIE_PLATFORM_API void SetKey(int code, InputAction action, InputModifiers modifiers, int scanCode = 0);
    PIXIE_PLATFORM_API void SetMouseButton(int button, bool down);
    PIXIE_PLATFORM_API void SetMouseButton(int button, InputAction action, InputModifiers modifiers);
    // The cursor in framebuffer pixels, origin at the top-left corner, Y down.
    PIXIE_PLATFORM_API void SetCursor(pxPoint position);
    PIXIE_PLATFORM_API void SetCursor(pxPoint position, InputModifiers modifiers);
    PIXIE_PLATFORM_API void AddScroll(pxVec2 offset);
    PIXIE_PLATFORM_API void AddScroll(pxVec2 offset, InputModifiers modifiers);

    /// Borrows ordered events until the host mutates Input. Do not mutate during dispatch.
    /// Polling describes the final accumulated state; events preserve intermediate transitions.
    std::span<const InputEvent> Events() const { return events_; }
    InputModifiers Modifiers() const { return modifiers_; }

    // Called after each fixed update: forgets the presses, releases, scroll, and movement it saw.
    PIXIE_PLATFORM_API void EndUpdate();

    // A modal host UI or focus loss cancels held controls. Report their releases once, discard
    // pending presses/movement/scroll, and keep the current cursor position.
    PIXIE_PLATFORM_API void ReleaseAll();

    PIXIE_PLATFORM_API bool IsDown(Key key) const;
    // Pressed at least once since the previous update, even if released again.
    PIXIE_PLATFORM_API bool WasPressed(Key key) const;
    PIXIE_PLATFORM_API bool WasReleased(Key key) const;

    PIXIE_PLATFORM_API bool IsDown(MouseButton button) const;
    PIXIE_PLATFORM_API bool WasPressed(MouseButton button) const;
    PIXIE_PLATFORM_API bool WasReleased(MouseButton button) const;

    // Where the cursor is now, in framebuffer pixels.
    pxPoint Cursor() const { return cursor_; }
    // How far the cursor moved since the previous update, in framebuffer pixels.
    pxVec2 CursorMoved() const { return cursor_ - cursorAtLastUpdate_; }
    // Scroll since the previous update; positive Y is away from the user.
    pxVec2 Scroll() const { return scroll_; }

private:
    enum : std::uint8_t { kDown = 1, kPressed = 2, kReleased = 4, kCancelled = 8 };

    PIXIE_PLATFORM_API static void Set(std::uint8_t& state, bool down);

    PIXIE_PLATFORM_API InputModifiers HeldModifiers() const;
    std::vector<InputEvent> events_;
    InputModifiers modifiers_ = InputModifiers::None;
    std::array<std::uint8_t, kKeyCount> keys_{};
    std::array<std::uint8_t, kButtonCount> buttons_{};
    pxPoint cursor_;
    pxPoint cursorAtLastUpdate_;
    pxVec2 scroll_;
};

} // namespace pixie
