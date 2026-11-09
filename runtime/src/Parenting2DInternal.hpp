#pragma once

#include "pixie/runtime/Entity.hpp"

#include <span>
#include <string>

namespace pixie::detail {

struct AuthoredParentLink2D {
    Entity child;
    Entity parent;
};

// Called after a scene reader constructs its entities with authored local poses.
std::string RestoreParenting2D(World& world, std::span<const AuthoredParentLink2D> links);

void RefreshParenting2D(World& world);
void CaptureParenting2D(World& world);
void RemoveParenting2D(Entity entity);
void ResetBranchInterpolation2D(Entity root);

} // namespace pixie::detail
