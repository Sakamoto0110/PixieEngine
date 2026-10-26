#include "pixie/runtime/PhysicsDebug2D.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/World.hpp"
#include "pixie/Camera2D.hpp"
#include "pixie/ShapeRenderer.hpp"
#include <cmath>
#include <numbers>

namespace pixie {
std::vector<PhysicsDebugLine2D> ScenePhysicsDebug2D(World& world, std::uint32_t flags) {
    std::vector<PhysicsDebugLine2D> lines;
    if ((flags & kPhysicsDebugAll2D) == 0) return lines;
    const auto* physics = SceneCollisionWorld2D(world);
    if (!physics) return lines;
    lines = physics->DebugGeometry(flags);
    const auto enabled = [&](PhysicsDebug2D kind) { return (flags & static_cast<std::uint32_t>(kind)) != 0; };
    for (auto [id, motor, transform] : world.Registry().view<const CharacterMotor2D, const Transform2D>().each()) {
        (void)motor;
        const auto* motion = CharacterMotion2D(world, id);
        const auto* settings = CharacterTuning2D(world, id);
        if (!motion || !settings) continue;
        (void)transform;
        const auto p = motion->position;
        if (enabled(PhysicsDebug2D::Colliders)) {
            std::vector<pxPoint> outline;
            for (int i = 0; i <= 16; ++i) {
                const double angle = std::numbers::pi + i * std::numbers::pi / 16;
                outline.emplace_back(p.x + settings->radius * std::cos(angle), p.y - settings->halfSegment + settings->radius * std::sin(angle));
            }
            for (int i = 0; i <= 16; ++i) {
                const double angle = i * std::numbers::pi / 16;
                outline.emplace_back(p.x + settings->radius * std::cos(angle), p.y + settings->halfSegment + settings->radius * std::sin(angle));
            }
            for (std::size_t i = 0; i < outline.size(); ++i)
                lines.push_back({outline[i], outline[(i + 1) % outline.size()], PhysicsDebug2D::Colliders, {80, 240, 170}});
        }
        if (enabled(PhysicsDebug2D::Origins)) {
            lines.push_back({p, {p.x + 6, p.y}, PhysicsDebug2D::Origins, {255, 90, 90}});
            lines.push_back({p, {p.x, p.y + 6}, PhysicsDebug2D::Origins, {90, 255, 90}});
        }
        if (enabled(PhysicsDebug2D::Velocities) && (motion->velocity.x != 0 || motion->velocity.y != 0))
            lines.push_back({p, {p.x + motion->velocity.x * 0.25, p.y + motion->velocity.y * 0.25}, PhysicsDebug2D::Velocities, {100, 220, 255}});
    }
    return lines;
}
void DrawPhysicsDebug2D(World& world, ShapeRenderer& renderer, const Camera2D& camera, std::uint32_t flags) {
    for (const auto& line : ScenePhysicsDebug2D(world, flags)) {
        ShapeInstance edge;
        edge.position = {static_cast<float>((line.a.x + line.b.x) / 2), static_cast<float>((line.a.y + line.b.y) / 2)};
        edge.scale = {static_cast<float>(std::hypot(line.b.x - line.a.x, line.b.y - line.a.y) / 2), static_cast<float>(0.75 / camera.Zoom())};
        edge.rotation = static_cast<float>(std::atan2(line.b.y - line.a.y, line.b.x - line.a.x));
        edge.color = line.color;
        renderer.Submit(ShapeKind::Quad, edge);
    }
}
}
