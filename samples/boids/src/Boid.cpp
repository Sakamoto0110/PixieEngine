#include "Boid.hpp"

#include "BoidControls.hpp"

#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/Velocity2D.hpp"
#include "pixie/runtime/World.hpp"

#include <cmath>

namespace {

pxVec2 ClampLength(pxVec2 v, double maxLength) {
    const double length = glm::length(v);
    return length > maxLength && length > 0.0 ? v * (maxLength / length) : v;
}

} // namespace

void Boid::Update(const pixie::FrameTime& time, const pixie::Input&) {
    const BoidControls& controls = controls_.Get<BoidControls>();
    const Pointer& pointer = controls_.Get<Pointer>();
    pixie::Transform2D& transform = Self().Get<pixie::Transform2D>();
    pxVec2& velocity = Self().Get<pixie::Velocity2D>().linear;

    pxVec2 desired = velocity;
    const pxVec2 toPointer = pointer.position - transform.position;
    const double distance = glm::length(toPointer);
    if (pointer.active && distance < controls.radius && distance > 1e-6) {
        // Full speed toward or away from the pointer; repelling grows stronger closer in.
        const pxVec2 direction = toPointer / distance;
        if (controls.mode == SteerMode::Attract) {
            desired = direction * controls.maxSpeed;
        } else {
            const double closeness = 1.0 - distance / controls.radius;
            desired = -direction * controls.maxSpeed * (0.5 + closeness);
        }
    } else {
        // Out of reach: turn a little at random and settle back to cruising speed.
        const double turn = GetWorld().GetRandom().Range(-controls.wander, controls.wander) * time.delta;
        const double c = std::cos(turn);
        const double s = std::sin(turn);
        const pxVec2 heading(velocity.x * c - velocity.y * s, velocity.x * s + velocity.y * c);
        const double speed = glm::length(heading);
        desired = speed > 1e-6 ? heading * (controls.cruiseSpeed / speed) : pxVec2(controls.cruiseSpeed, 0.0);
    }

    const pxVec2 steer = ClampLength(desired - velocity, controls.maxForce * time.delta);
    velocity = ClampLength(velocity + steer, controls.maxSpeed);
    if (glm::length(velocity) > 1e-6) {
        transform.rotation = std::atan2(velocity.y, velocity.x);
    }
}
