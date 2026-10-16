#include "pixie/runtime/TextDrawing.hpp"

#include "pixie/runtime/Fonts.hpp"
#include "pixie/runtime/Sprite.hpp"
#include "pixie/runtime/SpriteDrawing.hpp"
#include "pixie/runtime/Text.hpp"
#include "pixie/runtime/Transform2D.hpp"

#include <algorithm>
#include <cmath>

namespace pixie {

namespace {

// Blocks are measured at one atlas size and scaled: advances grow linearly with the size, so the
// block is the same at any size, wherever it is drawn.
constexpr int kMeasureSize = 64;

// A text's block is a rectangle around its pivot, as a sprite's is.
Sprite AsRectangle(const Text& text) {
    Sprite rectangle;
    rectangle.pivot = text.pivot;
    return rectangle;
}

} // namespace

pxVec2 TextBlock(const Text& text, Fonts& fonts) {
    if (Font* font = fonts.Find(text.font)) {
        const pxVec2 measured = font->Measure(text.text, kMeasureSize) * (text.size / kMeasureSize);
        if (measured.x > 0.0 && measured.y > 0.0) {
            return measured;
        }
    }
    return pxVec2(text.size, text.size);
}

std::array<pxPoint, 4> TextCorners(const Text& text, const Transform2D& transform, pxVec2 block) {
    return SpriteCorners(AsRectangle(text), transform, block);
}

bool TextContains(const Text& text, const Transform2D& transform, pxVec2 block, pxPoint point, double tolerance) {
    return SpriteContains(AsRectangle(text), transform, block, point, tolerance);
}

void SubmitText(const Text& text, const Transform2D& transform, Fonts& fonts, TextRenderer& renderer,
                SpriteRenderer& sprites, double zoom) {
    Font* font = fonts.Find(text.font);
    if (font == nullptr || text.text.empty() || text.size <= 0.0) {
        return;
    }
    const double scaleOnScreen = std::max(std::abs(transform.scale.x), std::abs(transform.scale.y));
    const int pixelSize = TextRenderer::LadderSize(text.size * scaleOnScreen * zoom);
    const unsigned texture = renderer.AtlasTexture(*font, pixelSize);
    // World units, before the transform's scale, per pixel of the atlas.
    const double k = text.size / pixelSize;
    const pxVec2 block = font->Measure(text.text, pixelSize) * k;
    const double originX = -text.pivot.x * block.x;
    const double originY = -text.pivot.y * block.y;
    const double c = std::cos(transform.rotation);
    const double s = std::sin(transform.rotation);

    for (const PlacedGlyph& placed : font->Layout(text.text, pixelSize, text.align)) {
        const AtlasGlyph& g = *placed.glyph;
        if (g.x1 <= g.x0 || g.y1 <= g.y0) {
            continue;
        }
        // The glyph's top-left in the entity's space, then scaled, turned, and moved with it.
        const double x = (originX + (placed.x + g.x0) * k) * transform.scale.x;
        const double y = (originY + (placed.baseline + g.y0) * k) * transform.scale.y;
        SpriteInstance instance;
        instance.position = pxPointf(static_cast<float>(transform.position.x + c * x - s * y),
                                     static_cast<float>(transform.position.y + s * x + c * y));
        instance.rotation = static_cast<float>(transform.rotation);
        instance.size = {static_cast<float>((g.x1 - g.x0) * k * transform.scale.x),
                         static_cast<float>((g.y1 - g.y0) * k * transform.scale.y)};
        instance.pivot = {0.0f, 0.0f};
        instance.uv = {g.u0, g.v0, g.u1, g.v1};
        instance.color = text.color;
        sprites.Submit(texture, SpriteFilter::Smooth, text.layer, instance);
    }
}

} // namespace pixie
