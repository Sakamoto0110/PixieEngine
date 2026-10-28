#pragma once

#include "pixie/render/Export.hpp"

#include "pixie/render/2d/Font.hpp"
#include "pixie/render/2d/SpriteRenderer.hpp"
#include "pixie/render/gl/Texture.hpp"

#include <pixie/pxColorRgba.hpp>
#include <pixie/pxPoint.hpp>

#include <cstdint>
#include <map>
#include <string_view>
#include <utility>

namespace pixie {

// Draws text as sprites, one per glyph, through a SpriteRenderer: the glyphs come from a Font's
// atlases, uploaded here as textures on first use. Text is placed by the top-left of its first
// line, Y down, and drawn in a color with straight alpha.
//
// Owns its textures: Destroy it while the GL context is still current.
class TextRenderer {
public:
    TextRenderer() = default;
    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;
    PIXIE_RENDER_API ~TextRenderer();

    // Text at exactly `pixelSize`, with every glyph on whole pixels, for a camera that maps one unit
    // to one pixel, such as a screen layer's: crisp at any size.
    PIXIE_RENDER_API void SubmitPixels(SpriteRenderer& sprites, Font& font, std::string_view text, pxPoint at, int pixelSize,
                      pxColorRgba color, int layer = 0);
    // Text `size` world units tall (its em size), for a camera of `zoom` pixels per unit. The atlas
    // is the one at LadderSize(size * zoom), scaled to fit, so zooming makes few atlases.
    PIXIE_RENDER_API void SubmitWorld(SpriteRenderer& sprites, Font& font, std::string_view text, pxPoint at, double size, double zoom,
                     pxColorRgba color, int layer = 0);

    // The texture of `font`'s atlas at `pixelSize`, uploaded on first use, for a caller that places
    // the glyphs itself (a text turned with its entity).
    PIXIE_RENDER_API unsigned AtlasTexture(Font& font, int pixelSize);

    // The smallest of 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, and 256 not below `pixels`; 256 above.
    PIXIE_RENDER_API static int LadderSize(double pixels);

    // Releases the textures of `font`, as when it is dropped.
    PIXIE_RENDER_API void Forget(const Font& font);
    // Releases every texture; they are uploaded again when next used.
    PIXIE_RENDER_API void Destroy();

private:
    PIXIE_RENDER_API const Texture& TextureOf(Font& font, const FontAtlas& atlas);

    std::map<std::pair<std::uint64_t, int>, Texture> textures_;
};

} // namespace pixie
