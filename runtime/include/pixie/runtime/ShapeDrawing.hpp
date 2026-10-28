#pragma once

#include "pixie/render/2d/ShapeRenderer.hpp"

#include <pixie/pxRect.hpp>

namespace pixie {

class World;

// Submits every entity with a Transform2D and a Shape to `renderer`, interpolated `alpha` of the
// way from its previous transform to its current one; with `view`, only those that may show in it
// (ShapeInView). Flush the renderer afterwards.
void SubmitShapes(World& world, ShapeRenderer& renderer, double alpha, const pxRect* view = nullptr);

} // namespace pixie
