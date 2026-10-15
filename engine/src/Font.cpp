#include "pixie/Font.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <format>
#include <fstream>
#include <iterator>
#include <map>

// The implementation is compiled here only. Fonts are read from memory, so stb_truetype never sees
// a file name.
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include <stb_truetype.h>

namespace pixie {

namespace {

constexpr char32_t kReplacement = 0xFFFD;
constexpr int kPadding = 2;
constexpr int kLargestAtlas = 4096;

std::atomic<std::uint64_t> nextFontId{1};

bool IsControl(char32_t c) {
    return c < 0x20 || (c >= 0x7F && c <= 0x9F);
}

} // namespace

struct Font::Data {
    std::vector<std::uint8_t> bytes;
    stbtt_fontinfo info{};
    std::map<int, FontAtlas> atlases;
};

const AtlasGlyph& FontAtlas::Glyph(char32_t codepoint) const {
    if (codepoint >= kFirst && codepoint <= kLast) {
        return glyphs[codepoint - kFirst];
    }
    return glyphs.back();
}

std::vector<char32_t> DecodeUtf8(std::string_view text) {
    std::vector<char32_t> out;
    out.reserve(text.size());
    const auto byte = [&](std::size_t i) { return static_cast<unsigned char>(text[i]); };
    std::size_t i = 0;
    while (i < text.size()) {
        const unsigned char lead = byte(i);
        int length = 0;
        char32_t c = 0;
        char32_t least = 0;
        if (lead < 0x80) {
            out.push_back(lead);
            ++i;
            continue;
        } else if ((lead & 0xE0) == 0xC0) {
            length = 2;
            c = lead & 0x1F;
            least = 0x80;
        } else if ((lead & 0xF0) == 0xE0) {
            length = 3;
            c = lead & 0x0F;
            least = 0x800;
        } else if ((lead & 0xF8) == 0xF0) {
            length = 4;
            c = lead & 0x07;
            least = 0x10000;
        }
        bool valid = length > 0 && i + static_cast<std::size_t>(length) <= text.size();
        for (int k = 1; valid && k < length; ++k) {
            const unsigned char next = byte(i + static_cast<std::size_t>(k));
            valid = (next & 0xC0) == 0x80;
            c = (c << 6) | (next & 0x3F);
        }
        if (valid && c >= least && c <= 0x10FFFF && !(c >= 0xD800 && c <= 0xDFFF)) {
            out.push_back(c);
            i += static_cast<std::size_t>(length);
        } else {
            out.push_back(kReplacement);
            ++i;
        }
    }
    return out;
}

Font::Font(std::unique_ptr<Data> data) : data_(std::move(data)), id_(nextFontId.fetch_add(1)) {}
Font::Font(Font&&) noexcept = default;
Font& Font::operator=(Font&&) noexcept = default;
Font::~Font() = default;

std::optional<Font> Font::FromBytes(std::vector<std::uint8_t> bytes, std::string& error) {
    auto data = std::make_unique<Data>();
    data->bytes = std::move(bytes);
    const unsigned char* raw = data->bytes.data();
    const int offset = data->bytes.empty() ? -1 : stbtt_GetFontOffsetForIndex(raw, 0);
    if (offset < 0 || data->bytes.size() < 12 || stbtt_InitFont(&data->info, raw, offset) == 0) {
        error = "not a TrueType font";
        return std::nullopt;
    }
    if (data->info.glyf == 0) {
        error = "the font's outlines are not in a glyf table (a CFF font cannot be rasterized)";
        return std::nullopt;
    }
    return Font(std::move(data));
}

std::optional<Font> Font::Load(const std::filesystem::path& path, std::string& error) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        error = "the file cannot be opened";
        return std::nullopt;
    }
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return FromBytes(std::move(bytes), error);
}

const FontAtlas& Font::Atlas(int pixelSize) {
    pixelSize = std::clamp(pixelSize, kMinPixelSize, kMaxPixelSize);
    if (const auto found = data_->atlases.find(pixelSize); found != data_->atlases.end()) {
        return found->second;
    }

    constexpr int kCount = static_cast<int>(FontAtlas::kLast - FontAtlas::kFirst + 1) + 1;
    std::vector<int> codepoints;
    for (char32_t c = FontAtlas::kFirst; c <= FontAtlas::kLast; ++c) {
        codepoints.push_back(static_cast<int>(c));
    }
    // A font without U+FFFD gives its .notdef glyph for it.
    codepoints.push_back(static_cast<int>(kReplacement));
    std::vector<stbtt_packedchar> packed(kCount);
    stbtt_pack_range range{};
    range.font_size = STBTT_POINT_SIZE(static_cast<float>(pixelSize));
    range.array_of_unicode_codepoints = codepoints.data();
    range.num_chars = kCount;
    range.chardata_for_range = packed.data();

    std::vector<std::uint8_t> alpha;
    int side = 128;
    for (;; side *= 2) {
        alpha.assign(static_cast<std::size_t>(side) * static_cast<std::size_t>(side), 0);
        stbtt_pack_context context;
        bool done = false;
        if (stbtt_PackBegin(&context, alpha.data(), side, side, 0, kPadding, nullptr) != 0) {
            done = stbtt_PackFontRanges(&context, data_->bytes.data(), 0, &range, 1) != 0;
            stbtt_PackEnd(&context);
        }
        if (done || side >= kLargestAtlas) {
            break;
        }
    }

    FontAtlas atlas;
    atlas.pixelSize = pixelSize;
    const float scale = stbtt_ScaleForMappingEmToPixels(&data_->info, static_cast<float>(pixelSize));
    int ascent = 0, descent = 0, lineGap = 0;
    stbtt_GetFontVMetrics(&data_->info, &ascent, &descent, &lineGap);
    atlas.ascent = static_cast<float>(ascent) * scale;
    atlas.descent = static_cast<float>(-descent) * scale;
    atlas.lineHeight = static_cast<float>(ascent - descent + lineGap) * scale;

    const float inverse = 1.0f / static_cast<float>(side);
    atlas.glyphs.reserve(packed.size());
    for (const stbtt_packedchar& p : packed) {
        atlas.glyphs.push_back({p.xoff, p.yoff, p.xoff2, p.yoff2, p.x0 * inverse, p.y0 * inverse, p.x1 * inverse,
                                p.y1 * inverse, p.xadvance});
    }
    atlas.image.width = side;
    atlas.image.height = side;
    atlas.image.pixels.resize(alpha.size() * 4);
    for (std::size_t i = 0; i < alpha.size(); ++i) {
        std::fill_n(atlas.image.pixels.begin() + static_cast<std::ptrdiff_t>(i * 4), 4, alpha[i]);
    }
    return data_->atlases.emplace(pixelSize, std::move(atlas)).first->second;
}

std::vector<PlacedGlyph> Font::Layout(std::string_view text, int pixelSize) {
    const FontAtlas& atlas = Atlas(pixelSize);
    std::vector<PlacedGlyph> placed;
    float x = 0.0f;
    float baseline = atlas.ascent;
    for (const char32_t c : DecodeUtf8(text)) {
        if (c == U'\n') {
            x = 0.0f;
            baseline += atlas.lineHeight;
            continue;
        }
        if (IsControl(c)) {
            continue;
        }
        const AtlasGlyph& glyph = atlas.Glyph(c);
        placed.push_back({&glyph, x, baseline});
        x += glyph.advance;
    }
    return placed;
}

pxVec2 Font::Measure(std::string_view text, int pixelSize) {
    if (text.empty()) {
        return {0.0, 0.0};
    }
    const FontAtlas& atlas = Atlas(pixelSize);
    double widest = 0.0;
    double x = 0.0;
    int lines = 1;
    for (const char32_t c : DecodeUtf8(text)) {
        if (c == U'\n') {
            widest = std::max(widest, x);
            x = 0.0;
            ++lines;
        } else if (!IsControl(c)) {
            x += atlas.Glyph(c).advance;
        }
    }
    widest = std::max(widest, x);
    return {widest, lines * static_cast<double>(atlas.lineHeight)};
}

} // namespace pixie
