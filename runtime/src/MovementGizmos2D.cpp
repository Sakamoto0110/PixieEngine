#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/World.hpp"
#include "pixie/render/2d/Camera2D.hpp"
#include "pixie/render/2d/ShapeRenderer.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <limits>

namespace pixie {
namespace {
std::vector<pxPoint> Outline(const Transform2D* t, const Collider2D* collider, const CharacterMotor2D* motor) {
    std::vector<pxPoint> local;
    pxPoint offset;
    if (motor) {
        if (!std::isfinite(motor->radius) || motor->radius <= 0 || !std::isfinite(motor->halfSegment)
            || motor->halfSegment < 0 || t->rotation != 0 || t->scale != pxVec2(1, 1)) return {};
        for (int i = 0; i <= 16; ++i) {
            const double angle = std::numbers::pi + i * std::numbers::pi / 16;
            local.emplace_back(motor->radius * std::cos(angle), -motor->halfSegment + motor->radius * std::sin(angle));
        }
        for (int i = 0; i <= 16; ++i) {
            const double angle = i * std::numbers::pi / 16;
            local.emplace_back(motor->radius * std::cos(angle), motor->halfSegment + motor->radius * std::sin(angle));
        }
    } else if (collider) {
        offset = collider->offset;
        switch (collider->kind) {
        case ColliderKind2D::Box:
            if (collider->halfSize.x <= 0 || collider->halfSize.y <= 0) return {};
            local = {{-collider->halfSize.x, -collider->halfSize.y}, {collider->halfSize.x, -collider->halfSize.y},
                     {collider->halfSize.x, collider->halfSize.y}, {-collider->halfSize.x, collider->halfSize.y}};
            break;
        case ColliderKind2D::Circle:
            if (!std::isfinite(collider->radius) || collider->radius <= 0
                || std::abs(std::abs(t->scale.x) - std::abs(t->scale.y)) > 1e-9) return {};
            for (int i = 0; i < 32; ++i) {
                const double angle = i * 2 * std::numbers::pi / 32;
                local.emplace_back(collider->radius * std::cos(angle), collider->radius * std::sin(angle));
            }
            break;
        case ColliderKind2D::Polygon:
            if (collider->points.size() < 3 || collider->points.size() > 8) return {};
            local = collider->points;
            break;
        default: return {};
        }
    }
    const double angle = collider ? collider->rotation : 0;
    if (!std::isfinite(angle)) return {};
    const double lc = std::cos(angle), ls = std::sin(angle);
    const double c = std::cos(t->rotation), s = std::sin(t->rotation);
    for (auto& point : local) {
        const double x = (lc * point.x - ls * point.y + offset.x) * t->scale.x;
        const double y = (ls * point.x + lc * point.y + offset.y) * t->scale.y;
        point = {t->position.x + c * x - s * y, t->position.y + s * x + c * y};
        if (!std::isfinite(point.x) || !std::isfinite(point.y)
            || std::abs(point.x) > std::numeric_limits<float>::max()
            || std::abs(point.y) > std::numeric_limits<float>::max()) return {};
    }
    return local;
}
} // namespace

std::vector<std::vector<pxPoint>> MovementOutlines2D(World& world, entt::entity id) {
    auto& registry = world.Registry();
    if (!registry.valid(id)) return {};
    const auto* t = registry.try_get<Transform2D>(id);
    if (!t) return {};
    std::vector<std::vector<pxPoint>> result;
    if (const auto* compound = registry.try_get<CompoundCollider2D>(id)) {
        for (const auto& shape : compound->shapes) {
            auto outline = Outline(t, &shape.value, nullptr);
            if (!outline.empty()) result.push_back(std::move(outline));
        }
    } else {
        auto outline = Outline(t, registry.try_get<Collider2D>(id), registry.try_get<CharacterMotor2D>(id));
        if (!outline.empty()) result.push_back(std::move(outline));
    }
    return result;
}
std::vector<pxPoint> MovementOutline2D(World& world, entt::entity id) {
    auto outlines = MovementOutlines2D(world, id);
    return outlines.empty() ? std::vector<pxPoint>{} : std::move(outlines.front());
}

namespace {
template<typename Fn> void EachGizmo(World& world, Fn fn) {
    for (const auto id : world.Registry().view<const Collider2D>()) fn(id);
    for (const auto id : world.Registry().view<const CompoundCollider2D>())
        if (!world.Registry().all_of<Collider2D>(id)) fn(id);
    for (const auto id : world.Registry().view<const CharacterMotor2D>())
        if (!world.Registry().any_of<Collider2D, CompoundCollider2D>(id)) fn(id);
}
bool Covers(const std::vector<pxPoint>& points, pxPoint p, double tolerance) {
    if (points.empty()) return false;
    bool inside = false;
    for (std::size_t i = 0; i < points.size(); ++i) {
        const auto a = points[i], b = points[(i + 1) % points.size()];
        if ((a.y > p.y) != (b.y > p.y) && p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x)
            inside = !inside;
        const double dx = b.x - a.x, dy = b.y - a.y;
        const double lengthSquared = dx * dx + dy * dy;
        const double fraction = lengthSquared > 0 ? std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / lengthSquared, 0.0, 1.0) : 0;
        if (std::hypot(p.x - a.x - fraction * dx, p.y - a.y - fraction * dy) <= tolerance) return true;
    }
    return inside;
}
}
void DrawMovementGizmos2D(World& world, ShapeRenderer& renderer, const Camera2D& camera) {
    const double halfWidth = 0.75 / camera.Zoom();
    const auto view = camera.VisibleWorld();
    EachGizmo(world, [&](entt::entity id) {
        for (const auto& points : MovementOutlines2D(world, id)) {
            if (points.empty()) continue;
            const auto [left, right] = std::minmax_element(points.begin(), points.end(), [](auto a, auto b) { return a.x < b.x; });
            const auto [top, bottom] = std::minmax_element(points.begin(), points.end(), [](auto a, auto b) { return a.y < b.y; });
            if (right->x < view.x || left->x > view.Right() || bottom->y < view.y || top->y > view.Bottom()) continue;
            const auto* collider = world.Registry().try_get<Collider2D>(id);
            const auto* compound = world.Registry().try_get<CompoundCollider2D>(id);
            const pxColorRgba color = compound ? (compound->enabled ? pxColorRgba{100, 190, 255} : pxColorRgba{120, 120, 140}) : collider ? (collider->enabled ? pxColorRgba{100, 190, 255} : pxColorRgba{120, 120, 140})
                                             : pxColorRgba{80, 240, 170};
            for (std::size_t i = 0; i < points.size(); ++i) {
                const auto a = points[i], b = points[(i + 1) % points.size()];
                ShapeInstance edge;
                edge.position = {static_cast<float>((a.x + b.x) / 2), static_cast<float>((a.y + b.y) / 2)};
                edge.scale = {static_cast<float>(std::hypot(b.x - a.x, b.y - a.y) / 2), static_cast<float>(halfWidth)};
                edge.rotation = static_cast<float>(std::atan2(b.y - a.y, b.x - a.x));
                edge.color = color;
                renderer.Submit(ShapeKind::Quad, edge);
            }
        }
    });
}
entt::entity PickMovementGizmo2D(World& world, pxPoint point, double tolerance) {
    entt::entity best = entt::null;
    double distance = INFINITY;
    EachGizmo(world, [&](entt::entity id) {
        const auto outlines = MovementOutlines2D(world, id);
        if (std::none_of(outlines.begin(), outlines.end(), [&](const auto& points) { return Covers(points, point, std::max(0.0, tolerance)); })) return;
        const auto& p = world.Registry().get<Transform2D>(id).position;
        const double candidate = std::hypot(p.x - point.x, p.y - point.y);
        if (candidate < distance) { best = id; distance = candidate; }
    });
    return best;
}
std::vector<entt::entity> PickMovementGizmosIn2D(World& world, const pxRect& rectangle) {
    std::vector<entt::entity> selected;
    EachGizmo(world, [&](entt::entity id) {
        const auto outlines = MovementOutlines2D(world, id);
        if (!outlines.empty() && std::all_of(outlines.begin(), outlines.end(), [&](const auto& points) {
            return std::all_of(points.begin(), points.end(), [&](auto p) {
                return p.x >= rectangle.x && p.x <= rectangle.Right() && p.y >= rectangle.y && p.y <= rectangle.Bottom();
            });
        })) selected.push_back(id);
    });
    return selected;
}
}
