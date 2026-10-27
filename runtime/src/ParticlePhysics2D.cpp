#include "pixie/runtime/ParticlePhysics2D.hpp"
#include "pixie/runtime/ParticleSystem.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace pixie {
namespace {
double Dot(pxVec2 a, pxVec2 b) { return a.x * b.x + a.y * b.y; }
void Expire(Particle& p, ParticlePhysicsStats2D& stats) { p.lifetime = 1; p.age = 1; ++stats.expired; }
bool Move(Particle& p, const ParticlePhysics2D& s, const CollisionWorld2D* world,
          pxVec2 gravity, double delta, ParticlePhysicsStats2D& stats) {
    const double dt = std::min(delta, static_cast<double>(p.age));
    pxPoint at{p.position.x, p.position.y};
    pxVec2 velocity{p.velocity.x, p.velocity.y};
    if (!std::isfinite(at.x) || !std::isfinite(at.y) || !std::isfinite(velocity.x) || !std::isfinite(velocity.y)) return false;
    velocity += gravity * (s.gravityScale * dt);
    const CollisionFilter2D filter{static_cast<std::uint64_t>(s.category), static_cast<std::uint64_t>(s.mask)};
    if (world && dt > 0) {
        ++stats.queries;
        const auto contacts = world->CapsuleContacts(at, s.collisionRadius, 0, filter);
        // The mover toolkit can omit a deeply buried shape. An overlap query detects that case
        // rather than treating an absent plane as free space and tunneling out of the body.
        if (contacts.empty()) {
            ++stats.queries;
            if (world->OverlapsCircle(at, s.collisionRadius, filter)) return false;
        }
        bool overlap = false;
        for (const auto& c : contacts) overlap = overlap || c.separation < 0;
        if (overlap) {
            ++stats.queries;
            const auto fixed = world->MoveCapsule(at, s.collisionRadius, 0, {0, 0}, {0, 0}, 0.70710678, filter);
            if (std::hypot(fixed.position.x-at.x, fixed.position.y-at.y) > 4*s.collisionRadius) return false;
            at = fixed.position;
            ++stats.queries;
            for (const auto& c : world->CapsuleContacts(at, s.collisionRadius, 0, filter)) {
                if (c.separation < -world->CapsuleSlop()-0.001) return false;
                if (c.separation < world->CapsuleSlop())
                    at = at + c.normal * (world->CapsuleSlop()-c.separation);
            }
            if (std::hypot(at.x-p.position.x, at.y-p.position.y) > 4*s.collisionRadius) return false;
        }
    }
    double remaining = dt;
    int hitCount = 0;
    while (remaining > 1e-8) {
        const auto travel = velocity * remaining;
        if (!world) { at = at + travel; break; }
        ++stats.queries;
        const auto hit = world->CircleCast(at, s.collisionRadius, travel, filter);
        if (!hit) { at = at + travel; break; }
        ++stats.contacts;
        // Shape casts allow a small separation tolerance. Rebuild the circle center from the
        // obstacle contact point so that tolerance cannot leave a resting spark slightly buried.
        at = hit->point + hit->normal * (s.collisionRadius + world->CapsuleSlop());
        const auto surface = world->BodyPointVelocity(hit->body, hit->point);
        const auto material = world->Material(hit->collider);
        const auto relative = velocity - surface;
        const double vn = Dot(relative, hit->normal);
        if (vn < 0) {
            const double e = -vn < s.bounceThreshold ? 0 : std::max(s.restitution, material.restitution);
            const auto tangent = relative - hit->normal * vn;
            const double speed = std::hypot(tangent.x, tangent.y);
            const double drop = std::sqrt(s.friction * material.friction) * (1+e) * -vn;
            velocity = surface + tangent * (speed > 0 ? std::max(0.0, 1-drop/speed) : 0)
                       - hit->normal * (e*vn);
        }
        remaining *= 1-hit->fraction;
        if (++hitCount == 4) { velocity = {0, 0}; ++stats.exhausted; break; }
    }
    if (!std::isfinite(at.x) || !std::isfinite(at.y) || !std::isfinite(velocity.x) || !std::isfinite(velocity.y)) return false;
    p.position = {static_cast<float>(at.x), static_cast<float>(at.y)};
    p.velocity = {static_cast<float>(velocity.x), static_cast<float>(velocity.y)};
    return true;
}
}
void MoveDynamicParticles2D(ParticleSystem& system, const ParticlePhysics2D& s,
                            const CollisionWorld2D* collision, pxVec2 gravity, double delta,
                            ParticlePhysicsStats2D& stats) {
    if (!std::isfinite(delta) || delta <= 0 || delta > 0.1 || !std::isfinite(gravity.x) || !std::isfinite(gravity.y))
        throw std::invalid_argument("particle physics requires finite gravity and 0 < delta <= 0.1");
    for (double value : {s.gravityScale, s.collisionRadius, s.restitution, s.friction, s.bounceThreshold})
        if (!std::isfinite(value) || value < 0) throw std::invalid_argument("particle physics settings must be finite and nonnegative");
    if (s.restitution > 1 || s.category <= 0 || s.mask < 0)
        throw std::invalid_argument("invalid particle restitution or collision filter");
    if (collision) collision->ValidateMoverCapsule(s.collisionRadius, 0);
    else if (s.collisionRadius <= 0) throw std::invalid_argument("particle collision radius must be positive");
    for (std::size_t i=0; i<system.particles.size();) {
        auto& p = system.particles[i];
        if ((p.flags & Particle::kDynamic) == 0) {
            p.position = p.position + p.velocity * static_cast<float>(delta); ++i; continue;
        }
        ++stats.dynamic;
        if (!Move(p, s, collision, gravity, delta, stats)) Expire(p, stats);
        if (p.lifetime > 0 && p.age >= p.lifetime) { p = system.particles.back(); system.particles.pop_back(); }
        else ++i;
    }
}
}
