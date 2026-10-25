#include "pixie/runtime/CharacterController2D.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace pixie {
namespace {
double Approach(double current, double target, double amount) {
    return current + std::clamp(target - current, -amount, amount);
}
}
CharacterController2D::CharacterController2D(pxPoint position, CharacterSettings2D settings)
    : settings_(settings) {
    Configure(settings);
    Teleport(position);
}
void CharacterController2D::Configure(CharacterSettings2D settings) {
    const double values[] = {settings.radius, settings.halfSegment, settings.maxSpeed,
        settings.acceleration, settings.braking, settings.gravity, settings.terminalSpeed,
        settings.jumpSpeed, settings.maxSlopeDegrees, settings.coyoteSeconds, settings.jumpBufferSeconds};
    for (double v : values) if (!std::isfinite(v) || v < 0)
        throw std::invalid_argument("character settings must be finite and nonnegative");
    if (settings.radius <= 0 || settings.maxSlopeDegrees >= 90)
        throw std::invalid_argument("invalid character radius or slope limit");
    settings_ = settings;
}
void CharacterController2D::Teleport(pxPoint position) {
    if (!std::isfinite(position.x) || !std::isfinite(position.y))
        throw std::invalid_argument("character position must be finite");
    state_ = {};
    state_.position = position;
    coyote_ = bufferedJump_ = 0;
}
void CharacterController2D::Step(const CollisionWorld2D& collision, double horizontal, bool jumpPressed,
                                double delta, CollisionFilter2D filter) {
    if (!std::isfinite(horizontal) || !std::isfinite(delta) || delta <= 0 || delta > 0.05)
        throw std::invalid_argument("character step requires finite input and 0 < delta <= 0.05 seconds");
    horizontal = std::clamp(horizontal, -1.0, 1.0);
    const bool previouslyGrounded = state_.grounded;
    coyote_ = previouslyGrounded ? settings_.coyoteSeconds : std::max(0.0, coyote_ - delta);
    bufferedJump_ = jumpPressed ? settings_.jumpBufferSeconds : std::max(0.0, bufferedJump_ - delta);
    state_.velocity.x = Approach(state_.velocity.x, horizontal * settings_.maxSpeed,
                                 (horizontal == 0 ? settings_.braking : settings_.acceleration) * delta);
    // Follow a walkable support tangent rather than accumulating downhill gravity.
    if (previouslyGrounded) {
        state_.velocity.y = -state_.groundNormal.x / state_.groundNormal.y * state_.velocity.x;
    }
    const bool jump = (jumpPressed || bufferedJump_ > 0) && (previouslyGrounded || coyote_ > 0);
    if (jump) {
        state_.velocity.y = -settings_.jumpSpeed;
        coyote_ = bufferedJump_ = 0;
    } else {
        state_.velocity.y = std::min(state_.velocity.y + settings_.gravity * delta, settings_.terminalSpeed);
    }
    const double threshold = std::cos(settings_.maxSlopeDegrees * std::numbers::pi / 180);
    state_ = collision.MoveCapsule(state_.position, settings_.radius, settings_.halfSegment,
                                   state_.velocity * delta, state_.velocity, threshold, filter);
}
} // namespace pixie
