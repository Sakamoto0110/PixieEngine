#pragma once

#include "pixie/Image.hpp"

#include <pixie/math/pxVec.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pixie {

// One glyph of a FontAtlas: its rectangle in the atlas, and where it is drawn relative to the pen.
struct AtlasGlyph {
    // From the pen, on the baseline, to the glyph's top-left and bottom-right corners, in pixels of
    // the atlas's size, Y down.
    float x0 = 0.0f;
    float y0 = 0.0f;
    float x1 = 0.0f;
    float y1 = 0.0f;
    // Texture coordinates of the top-left and bottom-right corners, v = 0 at the atlas's top.
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 0.0f;
    float v1 = 0.0f;
    // How far the pen moves after the glyph.
    float advance = 0.0f;
};

// A font rasterized at one pixel size: every glyph from U+0020 to U+00FF (Latin-1, so Portuguese,
// Spanish, French, and German text) in one image, white with premultiplied alpha, as the
// SpriteRenderer draws it. Other code points are drawn as the font's replacement glyph.
struct FontAtlas {
    int pixelSize = 0;
    // From the baseline up to the top of the tallest glyph, and down to the bottom of the lowest, in
    // pixels; the distance from one line's baseline to the next.
    float ascent = 0.0f;
    float descent = 0.0f;
    float lineHeight = 0.0f;
    Image image;

    // The glyph drawn for `codepoint`.
    const AtlasGlyph& Glyph(char32_t codepoint) const;

    static constexpr char32_t kFirst = 0x20;
    static constexpr char32_t kLast = 0xFF;
    // The glyphs from kFirst to kLast, then the replacement glyph.
    std::vector<AtlasGlyph> glyphs;
};

// How the lines of a text line up with each other, within the width of the widest.
enum class TextAlign : std::uint8_t { Left, Center, Right };

// A glyph of a laid-out string: which glyph, and where its pen stands on the baseline, in pixels of
// the atlas's size from the top-left of the text.
struct PlacedGlyph {
    const AtlasGlyph* glyph = nullptr;
    float x = 0.0f;
    float baseline = 0.0f;
};

// A TrueType font, read whole from its file. Atlases are rasterized on first use for each pixel size
// and kept; nothing here needs a GL context (TextRenderer uploads them).
//
// Sizes are em sizes, as CSS's font-size: a size of 16 makes the font's em square 16 pixels tall.
// Text is UTF-8; a line break starts a new line, other control characters draw nothing, and bytes
// that are not UTF-8 draw the replacement glyph.
class Font {
public:
    // Reads the font in `bytes`; nothing, with `error` set, when it is not a TrueType font that can
    // be rasterized (only outlines in a `glyf` table are).
    static std::optional<Font> FromBytes(std::vector<std::uint8_t> bytes, std::string& error);
    // FromBytes with the file at `path`.
    static std::optional<Font> Load(const std::filesystem::path& path, std::string& error);

    Font(Font&&) noexcept;
    Font& operator=(Font&&) noexcept;
    ~Font();

    // Different for every Font made, never reused, so a cache of textures can key on it.
    std::uint64_t Id() const { return id_; }

    // The atlas at `pixelSize` (clamped to [kMinPixelSize, kMaxPixelSize]).
    const FontAtlas& Atlas(int pixelSize);
    static constexpr int kMinPixelSize = 4;
    static constexpr int kMaxPixelSize = 256;

    // `text` laid out at `pixelSize`: lines from the top-left, the first baseline at the atlas's
    // ascent, each line aligned within the widest (whose width Measure gives) by `align`.
    std::vector<PlacedGlyph> Layout(std::string_view text, int pixelSize, TextAlign align = TextAlign::Left);
    // The size of `text` at `pixelSize`, in pixels: the widest line's advance, and the lines' height.
    // Empty text measures zero.
    pxVec2 Measure(std::string_view text, int pixelSize);

private:
    struct Data;
    explicit Font(std::unique_ptr<Data> data);

    std::unique_ptr<Data> data_;
    std::uint64_t id_ = 0;
};

// The code points of UTF-8 `text`; each byte that does not start a valid sequence is U+FFFD.
std::vector<char32_t> DecodeUtf8(std::string_view text);

} // namespace pixie
