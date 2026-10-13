#pragma once

#include "pixie/runtime/Behaviour.hpp"
#include "pixie/runtime/Entity.hpp"

// One boid: steers toward the pointer (or away from it) when within reach, and otherwise cruises
// with a little random turning. It writes only its own Velocity2D and Transform2D rotation; the
// World integrates the velocity and the boids system wraps positions at the bounds.
class Boid final : public pixie::Behaviour {
public:
    explicit Boid(pixie::Entity controls) : controls_(controls) {}

    void Update(const pixie::FrameTime& time, const pixie::Input& input) override;

private:
    pixie::Entity controls_;
};
