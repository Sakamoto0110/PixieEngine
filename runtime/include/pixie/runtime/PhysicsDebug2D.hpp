#pragma once
#include <pixie/pxPoint.hpp>
#include <pixie/pxColorRgba.hpp>
#include <cstdint>
#include <vector>

namespace pixie {
class World;
class ShapeRenderer;
class Camera2D;
enum class PhysicsDebug2D : std::uint32_t {
    Colliders = 1, Origins = 2, Centers = 4, Velocities = 8,
    Sensors = 16, Awake = 32, Contacts = 64
};
inline constexpr std::uint32_t kPhysicsDebugAll2D = 127;
struct PhysicsDebugLine2D {
    pxPoint a, b;
    PhysicsDebug2D kind;
    pxColorRgba color;
};
// Copied latest native state; no sync, stepping or authored mutation. Disabled bodies are omitted.
// Sensors have a separate switch. Contact normals include speculative manifold points.
std::vector<PhysicsDebugLine2D> ScenePhysicsDebug2D(World& world, std::uint32_t flags);
void DrawPhysicsDebug2D(World& world, ShapeRenderer& renderer, const Camera2D& camera, std::uint32_t flags);
}
