#include "pixie/runtime/InteractionAffector.hpp"

#include <cmath>
#include <cstddef>

namespace pixie {

void InteractionAffector::Apply(World&, Entity, std::span<Particle> particles, const FrameTime& time) {
    // The grid's ids are opaque 32-bit values; here they are indices into the pool.
    grid_.SetCellSize(radius);
    grid_.Clear();
    for (std::size_t i = 0; i < particles.size(); ++i) {
        const pxPointf p = particles[i].position;
        grid_.Add(static_cast<entt::entity>(i), pxPoint(p.x, p.y));
    }
    grid_.Build();

    // Every change is computed from the positions and velocities as they were, then applied, so
    // the result does not depend on particle order.
    change_.assign(particles.size(), pxVec2f(0.0f));
    const double dt = time.delta;
    for (std::size_t i = 0; i < particles.size(); ++i) {
        const Particle& self = particles[i];
        const pxPoint center(self.position.x, self.position.y);
        pxVec2 push(0.0);
        grid_.ForEachWithin(center, radius, [&](entt::entity id, pxPoint other, double distanceSquared) {
            const auto j = static_cast<std::size_t>(id);
            if (j == i || distanceSquared <= 1e-12) {
                return;
            }
            const double distance = std::sqrt(distanceSquared);
            const pxVec2 away = (center - other) / distance;
            const double overlap = 1.0 - distance / radius;
            push += away * (stiffness * overlap);
            const pxVec2f relative = particles[j].velocity - self.velocity;
            push += pxVec2(relative) * (damping * overlap);
        });
        change_[i] = pxVec2f(push * dt);
    }
    for (std::size_t i = 0; i < particles.size(); ++i) {
        particles[i].velocity += change_[i];
    }
}

} // namespace pixie
