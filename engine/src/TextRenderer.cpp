#include "pixie/TextRenderer.hpp"

#include <array>
#include <cassert>
#include <cmath>

namespace pixie {

namespace {

constexpr std::array kLadder{8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256};

} // namespace

TextRenderer::~TextRenderer() {
    assert(textures_.empty() && "Destroy the text renderer while the GL context is current");
}

int TextRenderer::LadderSize(double pixels) {
    for (const int size : kLadder) {
        if (size >= pixels) {
            return size;
        }
    }
    return kLadder.back();
}

const Texture& TextRenderer::TextureOf(Font& font, const FontAtlas& atlas) {
    Texture& texture = textures_[{font.Id(), atlas.pixelSize}];
    if (!texture.IsValid()) {
        texture.Create(atlas.image);
    }
    return texture;
}

void TextRenderer::SubmitPixels(SpriteRenderer& sprites, Font& font, std::string_view text, pxPoint at,
                                int pixelSize, pxColorRgba color, int layer) {
    const FontAtlas& atlas = font.Atlas(pixelSize);
    const unsigned texture = TextureOf(font, atlas).Id();
    const double left = std::round(at.x);
    const double top = std::round(at.y);
    for (const PlacedGlyph& placed : font.Layout(text, pixelSize)) {
        const AtlasGlyph& g = *placed.glyph;
        if (g.x1 <= g.x0 || g.y1 <= g.y0) {
            continue;
        }
        SpriteInstance instance;
        instance.position = pxPointf(static_cast<float>(left + std::round(placed.x) + g.x0),
                                     static_cast<float>(top + std::round(placed.baseline) + g.y0));
        instance.size = {g.x1 - g.x0, g.y1 - g.y0};
        instance.pivot = {0.0f, 0.0f};
        instance.uv = {g.u0, g.v0, g.u1, g.v1};
        instance.color = color;
        sprites.Submit(texture, SpriteFilter::Smooth, layer, instance);
    }
}

void TextRenderer::SubmitWorld(SpriteRenderer& sprites, Font& font, std::string_view text, pxPoint at, double size,
                               double zoom, pxColorRgba color, int layer) {
    const int pixelSize = LadderSize(size * zoom);
    const FontAtlas& atlas = font.Atlas(pixelSize);
    const unsigned texture = TextureOf(font, atlas).Id();
    const double scale = size / atlas.pixelSize;
    for (const PlacedGlyph& placed : font.Layout(text, pixelSize)) {
        const AtlasGlyph& g = *placed.glyph;
        if (g.x1 <= g.x0 || g.y1 <= g.y0) {
            continue;
        }
        SpriteInstance instance;
        instance.position = pxPointf(static_cast<float>(at.x + (placed.x + g.x0) * scale),
                                     static_cast<float>(at.y + (placed.baseline + g.y0) * scale));
        instance.size = {static_cast<float>((g.x1 - g.x0) * scale), static_cast<float>((g.y1 - g.y0) * scale)};
        instance.pivot = {0.0f, 0.0f};
        instance.uv = {g.u0, g.v0, g.u1, g.v1};
        instance.color = color;
        sprites.Submit(texture, SpriteFilter::Smooth, layer, instance);
    }
}

void TextRenderer::Forget(const Font& font) {
    for (auto it = textures_.begin(); it != textures_.end();) {
        if (it->first.first == font.Id()) {
            it->second.Destroy();
            it = textures_.erase(it);
        } else {
            ++it;
        }
    }
}

void TextRenderer::Destroy() {
    for (auto& [key, texture] : textures_) {
        texture.Destroy();
    }
    textures_.clear();
}

} // namespace pixie
