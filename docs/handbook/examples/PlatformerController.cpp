#include "PlatformerController.hpp"

#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/InputControl.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/World.hpp"

void PlatformerController::Describe(pixie::ComponentTypes& types) {
    types.Add<PlatformerController>("PlatformerController")
        .Source("src/components/PlatformerController.cpp")
        .Field<&PlatformerController::enabled>("enabled")
        .Tooltip("Accept horizontal movement and jump input.");
}

void PlatformerController::Start() {
    // Start runs inside a step. Publish the transient command after that step.
    GetWorld().Defer([entity = Self()](pixie::World&) {
        if (entity.IsAlive() && !entity.Has<pixie::CharacterCommand2D>()) {
            entity.Add<pixie::CharacterCommand2D>();
        }
    });
}

void PlatformerController::Update(const pixie::FrameTime&, const pixie::Input& input) {
    auto* command = Self().TryGet<pixie::CharacterCommand2D>();
    if (!command) return;
    command->horizontal = 0.0;
    command->jumpPressed = false;

    const auto* keyboard = Self().TryGet<pixie::KeyboardControl>();
    if (!enabled || !keyboard || !keyboard->enabled) return;
    command->horizontal = double(keyboard->IsDown(input, pixie::Key::D))
                        - double(keyboard->IsDown(input, pixie::Key::A));
    command->jumpPressed = keyboard->WasPressed(input, pixie::Key::Space);
    // Scene movement consumes this command during the Physics phase.
}
