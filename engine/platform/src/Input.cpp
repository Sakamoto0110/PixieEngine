#include "pixie/platform/Input.hpp"

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

InputModifiers Input::HeldModifiers() const {
    auto value = static_cast<std::uint32_t>(modifiers_) & 48u;
    if (IsDown(Key::LeftShift) || IsDown(Key::RightShift)) value |= 1;
    if (IsDown(Key::LeftControl) || IsDown(Key::RightControl)) value |= 2;
    if (IsDown(Key::LeftAlt) || IsDown(Key::RightAlt)) value |= 4;
    if (IsDown(Key::LeftSuper) || IsDown(Key::RightSuper)) value |= 8;
    return static_cast<InputModifiers>(value);
}

void Input::SetKey(int code, bool down) {
    if (code < 0 || code >= kKeyCount) return;
    const auto previous = keys_[static_cast<std::size_t>(code)];
    Set(keys_[static_cast<std::size_t>(code)], down);
    if (((previous & kDown) != 0) == down) return;
    modifiers_ = HeldModifiers();
    events_.emplace_back(KeyEvent{static_cast<Key>(code), down ? InputAction::Press : InputAction::Release, modifiers_, 0});
}

void Input::SetKey(int code, InputAction action, InputModifiers modifiers, int scanCode) {
    if (code < 0 || code >= kKeyCount || (action != InputAction::Release && action != InputAction::Press && action != InputAction::Repeat)) return;
    auto& state = keys_[static_cast<std::size_t>(code)];
    if ((action == InputAction::Repeat && (state & kDown) == 0)
        || (action == InputAction::Press && (state & kDown) != 0)
        || (action == InputAction::Release && (state & kDown) == 0)) return;
    modifiers_ = modifiers;
    if (action != InputAction::Repeat) Set(state, action == InputAction::Press);
    events_.emplace_back(KeyEvent{static_cast<Key>(code), action, modifiers, scanCode});
}

void Input::SetMouseButton(int button, bool down) {
    if (button < 0 || button >= kButtonCount) return;
    const bool previous = (buttons_[static_cast<std::size_t>(button)] & kDown) != 0;
    if (previous != down) SetMouseButton(button, down ? InputAction::Press : InputAction::Release, HeldModifiers());
}

void Input::SetMouseButton(int button, InputAction action, InputModifiers modifiers) {
    if (button < 0 || button >= kButtonCount || (action != InputAction::Release && action != InputAction::Press)) return;
    auto& state = buttons_[static_cast<std::size_t>(button)];
    if (((state & kDown) != 0) == (action == InputAction::Press)) return;
    Set(state, action == InputAction::Press);
    modifiers_ = modifiers;
    events_.emplace_back(MouseButtonEvent{static_cast<MouseButton>(button), action, modifiers, cursor_});
}

void Input::SetCursor(pxPoint position) { SetCursor(position, modifiers_); }
void Input::SetCursor(pxPoint position, InputModifiers modifiers) {
    const auto delta = position - cursor_;
    cursor_ = position;
    modifiers_ = modifiers;
    if (delta.x != 0 || delta.y != 0) events_.emplace_back(MouseMoveEvent{position, delta, modifiers});
}

void Input::AddScroll(pxVec2 offset) { AddScroll(offset, modifiers_); }
void Input::AddScroll(pxVec2 offset, InputModifiers modifiers) {
    scroll_ += offset;
    modifiers_ = modifiers;
    if (offset.x != 0 || offset.y != 0) events_.emplace_back(MouseScrollEvent{offset, cursor_, modifiers});
}

void Input::EndUpdate() {
    events_.clear();
    for (auto& state : keys_) {
        state &= kDown;
    }
    for (auto& state : buttons_) {
        state &= kDown;
    }
    scroll_ = pxVec2(0.0);
    cursorAtLastUpdate_ = cursor_;
}

void Input::ReleaseAll() {
    // Replace pending input with cancellation releases. kCancelled retains a release if the
    // host cancels again before a fixed update has consumed it.
    events_.clear();
    modifiers_ = InputModifiers::None;
    for (int key = 0; key < kKeyCount; ++key) {
        auto& state = keys_[static_cast<std::size_t>(key)];
        const bool release = (state & (kDown | kCancelled)) != 0 || ((state & kReleased) != 0 && (state & kPressed) == 0);
        if (release) events_.emplace_back(KeyEvent{static_cast<Key>(key), InputAction::Release, modifiers_, 0});
        state = release ? static_cast<std::uint8_t>(kReleased | kCancelled) : std::uint8_t{0};
    }
    for (int button = 0; button < kButtonCount; ++button) {
        auto& state = buttons_[static_cast<std::size_t>(button)];
        const bool release = (state & (kDown | kCancelled)) != 0 || ((state & kReleased) != 0 && (state & kPressed) == 0);
        if (release) events_.emplace_back(MouseButtonEvent{static_cast<MouseButton>(button), InputAction::Release, modifiers_, cursor_});
        state = release ? static_cast<std::uint8_t>(kReleased | kCancelled) : std::uint8_t{0};
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
