#include "PlayerController.hpp"

#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/InputControl.hpp"
#include "pixie/runtime/Velocity2D.hpp"
#include "pixie/runtime/World.hpp"

#include <algorithm>
#include <cmath>

void PlayerController::Describe(pixie::ComponentTypes& types) {
    types.Add<PlayerController>("PlayerController")
        .Source("src/components/PlayerController.cpp")
        .Field<&PlayerController::enabled>("enabled")
        .Tooltip("Accept movement input.")
        .Field<&PlayerController::speed>("speed")
        .Tooltip("Movement speed in world units per second.")
        .Range(0.0, 1000.0).Scrub(1.0);
}

void PlayerController::Update(const pixie::FrameTime&, const pixie::Input& input) {
    auto* velocity = Self().TryGet<pixie::Velocity2D>();
    if (!velocity) return;
    velocity->linear = {0.0, 0.0};

    const auto* keyboard = Self().TryGet<pixie::KeyboardControl>();
    if (!enabled || !keyboard || !keyboard->enabled) return;

    const double x = double(keyboard->IsDown(input, pixie::Key::D))
                   - double(keyboard->IsDown(input, pixie::Key::A));
    const double y = double(keyboard->IsDown(input, pixie::Key::S))
                   - double(keyboard->IsDown(input, pixie::Key::W));
    const double length = std::hypot(x, y);
    if (length > 0.0) {
        const double scale = std::max(0.0, speed) / length;
        velocity->linear = {x * scale, y * scale};
    }
    // World integrates Velocity2D once. Do not also move Transform2D here.
}
