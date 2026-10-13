#pragma once

#include "pixie/runtime/Behaviour.hpp"
#include "pixie/runtime/Entity.hpp"

// One boid of a flock: separation, alignment, and cohesion with the neighbours that the flock's
// spatial grid finds, plus the pointer. It reads its neighbours' positions (from the grid's
// snapshot) and velocities, and writes only its own Acceleration2D; the World integrates after
// every Flocker has run, so the result does not depend on the order they run in.
class Flocker final : public pixie::Behaviour {
public:
    explicit Flocker(pixie::Entity flock) : flock_(flock) {}

    void Update(const pixie::FrameTime& time, const pixie::Input& input) override;

private:
    pixie::Entity flock_;
};
