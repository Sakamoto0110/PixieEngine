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
double Dot(pxVec2 a, pxVec2 b) { return a.x * b.x + a.y * b.y; }
pxVec2 Difference(pxPoint a, pxPoint b) { return {a.x - b.x, a.y - b.y}; }
pxVec2 Limited(pxVec2 value, double maximum) {
    const double length = std::hypot(value.x, value.y);
    return length > maximum && length > 0 ? value * (maximum / length) : value;
}
// Separation slower than this (world units/s) does not leave the ground. Clipping against a pushed
// body's slightly tilted face leaves hundredths of a unit/s of upward velocity; a takeoff is hundreds.
constexpr double kSeparatingSpeed = 1.0;
const CapsuleContact2D* Ground(const std::vector<CapsuleContact2D>& contacts, double threshold, double slop, pxVec2 velocity) {
    const CapsuleContact2D* best = nullptr;
    for (const auto& c : contacts) {
        if (c.separation > slop || c.normal.y > -threshold || Dot(velocity, c.normal) > kSeparatingSpeed) continue;
        if (!best || c.normal.y < best->normal.y) best = &c;
    }
    return best;
}
}
CharacterController2D::CharacterController2D(pxPoint position, CharacterSettings2D settings)
    : settings_(settings) {
    Configure(settings);
    Teleport(position);
}
void CharacterController2D::Configure(CharacterSettings2D settings) {
    if (prepared_) throw std::logic_error("finish the prepared character step before configuring");
    const double values[] = {settings.radius, settings.halfSegment, settings.maxSpeed,
        settings.acceleration, settings.braking, settings.gravity, settings.terminalSpeed,
        settings.jumpSpeed, settings.maxSlopeDegrees, settings.coyoteSeconds, settings.jumpBufferSeconds,
        settings.effectiveMass, settings.maxPushForce, settings.maxPushSpeed, settings.maxSupportForce,
        settings.maxInheritedSpeed, settings.maxCarryStep, settings.maxRecoveryDistance};
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
    coupling_ = {};
    prepared_ = jumping_ = false;
    preparedWorld_ = nullptr;
}
bool CharacterController2D::IntegrateIntent(double horizontal, bool jumpPressed, double delta) {
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
    return jump;
}
void CharacterController2D::Step(const CollisionWorld2D& collision, double horizontal, bool jumpPressed,
                                double delta, CollisionFilter2D filter) {
    if (prepared_) throw std::logic_error("finish the prepared character step before using the query-only path");
    IntegrateIntent(horizontal, jumpPressed, delta);
    coupling_ = {};
    const double threshold = std::cos(settings_.maxSlopeDegrees * std::numbers::pi / 180);
    state_ = collision.MoveCapsule(state_.position, settings_.radius, settings_.halfSegment,
                                   state_.velocity * delta, state_.velocity, threshold, filter);
}

void CharacterController2D::PrepareStep(CollisionWorld2D& collision, double horizontal, bool jumpPressed,
                                      double delta, CollisionFilter2D filter) {
    if (prepared_) throw std::logic_error("character step is already prepared");
    if (!std::isfinite(horizontal) || !std::isfinite(delta) || delta <= 0 || delta > 0.05)
        throw std::invalid_argument("character step requires finite input and 0 < delta <= 0.05 seconds");
    const double threshold = std::cos(settings_.maxSlopeDegrees * std::numbers::pi / 180);
    // Always reacquire from current geometry. A teleported support can supply a fresh anchor,
    // but its teleport delta can never become carry from the last completed tick.
    coupling_ = {};
    const auto contacts = collision.CapsuleContacts(state_.position, settings_.radius, settings_.halfSegment, filter);
    const auto* ground = Ground(contacts, threshold, collision.CapsuleSlop(), state_.velocity);
    if (!ground && state_.grounded) coyote_ = settings_.coyoteSeconds + delta;
    state_.grounded = ground != nullptr;
    if (ground) {
        state_.groundNormal = ground->normal;
        coupling_.support = ground->body;
        coupling_.supportShape = ground->collider;
        coupling_.supportPoint = ground->point;
        coupling_.localSupportPoint = collision.BodyLocalPoint(ground->body, ground->point);
        coupling_.supportVelocity = collision.BodyPointVelocity(ground->body, ground->point);
        preparedSupportPoint_ = collision.BodyWorldPoint(ground->body, coupling_.localSupportPoint);
        preparedSupportRevision_ = collision.BodyRevision(ground->body);
    }
    jumping_ = IntegrateIntent(horizontal, jumpPressed, delta);
    if (jumping_) state_.velocity += Limited(coupling_.supportVelocity, settings_.maxInheritedSpeed);
    if (ground && !jumping_ && collision.BodyType(ground->body) == BodyType2D::Dynamic) {
        coupling_.supportLoad = std::min(settings_.maxSupportForce, settings_.effectiveMass * settings_.gravity);
        if (coupling_.supportLoad > 0) collision.ApplyForce(ground->body, {0, coupling_.supportLoad}, ground->point);
    }
    const auto carryPrediction = ground && !jumping_ ? Limited(coupling_.supportVelocity * delta, settings_.maxCarryStep) : pxVec2{0, 0};
    const auto prediction = collision.MoveCapsule(state_.position, settings_.radius, settings_.halfSegment,
        state_.velocity * delta + carryPrediction, state_.velocity, threshold, filter);
    const auto obstacles = collision.CapsuleContacts(prediction.position, settings_.radius, settings_.halfSegment, filter);
    std::vector<std::uint64_t> pushed;
    double budget = settings_.maxPushForce * delta;
    const auto desiredVelocity = state_.velocity + (jumping_ ? pxVec2{0, 0} : coupling_.supportVelocity);
    for (const auto& c : obstacles) {
        if (budget <= 0 || settings_.effectiveMass == 0 || std::abs(c.normal.x) < 0.25 || c.normal.y > 0.5
            || c.normal.y <= -threshold || c.separation > collision.CapsuleSlop()
            || c.separation < -settings_.maxRecoveryDistance || collision.BodyType(c.body) != BodyType2D::Dynamic
            || std::find(pushed.begin(), pushed.end(), c.body.value) != pushed.end()) continue;
        const pxVec2 direction{c.normal.x < 0 ? 1.0 : -1.0, 0};
        const auto velocity = collision.BodyPointVelocity(c.body, c.point);
        const double closing = Dot(desiredVelocity - velocity, direction);
        const double speedRoom = settings_.maxPushSpeed - Dot(velocity, direction);
        const auto mass = collision.BodyMass(c.body);
        if (closing <= 0 || speedRoom <= 0 || mass.mass <= 0) continue;
        const auto center = collision.BodyWorldPoint(c.body, mass.localCenter);
        const auto arm = Difference(c.point, center);
        const double lever = arm.x * direction.y - arm.y * direction.x;
        const double inverse = 1 / mass.mass + (mass.inertia > 0 ? lever * lever / mass.inertia : 0);
        const double impulse = std::min({budget, settings_.effectiveMass * closing, speedRoom / inverse});
        collision.ApplyImpulse(c.body, direction * impulse, c.point);
        budget -= impulse;
        coupling_.pushedImpulse += impulse;
        pushed.push_back(c.body.value);
    }
    preparedPosition_ = state_.position;
    preparedDelta_ = delta;
    preparedFilter_ = filter;
    preparedWorld_ = &collision;
    preparedGeneration_ = collision.Generation();
    prepared_ = true;
}

void CharacterController2D::FinishStep(const CollisionWorld2D& collision) {
    if (!prepared_) throw std::logic_error("prepare the character before finishing its step");
    prepared_ = false;
    const auto fail = [&](CharacterBlocked2D reason) {
        state_.position = preparedPosition_;
        state_.velocity = {0, 0}; state_.grounded = false;
        coupling_.support = {}; coupling_.supportShape = {};
        coupling_.worldVelocity = {0, 0}; coupling_.carried = {0, 0};
        coupling_.blocked = reason; coupling_.unsafePlacement = true;
    };
    if (preparedWorld_ != &collision || preparedGeneration_ != collision.Generation()) {
        fail(CharacterBlocked2D::WorldChanged); return;
    }
    const double threshold = std::cos(settings_.maxSlopeDegrees * std::numbers::pi / 180);
    pxVec2 carry{0, 0};
    if (!jumping_ && coupling_.support.value && collision.ContainsBody(coupling_.support)
        && collision.Contains(coupling_.supportShape) && collision.BodyEnabled(coupling_.support)
        && collision.BodyRevision(coupling_.support) == preparedSupportRevision_)
        carry = Difference(collision.BodyWorldPoint(coupling_.support, coupling_.localSupportPoint), preparedSupportPoint_);
    if (std::hypot(carry.x, carry.y) > settings_.maxCarryStep + collision.CapsuleSlop()) {
        fail(CharacterBlocked2D::SupportStepLimit); return;
    }
    const auto carried = collision.MoveCapsule(preparedPosition_, settings_.radius, settings_.halfSegment,
        carry, {0, 0}, threshold, preparedFilter_);
    const double correction = std::hypot(carried.position.x - preparedPosition_.x - carry.x,
                                         carried.position.y - preparedPosition_.y - carry.y);
    if (correction > settings_.maxRecoveryDistance + collision.CapsuleSlop()) {
        fail(CharacterBlocked2D::NoValidPlacement); return;
    }
    if (std::hypot(carry.x, carry.y) > collision.CapsuleSlop() && correction > collision.CapsuleSlop())
        coupling_.blocked = CharacterBlocked2D::CarryObstructed;
    const auto moved = collision.MoveCapsule(carried.position, settings_.radius, settings_.halfSegment,
        state_.velocity * preparedDelta_, state_.velocity, threshold, preparedFilter_);
    const auto contacts = collision.CapsuleContacts(moved.position, settings_.radius, settings_.halfSegment, preparedFilter_);
    for (const auto& c : contacts) {
        if (c.separation < -collision.CapsuleSlop() - 0.001) { fail(CharacterBlocked2D::NoValidPlacement); return; }
    }
    state_ = moved;
    coupling_.carried = Difference(carried.position, preparedPosition_);
    coupling_.worldVelocity = Difference(state_.position, preparedPosition_) * (1 / preparedDelta_);
    coupling_.support = {}; coupling_.supportShape = {};
    coupling_.supportVelocity = {0, 0};
    if (const auto* ground = Ground(contacts, threshold, collision.CapsuleSlop(), state_.velocity)) {
        coupling_.support = ground->body; coupling_.supportShape = ground->collider;
        coupling_.supportPoint = ground->point;
        coupling_.localSupportPoint = collision.BodyLocalPoint(ground->body, ground->point);
        coupling_.supportVelocity = collision.BodyPointVelocity(ground->body, ground->point);
    }
}
} // namespace pixie
