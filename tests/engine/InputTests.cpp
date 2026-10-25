#include "Check.hpp"

#include "pixie/Input.hpp"

using pixie::Input;
using pixie::Key;
using pixie::MouseButton;

TEST(PressBetweenUpdatesIsSeenByTheNextUpdateOnce) {
    Input input;
    input.SetKey(static_cast<int>(Key::Space), true);
    input.SetKey(static_cast<int>(Key::Space), false);
    // Pressed and released before any update ran: the update still sees both.
    CHECK(input.WasPressed(Key::Space));
    CHECK(input.WasReleased(Key::Space));
    CHECK(!input.IsDown(Key::Space));
    input.EndUpdate();
    CHECK(!input.WasPressed(Key::Space));
    CHECK(!input.WasReleased(Key::Space));
}

TEST(HeldKeyStaysDownAcrossUpdates) {
    Input input;
    input.SetKey(static_cast<int>(Key::A), true);
    input.EndUpdate();
    input.SetKey(static_cast<int>(Key::A), true); // a repeat is not a new press
    CHECK(input.IsDown(Key::A));
    CHECK(!input.WasPressed(Key::A));
}

TEST(MouseButtonsScrollAndMovementAccumulateUntilAnUpdate) {
    Input input;
    input.SetCursor(pxPoint(10.0, 20.0));
    input.EndUpdate();
    input.SetMouseButton(static_cast<int>(MouseButton::Right), true);
    input.SetCursor(pxPoint(15.0, 18.0));
    input.SetCursor(pxPoint(25.0, 30.0));
    input.AddScroll(pxVec2(0.0, 1.0));
    input.AddScroll(pxVec2(0.0, 2.0));
    CHECK(input.WasPressed(MouseButton::Right));
    CHECK(input.CursorMoved() == pxVec2(15.0, 10.0));
    CHECK(input.Scroll() == pxVec2(0.0, 3.0));
    input.EndUpdate();
    CHECK(input.IsDown(MouseButton::Right));
    CHECK(input.CursorMoved() == pxVec2(0.0, 0.0));
    CHECK(input.Scroll() == pxVec2(0.0, 0.0));
}

TEST(UnknownCodesAreIgnored) {
    Input input;
    input.SetKey(-1, true);
    input.SetKey(Input::kKeyCount, true);
    input.SetMouseButton(Input::kButtonCount, true);
    CHECK(!input.IsDown(static_cast<Key>(-1)));
}

TEST(ModalInputCancelsHeldControlsAndDiscardsPendingActions) {
    Input input;
    input.SetKey(static_cast<int>(Key::A), true);
    input.SetMouseButton(static_cast<int>(MouseButton::Left), true);
    input.EndUpdate();
    input.SetKey(static_cast<int>(Key::B), true);
    input.SetKey(static_cast<int>(Key::B), false);
    input.SetCursor(pxPoint(80.0, 30.0));
    input.AddScroll(pxVec2(0.0, 4.0));
    input.ReleaseAll();
    CHECK(!input.IsDown(Key::A) && input.WasReleased(Key::A) && !input.WasPressed(Key::A));
    CHECK(!input.IsDown(MouseButton::Left) && input.WasReleased(MouseButton::Left));
    CHECK(!input.WasPressed(Key::B) && !input.WasReleased(Key::B));
    CHECK(input.Cursor() == pxPoint(80.0, 30.0));
    CHECK(input.CursorMoved() == pxVec2(0.0) && input.Scroll() == pxVec2(0.0));
    input.EndUpdate();
    CHECK(!input.WasReleased(Key::A) && !input.WasReleased(MouseButton::Left));
}
