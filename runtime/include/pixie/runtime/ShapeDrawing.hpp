#pragma once

#include "pixie/runtime/Export.hpp"

#include "pixie/render/2d/ShapeRenderer.hpp"

#include <pixie/pxRect.hpp>

namespace pixie {

class World;

// Submits every entity with a Transform2D and a Shape to `renderer`, interpolated `alpha` of the
// way from its previous transform to its current one; with `view`, only those that may show in it
// (ShapeInView). Flush the renderer afterwards.
PIXIE_RUNTIME_API void SubmitShapes(World& world, ShapeRenderer& renderer, double alpha, const pxRect* view = nullptr);
// The same shapes, in the same order, for the lighting's receiver pass: each one's color is its
// LightReceiver2D (ReceiverColor). Flush with the renderer's receiver pass on.
PIXIE_RUNTIME_API void SubmitShapeReceivers(World& world, ShapeRenderer& renderer, double alpha,
                                            const pxRect* view = nullptr);
// The same for the emission pass: each one's color is its Emission2D (EmissionColor).
PIXIE_RUNTIME_API void SubmitShapeEmission(World& world, ShapeRenderer& renderer, double alpha,
                                           const pxRect* view = nullptr);

} // namespace pixie
