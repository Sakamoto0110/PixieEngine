#include "pixie/runtime/AttractorAffector.hpp"

#include "pixie/runtime/Attractor.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

#include <cmath>

namespace pixie {

void AttractorAffector::Apply(World& world, Entity, std::span<Particle> particles,
                              const FrameTime& time) {
    sources_.clear();
    for (auto [id, transform, attractor] :
         world.Registry().view<const Transform2D, const Attractor>().each()) {
        const double radiusSquared = attractor.radius > 0.0 ? attractor.radius * attractor.radius : 0.0;
        sources_.push_back({static_cast<float>(transform.position.x),
                            static_cast<float>(transform.position.y),
                            static_cast<float>(attractor.strength),
                            static_cast<float>(attractor.softening * attractor.softening),
                            static_cast<float>(radiusSquared)});
    }
    if (sources_.empty()) {
        return;
    }
    const auto dt = static_cast<float>(time.delta);
    for (Particle& particle : particles) {
        float ax = 0.0f;
        float ay = 0.0f;
        for (const Source& source : sources_) {
            const float dx = source.x - particle.position.x;
            const float dy = source.y - particle.position.y;
            const float distanceSquared = dx * dx + dy * dy;
            if (source.radiusSquared > 0.0f && distanceSquared > source.radiusSquared) {
                continue;
            }
            const float softened = distanceSquared + source.softeningSquared;
            const float pull = source.strength / (softened * std::sqrt(softened));
            ax += dx * pull;
            ay += dy * pull;
        }
        particle.velocity.x += ax * dt;
        particle.velocity.y += ay * dt;
    }
}

} // namespace pixie
