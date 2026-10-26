#pragma once
#include <pixie/pxPoint.hpp>
#include <entt/entity/entity.hpp>
#include <optional>

namespace pixie {
class World;
enum class ColliderDrag2D { Resize, Rotate, Height };
struct ColliderHandles2D {
    pxPoint center, resize, rotate, height;
    bool canRotate = false, canHeight = false;
};
// Authored collider geometry, independent of drawable size and without advancing physics.
// shape = -1 selects Collider2D or an upright CharacterMotor2D; 0..255 selects a compound item.
// Character resize changes radius; Height changes halfSegment to match the full height.
// Missing/invalid geometry has no handles. Polygons resize uniformly; characters cannot rotate.
std::optional<ColliderHandles2D> DescribeColliderHandles2D(World& world, entt::entity entity, int shape);
// Absolute pointer in world coordinates. Resize keeps the local origin/offset fixed.
// Rejected input leaves authored data unchanged. Rotation snaps to 15 degrees when requested.
bool DragCollider2D(World& world, entt::entity entity, int shape, ColliderDrag2D kind,
                    pxPoint pointer, bool snapRotation = false);
}
