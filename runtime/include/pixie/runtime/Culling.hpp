#pragma once

#include <pixie/math/pxVec.hpp>
#include <pixie/pxRect.hpp>

namespace pixie {

class Camera2D;
struct Path;
struct Shape;
struct Sprite;
struct Text;
struct Transform2D;

// What a session draws: the camera's visible area, in world units, widened by `pixels` view pixels
// on every side, so what straddles the edge, or moves a little before the next frame, is drawn.
pxRect PaddedView(const Camera2D& camera, double pixels);

// Whether something drawn at `transform` may show in `view`. Each bound is conservative: a circle
// that holds the sprite (of `pixelSize`, its image's size), the text (of `block`, as TextBlock
// measures it), or the shape however each is turned and pivoted; the box of a path's points, widened
// by half its width. So something that shows is never skipped; something just outside may be drawn.
bool SpriteInView(const Sprite& sprite, const Transform2D& transform, pxVec2 pixelSize, const pxRect& view);
bool TextInView(const Text& text, const Transform2D& transform, pxVec2 block, const pxRect& view);
bool PathInView(const Path& path, const Transform2D& transform, const pxRect& view);
bool ShapeInView(const Shape& shape, const Transform2D& transform, const pxRect& view);

} // namespace pixie
