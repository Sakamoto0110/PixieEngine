#include "pixie/runtime/Picking.hpp"

#include "pixie/runtime/ParticleSystem.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Sprite.hpp"
#include "pixie/runtime/SpriteDrawing.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace pixie {

double PickRadius(const Shape& shape, const Transform2D& transform) {
    const double extent = std::max(std::abs(shape.size.x), std::abs(shape.size.y));
    const double scale = std::max(std::abs(transform.scale.x), std::abs(transform.scale.y));
    return extent * scale;
}

entt::entity PickAt(World& world, pxPoint point, double tolerance, SpriteImages* sprites) {
    entt::entity best = entt::null;
    double bestDistance = std::numeric_limits<double>::max();
    for (auto [id, transform, shape] : world.Registry().view<const Transform2D, const Shape>().each()) {
        const double distance = std::hypot(transform.position.x - point.x, transform.position.y - point.y);
        if (distance <= PickRadius(shape, transform) + tolerance && distance < bestDistance) {
            best = id;
            bestDistance = distance;
        }
    }
    if (best != entt::null || sprites == nullptr) {
        return best;
    }
    // The last sprite drawn over the point is the one seen there.
    ForEachSpriteInDrawOrder(world, [&](entt::entity id, const Transform2D& transform, const Sprite& sprite) {
        if (SpriteContains(sprite, transform, SpritePixelSize(sprite, *sprites), point, tolerance)) {
            best = id;
        }
    });
    return best;
}

std::vector<entt::entity> PickIn(World& world, const pxRect& box) {
    std::vector<entt::entity> picked;
    const entt::registry& registry = world.Registry();
    for (auto [id, transform] : registry.view<const Transform2D>().each()) {
        const pxPoint p = transform.position;
        if (registry.any_of<Shape, Sprite>(id) && p.x >= box.x && p.x <= box.Right() && p.y >= box.y &&
            p.y <= box.Bottom()) {
            picked.push_back(id);
        }
    }
    // EnTT iterates newest first.
    std::reverse(picked.begin(), picked.end());
    return picked;
}

bool RemoveParticleAt(World& world, pxPoint point, double radius) {
    ParticleSystem* bestSystem = nullptr;
    std::size_t bestIndex = 0;
    double bestSquared = radius * radius;
    for (auto [id, system] : world.Registry().view<ParticleSystem>().each()) {
        for (std::size_t i = 0; i < system.particles.size(); ++i) {
            const double dx = system.particles[i].position.x - point.x;
            const double dy = system.particles[i].position.y - point.y;
            const double squared = dx * dx + dy * dy;
            if (squared <= bestSquared) {
                bestSystem = &system;
                bestIndex = i;
                bestSquared = squared;
            }
        }
    }
    if (bestSystem == nullptr) {
        return false;
    }
    auto& particles = bestSystem->particles;
    particles[bestIndex] = particles.back();
    particles.pop_back();
    return true;
}

} // namespace pixie
