#pragma once

#include "pixie/runtime/Entity.hpp"
#include "pixie/runtime/Transform2D.hpp"

#include <optional>
#include <span>
#include <string>
#include <vector>

namespace pixie {

enum class ReparentMode2D { KeepWorld, KeepLocal };

/// Attaches a visual child to a live parent in the same World and scope. Both need Transform2D.
/// Every parent pose must be finite with positive uniform scale. Children cannot own independent
/// movement or collider components. Authored children require authored parents in the unscoped
/// v1/v3 family and persist as v3. An empty parent detaches the child. KeepWorld preserves the
/// evaluated pose. KeepLocal preserves its local pose, using the current world pose for roots.
/// Call between steps or through World::Defer. Empty string means success. Rejection leaves poses
/// and links intact.
PIXIE_RUNTIME_API std::string SetParent2D(Entity child, Entity parent,
                                         ReparentMode2D mode = ReparentMode2D::KeepWorld);
/// Reparents selected branch roots as a batch. Selected descendants retain their links. When
/// supplied, changed reports whether a successful call actually changed any links.
PIXIE_RUNTIME_API std::string SetParents2D(std::span<const Entity> selection, Entity parent,
                                          ReparentMode2D mode, bool* changed = nullptr);
PIXIE_RUNTIME_API Entity Parent2D(Entity child);
PIXIE_RUNTIME_API std::vector<Entity> Children2D(Entity parent);

/// Returns the authored local pose by value, using the world pose for a root. An entity without a
/// live Transform2D returns no value.
PIXIE_RUNTIME_API std::optional<Transform2D> LocalTransform2D(Entity entity);
/// Changes the authoritative pose and immediately refreshes the branch. Uses SetParent2D's
/// mutation phase and error convention. An attached Transform2D is the evaluated output.
PIXIE_RUNTIME_API std::string SetLocalTransform2D(Entity entity, const Transform2D& pose);
/// Converts a world-space pose to local space for a child and applies the same validation.
PIXIE_RUNTIME_API std::string SetWorldTransform2D(Entity entity, const Transform2D& pose);

/// Samples each ancestor in its own pose space before composing the branch. Requires a live
/// Transform2D and alpha in [0, 1].
PIXIE_RUNTIME_API Transform2D InterpolatedTransform2D(Entity entity, double alpha);

} // namespace pixie
