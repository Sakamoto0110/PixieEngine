#pragma once

#include "pixie/ShapeRenderer.hpp"

namespace pixie {

class World;

// Submits every entity with a Transform2D and a Shape to `renderer`, interpolated `alpha` of the
// way from its previous transform to its current one. Flush the renderer afterwards.
void SubmitShapes(World& world, ShapeRenderer& renderer, double alpha);

} // namespace pixie
