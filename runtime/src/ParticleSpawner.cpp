#include "pixie/runtime/ParticleSpawner.hpp"

#include "pixie/runtime/ParticleSystem.hpp"
#include "pixie/runtime/Random.hpp"
#include "pixie/runtime/Transform2D.hpp"

#include <pixie/math/pxSegment.hpp>

#include <algorithm>
#include <cmath>

namespace pixie {

namespace {

struct Start {
    // Offset from the spawner, in its rotated frame, and the outward normal there (local angle).
    pxVec2 offset;
    double normal = 0.0;
};

Start StartOnShape(const ParticleSpawner& s, Random& random) {
    switch (s.shape) {
    case EmissionShape::Point:
        return {pxVec2(0.0), 0.0};
    case EmissionShape::Circle: {
        // Uniform over the disc's area.
        const double r = s.radius * std::sqrt(random.NextDouble());
        const pxVec2 out = random.Direction();
        return {out * r, std::atan2(out.y, out.x)};
    }
    case EmissionShape::Ring: {
        const pxVec2 out = random.Direction();
        return {out * s.radius, std::atan2(out.y, out.x)};
    }
    case EmissionShape::Rectangle: {
        const pxVec2 at(random.Range(-s.size.x, s.size.x), random.Range(-s.size.y, s.size.y));
        // The nearest edge's normal.
        const double toRight = s.size.x - at.x;
        const double toLeft = at.x + s.size.x;
        const double toBottom = s.size.y - at.y;
        const double toTop = at.y + s.size.y;
        double normal = 0.0;
        double nearest = toRight;
        const auto consider = [&](double distance, double angle) {
            if (distance < nearest) {
                nearest = distance;
                normal = angle;
            }
        };
        consider(toLeft, std::numbers::pi);
        consider(toBottom, std::numbers::pi / 2.0);
        consider(toTop, -std::numbers::pi / 2.0);
        return {at, normal};
    }
    case EmissionShape::Segment: {
        const pxSegment segment(pxPoint(0.0, 0.0), pxToPoint(s.segmentEnd));
        const pxVec2 along = segment.Direction();
        return {pxToVec2(segment.PointAt(random.NextDouble())),
                std::atan2(along.y, along.x) + std::numbers::pi / 2.0};
    }
    }
    return {};
}

// A Gaussian offset with a standard deviation of half `jitter` on each axis, so about 86% of the
// offsets are within `jitter` (Box-Muller, from the World's random numbers).
pxVec2 Jitter(double jitter, Random& random) {
    const double sigma = 0.5 * jitter;
    const double radius = sigma * std::sqrt(-2.0 * std::log(1.0 - random.NextDouble()));
    return random.Direction() * radius;
}

pxVec2 Rotate(pxVec2 v, double angle) {
    const double c = std::cos(angle);
    const double s = std::sin(angle);
    return pxVec2(v.x * c - v.y * s, v.x * s + v.y * c);
}

} // namespace

int EmitFromSpawner(ParticleSpawner& spawner, const Transform2D& transform, const Transform2D* target,
                    ParticleSystem& system, Random& random, double delta) {
    if (spawner.rate <= 0.0) {
        return 0;
    }
    const double owedBefore = spawner.pending;
    spawner.pending += spawner.rate * delta;
    int emitted = 0;
    while (spawner.pending >= 1.0) {
        spawner.pending -= 1.0;
        // When within the step this particle came due: its share of the rate completed then.
        const double due = std::clamp((emitted + 1 - owedBefore) / spawner.rate, 0.0, delta);
        const Start start = StartOnShape(spawner, random);
        pxPoint at = transform.position + Rotate(start.offset, transform.rotation);
        if (spawner.jitter > 0.0) {
            at = at + Jitter(spawner.jitter, random);
        }

        double base = transform.rotation;
        if (spawner.directionMode == DirectionMode::ShapeNormal) {
            base = transform.rotation + start.normal;
        } else if (spawner.directionMode == DirectionMode::TowardTarget && target != nullptr) {
            const pxVec2 toTarget = target->position - at;
            base = std::atan2(toTarget.y, toTarget.x);
        }
        const double angle = base + spawner.direction + random.Range(-0.5, 0.5) * spawner.spread;
        const double speed = random.Range(spawner.speedMin, spawner.speedMax);
        const pxVec2f velocity(static_cast<float>(std::cos(angle) * speed),
                               static_cast<float>(std::sin(angle) * speed));
        // The step moves every particle a whole step after this; one born at `due` starts back by that
        // much, so it ends the step where it would have got to, from its birth point.
        const pxPointf born(static_cast<float>(at.x), static_cast<float>(at.y));
        const auto early = static_cast<float>(due);
        const pxPointf startsAt = born + velocity * -early;
        const bool emittedOne = spawner.ownLook
            ? system.Emit(startsAt, velocity, spawner.lifetime, spawner.particleColor,
                          static_cast<float>(spawner.particleSize), spawner.particleShape)
            : system.Emit(startsAt, velocity, spawner.lifetime);
        if (!emittedOne) {
            spawner.pending = 0.0;
            break;
        }
        Particle& particle = system.particles.back();
        particle.previous = born;
        particle.age = static_cast<float>(delta - due);
        ++emitted;
    }
    return emitted;
}

} // namespace pixie
