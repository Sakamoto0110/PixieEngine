#pragma once

#include "pixie/SpriteRenderer.hpp"
#include "pixie/TextRenderer.hpp"

#include <pixie/math/pxVec.hpp>
#include <pixie/pxPoint.hpp>

#include <array>

namespace pixie {

class Fonts;
struct Text;
struct Transform2D;

// The size of a text's block of lines, in world units before the transform's scale: its widest line
// and its lines' height at its size. One em square when it has no readable font or no characters, so
// it can still be seen as selected and be picked.
pxVec2 TextBlock(const Text& text, Fonts& fonts);

// The corners of a block of `block` world units at `transform`, around the text's pivot: its
// top-left, top-right, bottom-right, and bottom-left, turned and mirrored with the transform.
std::array<pxPoint, 4> TextCorners(const Text& text, const Transform2D& transform, pxVec2 block);

// Whether `point` lies in that block, widened by `tolerance` world units on every side.
bool TextContains(const Text& text, const Transform2D& transform, pxVec2 block, pxPoint point, double tolerance);

// Submits the glyphs of `text` at `transform` to `sprites` on the text's layer, each a quad turned
// and scaled with the transform. The atlas is the ladder's for the text's size on screen, `zoom`
// pixels per world unit. Nothing when its font cannot be read.
void SubmitText(const Text& text, const Transform2D& transform, Fonts& fonts, TextRenderer& renderer,
                SpriteRenderer& sprites, double zoom);

} // namespace pixie
