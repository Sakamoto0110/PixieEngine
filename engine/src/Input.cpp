#include "pixie/Input.hpp"

namespace pixie {

namespace {

template<typename States>
std::uint8_t StateOf(const States& states, int code) {
    return code >= 0 && code < static_cast<int>(states.size()) ? states[static_cast<std::size_t>(code)]
                                                                : std::uint8_t{0};
}

} // namespace

void Input::Set(std::uint8_t& state, bool down) {
    const bool wasDown = (state & kDown) != 0;
    if (down && !wasDown) {
        state |= kDown | kPressed;
    } else if (!down && wasDown) {
        state = static_cast<std::uint8_t>((state & ~kDown) | kReleased);
    }
}

void Input::SetKey(int code, bool down) {
    if (code >= 0 && code < kKeyCount) {
        Set(keys_[static_cast<std::size_t>(code)], down);
    }
}

void Input::SetMouseButton(int button, bool down) {
    if (button >= 0 && button < kButtonCount) {
        Set(buttons_[static_cast<std::size_t>(button)], down);
    }
}

void Input::SetCursor(pxPoint position) {
    cursor_ = position;
}

void Input::AddScroll(pxVec2 offset) {
    scroll_ += offset;
}

void Input::EndUpdate() {
    for (auto& state : keys_) {
        state &= kDown;
    }
    for (auto& state : buttons_) {
        state &= kDown;
    }
    scroll_ = pxVec2(0.0);
    cursorAtLastUpdate_ = cursor_;
}

bool Input::IsDown(Key key) const {
    return (StateOf(keys_, static_cast<int>(key)) & kDown) != 0;
}

bool Input::WasPressed(Key key) const {
    return (StateOf(keys_, static_cast<int>(key)) & kPressed) != 0;
}

bool Input::WasReleased(Key key) const {
    return (StateOf(keys_, static_cast<int>(key)) & kReleased) != 0;
}

bool Input::IsDown(MouseButton button) const {
    return (StateOf(buttons_, static_cast<int>(button)) & kDown) != 0;
}

bool Input::WasPressed(MouseButton button) const {
    return (StateOf(buttons_, static_cast<int>(button)) & kPressed) != 0;
}

bool Input::WasReleased(MouseButton button) const {
    return (StateOf(buttons_, static_cast<int>(button)) & kReleased) != 0;
}

} // namespace pixie
