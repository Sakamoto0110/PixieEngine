#include "pixie/runtime/Picking.hpp"

#include "pixie/runtime/ParticleSystem.hpp"
#include "pixie/runtime/Path.hpp"
#include "pixie/runtime/PathDrawing.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Sprite.hpp"
#include "pixie/runtime/SpriteDrawing.hpp"
#include "pixie/runtime/Text.hpp"
#include "pixie/runtime/TextDrawing.hpp"
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

entt::entity PickAt(World& world, pxPoint point, double tolerance, SpriteImages* sprites, Fonts* fonts) {
    entt::entity best = entt::null;
    double bestDistance = std::numeric_limits<double>::max();
    for (auto [id, transform, shape] : world.Registry().view<const Transform2D, const Shape>().each()) {
        if (!shape.visible) {
            continue;
        }
        const double distance = std::hypot(transform.position.x - point.x, transform.position.y - point.y);
        if (distance <= PickRadius(shape, transform) + tolerance && distance < bestDistance) {
            best = id;
            bestDistance = distance;
        }
    }
    if (best != entt::null) {
        return best;
    }
    // The last sprite drawn over the point is the one seen there; paths lie between the sprites of
    // negative layers and the rest.
    entt::entity background = entt::null;
    ForEachDrawnInOrder(world, [&](const Drawn& drawn) {
        const bool covers =
            drawn.sprite != nullptr
                ? sprites != nullptr && SpriteContains(*drawn.sprite, drawn.transform,
                                                       SpritePixelSize(*drawn.sprite, *sprites), point, tolerance)
                : fonts != nullptr && TextContains(*drawn.text, drawn.transform, TextBlock(*drawn.text, *fonts),
                                                   point, tolerance);
        if (covers) {
            (drawn.layer < 0 ? background : best) = drawn.id;
        }
    });
    if (best != entt::null) {
        return best;
    }
    std::vector<entt::entity> paths;
    for (auto [id, transform, path] : world.Registry().view<const Transform2D, const Path>().each()) {
        if (path.visible && PathContains(path, transform, point, tolerance)) {
            paths.push_back(id);
        }
    }
    // EnTT iterates newest first, and the newest path is drawn last.
    return !paths.empty() ? paths.front() : background;
}

std::vector<entt::entity> PickIn(World& world, const pxRect& box) {
    std::vector<entt::entity> picked;
    const entt::registry& registry = world.Registry();
    for (auto [id, transform] : registry.view<const Transform2D>().each()) {
        const pxPoint p = transform.position;
        const auto inside = [&](pxPoint q) {
            return q.x >= box.x && q.x <= box.Right() && q.y >= box.y && q.y <= box.Bottom();
        };
        // Hidden shapes, sprites, texts, and paths are not picked.
        const auto* shape = registry.try_get<Shape>(id);
        const auto* sprite = registry.try_get<Sprite>(id);
        const auto* text = registry.try_get<Text>(id);
        const bool drawn = (shape != nullptr && shape->visible) || (sprite != nullptr && sprite->visible) ||
                           (text != nullptr && text->visible);
        const Path* path = registry.try_get<Path>(id);
        if (drawn && inside(p)) {
            picked.push_back(id);
        } else if (path != nullptr && path->visible) {
            const std::vector<pxPoint> line = PathLine(*path, transform);
            if (!line.empty() && std::all_of(line.begin(), line.end(), inside)) {
                picked.push_back(id);
            }
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
