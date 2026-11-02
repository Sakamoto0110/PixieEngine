#include "pixie/runtime/LegacyKeyboardControl2D.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"

namespace pixie {
CharacterCommand2D LegacyKeyboardCommand(const KeyboardControl2D& control, const Input& input) {
    if (!control.enabled) return {};
    return {(input.IsDown(Key::D) || input.IsDown(Key::Right) ? 1.0 : 0.0)
            - (input.IsDown(Key::A) || input.IsDown(Key::Left) ? 1.0 : 0.0), input.WasPressed(Key::Space)};
}
} // namespace pixie
