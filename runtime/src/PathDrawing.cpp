#include "pixie/runtime/PathDrawing.hpp"

#include "pixie/runtime/Path.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

#include <algorithm>
#include <cmath>

namespace pixie {

std::vector<pxPoint> PathLine(const Path& path, const Transform2D& transform) {
    const double c = std::cos(transform.rotation);
    const double s = std::sin(transform.rotation);
    std::vector<pxPoint> world;
    world.reserve(path.points.size());
    for (const pxPoint& p : path.points) {
        const double x = p.x * transform.scale.x;
        const double y = p.y * transform.scale.y;
        world.emplace_back(transform.position.x + c * x - s * y, transform.position.y + s * x + c * y);
    }
    return RoundCorners(world, path.cornerRadius);
}

bool PathContains(const Path& path, const Transform2D& transform, pxPoint point, double tolerance) {
    const std::vector<pxPoint> line = PathLine(path, transform);
    const double reach = path.width / 2.0 + tolerance;
    for (std::size_t i = 1; i < line.size(); ++i) {
        const double ax = line[i].x - line[i - 1].x;
        const double ay = line[i].y - line[i - 1].y;
        const double squared = ax * ax + ay * ay;
        const double t =
            squared > 0.0
                ? std::clamp(((point.x - line[i - 1].x) * ax + (point.y - line[i - 1].y) * ay) / squared, 0.0, 1.0)
                : 0.0;
        if (std::hypot(point.x - (line[i - 1].x + ax * t), point.y - (line[i - 1].y + ay * t)) <= reach) {
            return true;
        }
    }
    return false;
}

void SubmitPaths(World& world, PathRenderer& renderer, double alpha) {
    entt::registry& registry = world.Registry();
    for (auto [id, current, path] : registry.view<const Transform2D, const Path>().each()) {
        if (!path.visible) {
            continue;
        }
        const auto* previous = registry.try_get<const PreviousTransform2D>(id);
        const Transform2D drawn = previous != nullptr ? Interpolate(previous->value, current, alpha) : current;
        renderer.Submit(PathLine(path, drawn), path.width, path.color, path.fillColor, path.fill);
    }
}

} // namespace pixie
