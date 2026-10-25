#pragma once

#include "pixie/runtime/CollisionWorld2D.hpp"

namespace pixie {

struct CharacterSettings2D {
    double radius = 12;
    double halfSegment = 12;
    double maxSpeed = 200;
    double acceleration = 1400;
    double braking = 1800;
    double gravity = 1100;
    double terminalSpeed = 700;
    double jumpSpeed = 380;
    double maxSlopeDegrees = 45;
    double coyoteSeconds = 0.08;
    double jumpBufferSeconds = 0.1;
    bool operator==(const CharacterSettings2D&) const = default;
};

// An explicit, caller-owned controller. Step once per fixed update; do not also integrate this
// position through Velocity2D. Jump is an optional command, not an engine input binding or game rule.
class CharacterController2D {
public:
    explicit CharacterController2D(pxPoint position = {}, CharacterSettings2D settings = {});
    // Validates new tuning while preserving current motion and pending commands.
    void Configure(CharacterSettings2D settings);
    void Teleport(pxPoint position);
    void Step(const CollisionWorld2D& collision, double horizontal, bool jumpPressed, double delta,
              CollisionFilter2D filter = {});
    const CapsuleMotion2D& State() const { return state_; }
    const CharacterSettings2D& Settings() const { return settings_; }
private:
    CharacterSettings2D settings_;
    CapsuleMotion2D state_;
    double coyote_ = 0;
    double bufferedJump_ = 0;
};

} // namespace pixie
