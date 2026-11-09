#pragma once

#include "pixie/runtime/Entity.hpp"
#include "pixie/runtime/Transform2D.hpp"

#include <optional>
#include <span>
#include <string>
#include <vector>

namespace pixie {

enum class ReparentMode2D { KeepWorld, KeepLocal };

// Visual children may be runtime-only or authored. An authored child needs an authored parent
// in a World using the flat/parenting scene family. Mutations run between steps, return an empty
// string on success, and preserve existing state on rejection.
// An empty parent detaches the child. KeepWorld preserves the resolved pose on detach or attach.
PIXIE_RUNTIME_API std::string SetParent2D(Entity child, Entity parent,
                                         ReparentMode2D mode = ReparentMode2D::KeepWorld);
// Reparents the selected branch roots together. Selected descendants retain their edges.
PIXIE_RUNTIME_API std::string SetParents2D(std::span<const Entity> selection, Entity parent,
                                          ReparentMode2D mode, bool* changed = nullptr);
PIXIE_RUNTIME_API Entity Parent2D(Entity child);
PIXIE_RUNTIME_API std::vector<Entity> Children2D(Entity parent);

// Roots use their world pose as local. For a child, edit through this setter rather than writing
// the evaluated Transform2D component directly. A successful edit updates the branch immediately.
PIXIE_RUNTIME_API std::optional<Transform2D> LocalTransform2D(Entity entity);
PIXIE_RUNTIME_API std::string SetLocalTransform2D(Entity entity, const Transform2D& pose);
PIXIE_RUNTIME_API std::string SetWorldTransform2D(Entity entity, const Transform2D& pose);

// Rendering samples each ancestor in its own pose space before composing the branch.
// The entity must have a live Transform2D and alpha must be in [0, 1].
PIXIE_RUNTIME_API Transform2D InterpolatedTransform2D(Entity entity, double alpha);

} // namespace pixie
