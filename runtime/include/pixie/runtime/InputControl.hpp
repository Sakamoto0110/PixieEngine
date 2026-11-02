#pragma once

#include "pixie/platform/Input.hpp"

namespace pixie {

// Entity opt-in for keyboard delivery. Queries explicitly use the current callback/Update input,
// never retain a host pointer, and return neutral state while this component is disabled.
struct KeyboardControl {
    bool enabled = true;
    bool IsDown(const Input& input, Key key) const { return enabled && input.IsDown(key); }
    bool WasPressed(const Input& input, Key key) const { return enabled && input.WasPressed(key); }
    bool WasReleased(const Input& input, Key key) const { return enabled && input.WasReleased(key); }
    InputModifiers Modifiers(const Input& input) const { return enabled ? input.Modifiers() : InputModifiers::None; }
};

// Cursor coordinates are host viewport pixels, not inferred world coordinates or game actions.
struct MouseControl {
    bool enabled = true;
    bool IsDown(const Input& input, MouseButton button) const { return enabled && input.IsDown(button); }
    bool WasPressed(const Input& input, MouseButton button) const { return enabled && input.WasPressed(button); }
    bool WasReleased(const Input& input, MouseButton button) const { return enabled && input.WasReleased(button); }
    pxPoint Position(const Input& input) const { return enabled ? input.Cursor() : pxPoint(0.0, 0.0); }
    pxVec2 Movement(const Input& input) const { return enabled ? input.CursorMoved() : pxVec2(0.0); }
    pxVec2 Scroll(const Input& input) const { return enabled ? input.Scroll() : pxVec2(0.0); }
    InputModifiers Modifiers(const Input& input) const { return enabled ? input.Modifiers() : InputModifiers::None; }
};

} // namespace pixie
