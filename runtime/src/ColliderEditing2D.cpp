#include "pixie/runtime/ColliderEditing2D.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/World.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <limits>
#include <stdexcept>

namespace pixie {
namespace {
bool Finite(pxPoint p) { return std::isfinite(p.x) && std::isfinite(p.y); }
struct MotorShape {
    CharacterMotor2D* motor;
    const Transform2D* transform;
    double units;
};
std::optional<MotorShape> CharacterShape(World& world, entt::entity id, int index) {
    auto& r = world.Registry();
    if (index != -1 || !r.valid(id) || r.any_of<Collider2D, CompoundCollider2D, RigidBody2D>(id)) return {};
    auto* motor = r.try_get<CharacterMotor2D>(id);
    const auto* t = r.try_get<Transform2D>(id);
    if (!motor || !t || !Finite(t->position) || t->rotation != 0 || t->scale != pxVec2(1, 1)) return {};
    const auto settings = r.view<const PhysicsSettings2D>();
    if (settings.size() > 1) return {};
    const double units = settings.empty() ? 32 : settings.get<const PhysicsSettings2D>(*settings.begin()).unitsPerMeter;
    try { CollisionWorld2D::ValidateMoverCapsule(motor->radius, motor->halfSegment, units); }
    catch (const std::invalid_argument&) { return {}; }
    return MotorShape{motor, t, units};
}
bool DragCharacter(MotorShape shape, ColliderDrag2D kind, pxPoint pointer) {
    if (!Finite(pointer)) return false;
    auto candidate = *shape.motor;
    if (kind == ColliderDrag2D::Resize) {
        // The backend's strict float radius bound must remain satisfied after conversion.
        const double minimum = static_cast<double>(std::nextafter(0.01f, std::numeric_limits<float>::infinity())) * shape.units;
        candidate.radius = std::max(minimum, std::abs(pointer.x - shape.transform->position.x));
    } else if (kind == ColliderDrag2D::Height) {
        candidate.halfSegment = std::max(0.0, std::abs(pointer.y - shape.transform->position.y) - candidate.radius);
    } else return false;
    try { CollisionWorld2D::ValidateMoverCapsule(candidate.radius, candidate.halfSegment, shape.units); }
    catch (const std::invalid_argument&) { return false; }
    *shape.motor = candidate;
    return true;
}
pxPoint Rotate(pxPoint p, double angle) {
    const double c = std::cos(angle), s = std::sin(angle);
    return {c * p.x - s * p.y, s * p.x + c * p.y};
}
Collider2D* Shape(World& world, entt::entity id, int index) {
    auto& r = world.Registry();
    if (!r.valid(id) || index < -1 || index > 255 || r.all_of<CharacterMotor2D>(id)) return nullptr;
    if (index == -1 && !r.all_of<CompoundCollider2D>(id)) return r.try_get<Collider2D>(id);
    auto* compound = r.try_get<CompoundCollider2D>(id);
    if (!compound || r.all_of<Collider2D>(id) || index < 0 || index >= static_cast<int>(compound->shapes.size())) return nullptr;
    return &compound->shapes[static_cast<std::size_t>(index)].value;
}
bool Valid(const Transform2D* t, const Collider2D* c) {
    if (!t || !c || !Finite(t->position) || !Finite(c->offset) || !std::isfinite(t->rotation)
        || !std::isfinite(c->rotation) || !std::isfinite(t->scale.x) || !std::isfinite(t->scale.y)
        || std::abs(t->scale.x) < 1e-9 || std::abs(t->scale.y) < 1e-9) return false;
    switch (c->kind) {
    case ColliderKind2D::Box:
        return std::isfinite(c->halfSize.x) && std::isfinite(c->halfSize.y) && c->halfSize.x > 0 && c->halfSize.y > 0;
    case ColliderKind2D::Circle:
        return std::isfinite(c->radius) && c->radius > 0 && std::abs(std::abs(t->scale.x) - std::abs(t->scale.y)) < 1e-9;
    case ColliderKind2D::Polygon: {
        if (c->points.size() < 3 || c->points.size() > 8) return false;
        double sign = 0;
        for (std::size_t i = 0; i < c->points.size(); ++i) {
            const auto a = c->points[i], b = c->points[(i + 1) % c->points.size()], d = c->points[(i + 2) % c->points.size()];
            if (!Finite(a)) return false;
            const double cross = (b.x - a.x) * (d.y - b.y) - (b.y - a.y) * (d.x - b.x);
            if (!std::isfinite(cross) || std::abs(cross) < 1e-9 || (sign != 0 && sign * cross < 0)) return false;
            sign = cross;
            // Every other point must be strictly on the same side of each edge.
            // Consecutive turns alone also accept some self-intersecting polygons.
            for (std::size_t j = 0; j < c->points.size(); ++j) {
                if (j == i || j == (i + 1) % c->points.size()) continue;
                const auto p = c->points[j];
                const double side = (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
                if (!Finite(p) || side * sign <= 0) return false;
            }
        }
        return true;
    }
    default: return false;
    }
}
pxPoint WorldPoint(const Transform2D& t, const Collider2D& c, pxPoint p) {
    p = Rotate(p, c.rotation);
    p = {(p.x + c.offset.x) * t.scale.x, (p.y + c.offset.y) * t.scale.y};
    p = Rotate(p, t.rotation);
    return {p.x + t.position.x, p.y + t.position.y};
}
pxPoint LocalPoint(const Transform2D& t, const Collider2D& c, pxPoint p) {
    p = Rotate({p.x - t.position.x, p.y - t.position.y}, -t.rotation);
    return {p.x / t.scale.x - c.offset.x, p.y / t.scale.y - c.offset.y};
}
pxPoint ResizePoint(const Collider2D& c) {
    if (c.kind == ColliderKind2D::Box) return {c.halfSize.x, c.halfSize.y};
    if (c.kind == ColliderKind2D::Circle) return {c.radius, 0};
    return *std::max_element(c.points.begin(), c.points.end(), [](auto a, auto b) {
        return std::hypot(a.x, a.y) < std::hypot(b.x, b.y);
    });
}
}
std::optional<ColliderHandles2D> DescribeColliderHandles2D(World& world, entt::entity id, int shape) {
    if (const auto motor = CharacterShape(world, id, shape)) {
        const auto p = motor->transform->position;
        const auto& m = *motor->motor;
        ColliderHandles2D result{p, {p.x + m.radius, p.y}, {}, {p.x, p.y - m.halfSegment - m.radius}, false, true};
        if (!Finite(result.resize) || !Finite(result.height)) return {};
        return result;
    }
    const auto* c = Shape(world, id, shape);
    const auto* t = world.Registry().valid(id) ? world.Registry().try_get<Transform2D>(id) : nullptr;
    if (!Valid(t, c)) return {};
    const auto extent = ResizePoint(*c);
    ColliderHandles2D result{WorldPoint(*t, *c, {}), WorldPoint(*t, *c, extent),
        WorldPoint(*t, *c, {std::hypot(extent.x, extent.y) * 1.4 + 8, 0}), {}, c->kind != ColliderKind2D::Circle, false};
    if (!Finite(result.center) || !Finite(result.resize) || !Finite(result.rotate)) return {};
    return result;
}
bool DragCollider2D(World& world, entt::entity id, int shape, ColliderDrag2D kind, pxPoint pointer, bool snap) {
    if (const auto motor = CharacterShape(world, id, shape)) return DragCharacter(*motor, kind, pointer);
    auto* c = Shape(world, id, shape);
    const auto* t = world.Registry().valid(id) ? world.Registry().try_get<Transform2D>(id) : nullptr;
    if (!Valid(t, c) || !Finite(pointer)) return false;
    auto candidate = *c;
    auto local = LocalPoint(*t, *c, pointer);
    if (!Finite(local)) return false;
    if (kind == ColliderDrag2D::Rotate) {
        if (c->kind == ColliderKind2D::Circle || std::hypot(local.x, local.y) < 1e-6) return false;
        candidate.rotation = std::atan2(local.y, local.x);
        if (snap) {
            constexpr double step = std::numbers::pi / 12;
            candidate.rotation = std::round(candidate.rotation / step) * step;
        }
    } else if (kind == ColliderDrag2D::Resize) {
        local = Rotate(local, -c->rotation);
        // Keep dimensions positive; do not collapse or reflect geometry through its center.
        if (c->kind == ColliderKind2D::Box) candidate.halfSize = {std::max(0.01, std::abs(local.x)), std::max(0.01, std::abs(local.y))};
        else if (c->kind == ColliderKind2D::Circle) candidate.radius = std::max(0.01, std::hypot(local.x, local.y));
        else {
            const auto extent = ResizePoint(*c);
            const double factor = std::hypot(local.x, local.y) / std::hypot(extent.x, extent.y);
            if (!std::isfinite(factor) || factor <= 0) return false;
            for (auto& p : candidate.points) { p.x *= factor; p.y *= factor; }
        }
    } else return false;
    if (!Valid(t, &candidate)) return false;
    const auto extent = ResizePoint(candidate);
    if (std::max(std::abs(extent.x), std::abs(extent.y)) > 1000000) return false;
    *c = candidate;
    return true;
}
}
