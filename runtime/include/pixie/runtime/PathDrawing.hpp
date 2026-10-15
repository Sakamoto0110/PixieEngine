#pragma once

#include "pixie/PathRenderer.hpp"

#include <pixie/pxPoint.hpp>

#include <vector>

namespace pixie {

class World;
struct Path;
struct Transform2D;

// The line a path draws at `transform`, in world units, with its corners rounded.
std::vector<pxPoint> PathLine(const Path& path, const Transform2D& transform);

// Whether `point` lies within half the path's width of its line, widened by `tolerance` world units.
bool PathContains(const Path& path, const Transform2D& transform, pxPoint point, double tolerance);

// Submits every entity with a Transform2D and a Path to `renderer`, interpolated `alpha` of the way
// from its previous transform to its current one. Flush the renderer afterwards.
void SubmitPaths(World& world, PathRenderer& renderer, double alpha);

} // namespace pixie
