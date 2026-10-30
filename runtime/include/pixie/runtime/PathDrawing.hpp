#pragma once

#include "pixie/runtime/Export.hpp"

#include "pixie/render/2d/PathRenderer.hpp"

#include <pixie/pxPoint.hpp>
#include <pixie/pxRect.hpp>

#include <vector>

namespace pixie {

class World;
struct Path;
struct Transform2D;

// The line a path draws at `transform`, in world units, with its corners rounded.
PIXIE_RUNTIME_API std::vector<pxPoint> PathLine(const Path& path, const Transform2D& transform);

// Whether `point` lies within half the path's width of its line, widened by `tolerance` world units.
PIXIE_RUNTIME_API bool PathContains(const Path& path, const Transform2D& transform, pxPoint point, double tolerance);

// Submits every entity with a Transform2D and a Path to `renderer`, interpolated `alpha` of the way
// from its previous transform to its current one; with `view`, only those that may show in it
// (PathInView). Flush the renderer afterwards.
PIXIE_RUNTIME_API void SubmitPaths(World& world, PathRenderer& renderer, double alpha, const pxRect* view = nullptr);

} // namespace pixie
