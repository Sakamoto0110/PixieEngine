#pragma once

#include "pixie/runtime/Export.hpp"

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
    double effectiveMass = 1;
    double maxPushForce = 1200, maxPushSpeed = 200, maxSupportForce = 1200;
    double maxInheritedSpeed = 512, maxCarryStep = 8, maxRecoveryDistance = 8;
    bool operator==(const CharacterSettings2D&) const = default;
};

enum class CharacterBlocked2D { None, SupportStepLimit, CarryObstructed, NoValidPlacement, WorldChanged };
struct CharacterCouplingState2D {
    // Point/velocity fields describe support only when its handle is nonzero.
    BodyHandle2D support;
    ColliderHandle2D supportShape;
    pxPoint supportPoint, localSupportPoint;
    pxVec2 supportVelocity{0, 0}, carried{0, 0}, worldVelocity{0, 0};
    double pushedImpulse = 0, supportLoad = 0;
    double receivedImpulse = 0; // What bodies pressed on the character this tick (kg*worldUnits/s).
    CharacterBlocked2D blocked = CharacterBlocked2D::None;
    bool unsafePlacement = false;
};

// An explicit, caller-owned controller. Step once per fixed update; do not also integrate this
// position through Velocity2D. Jump is an optional command, not an engine input binding or game rule.
class CharacterController2D {
public:
    PIXIE_RUNTIME_API explicit CharacterController2D(pxPoint position = {}, CharacterSettings2D settings = {});
    // Validates new tuning while preserving current motion. Call between completed steps.
    PIXIE_RUNTIME_API void Configure(CharacterSettings2D settings);
    PIXIE_RUNTIME_API void Teleport(pxPoint position);
    PIXIE_RUNTIME_API void Step(const CollisionWorld2D& collision, double horizontal, bool jumpPressed, double delta,
              CollisionFilter2D filter = {});
    // Coupled path: prepare all motors, step this same physics world once, then finish all motors.
    // Preparation integrates intent once and applies bounded body commands; finish only queries.
    PIXIE_RUNTIME_API void PrepareStep(CollisionWorld2D& collision, double horizontal, bool jumpPressed, double delta,
                     CollisionFilter2D filter = {});
    // Between PrepareStep and FinishStep: an impulse bodies applied to the character, for example
    // read from its character proxy's contacts. Finish moves the capsule by impulse / effectiveMass
    // over the step, capped at maxPushSpeed and through collision; it never changes motor velocity.
    PIXIE_RUNTIME_API void ReceiveImpulse(pxVec2 impulse);
    PIXIE_RUNTIME_API void FinishStep(const CollisionWorld2D& collision);
    // Motor velocity excludes carry while supported; Coupling().worldVelocity includes solved displacement.
    const CapsuleMotion2D& State() const { return state_; }
    const CharacterSettings2D& Settings() const { return settings_; }
    const CharacterCouplingState2D& Coupling() const { return coupling_; }
private:
    PIXIE_RUNTIME_API bool IntegrateIntent(double horizontal, bool jumpPressed, double delta);
    CharacterSettings2D settings_;
    CapsuleMotion2D state_;
    double coyote_ = 0;
    double bufferedJump_ = 0;
    CharacterCouplingState2D coupling_;
    bool prepared_ = false, jumping_ = false;
    double preparedDelta_ = 0;
    CollisionFilter2D preparedFilter_;
    const CollisionWorld2D* preparedWorld_ = nullptr;
    std::uint64_t preparedGeneration_ = 0;
    std::uint64_t preparedSupportRevision_ = 0;
    pxPoint preparedPosition_, preparedSupportPoint_;
    pxVec2 received_{0, 0};
};

} // namespace pixie
