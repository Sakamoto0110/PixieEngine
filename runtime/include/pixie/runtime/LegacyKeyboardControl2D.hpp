#pragma once

#include "pixie/runtime/Export.hpp"
#include "pixie/platform/Input.hpp"

namespace pixie {
struct KeyboardControl2D;
struct CharacterCommand2D;

// Compatibility only for the serialized KeyboardControl2D component. New game code owns mapping.
PIXIE_RUNTIME_API CharacterCommand2D LegacyKeyboardCommand(const KeyboardControl2D& control, const Input& input);
} // namespace pixie
