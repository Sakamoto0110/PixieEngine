#pragma once

#include "pixie/SpriteRenderer.hpp"

#include <entt/entity/entity.hpp>

#include <pixie/math/pxVec.hpp>
#include <pixie/pxPoint.hpp>

#include <array>
#include <functional>
#include <optional>

namespace pixie {

class SpriteImages;
class World;
struct Sprite;
struct Transform2D;

// The size of what a sprite draws, in image pixels, before the transform's scale: its region's when
// it has one, else its image's, else SpriteImages::kMissingSize on both axes.
pxVec2 SpritePixelSize(const Sprite& sprite, SpriteImages& images);

// A point of a sprite of `pixelSize` at `transform`, given in the sprite's own units (0 to 1 across
// the drawn rectangle, Y down: (0, 0) its top-left, (1, 1) its bottom-right), in world units. It
// follows the transform's rotation and mirroring, so an anchor drawn on the image (a pin of a logic
// gate) stays on it however the entity is turned.
pxPoint SpritePoint(const Sprite& sprite, const Transform2D& transform, pxVec2 pixelSize, pxPoint uv);

// The inverse: a world point in the sprite's own units; nothing when the sprite has no area (a
// scale of 0 on an axis).
std::optional<pxPoint> SpriteLocal(const Sprite& sprite, const Transform2D& transform, pxVec2 pixelSize,
                                   pxPoint world);

// The corners of the rectangle a sprite of `pixelSize` covers at `transform`, in world units: the
// image's top-left, top-right, bottom-right, and bottom-left, in that order.
std::array<pxPoint, 4> SpriteCorners(const Sprite& sprite, const Transform2D& transform, pxVec2 pixelSize);

// Whether `point` lies in that rectangle, widened by `tolerance` world units on every side.
bool SpriteContains(const Sprite& sprite, const Transform2D& transform, pxVec2 pixelSize, pxPoint point,
                    double tolerance);

// Calls `visit` for every entity with a Transform2D and a Sprite, in the order they are drawn:
// lower layers first, and the World's order (oldest first) within a layer.
void ForEachSpriteInDrawOrder(World& world,
                              const std::function<void(entt::entity, const Transform2D&, const Sprite&)>& visit);

// Submits every entity with a Transform2D and a Sprite to `renderer`, interpolated `alpha` of the
// way from its previous transform to its current one, with its image from `images`. Flush the
// renderer afterwards.
void SubmitSprites(World& world, SpriteRenderer& renderer, SpriteImages& images, double alpha);

} // namespace pixie
