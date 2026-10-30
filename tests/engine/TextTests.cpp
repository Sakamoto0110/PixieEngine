#include "Check.hpp"
#include "pixie/render/2d/Camera2D.hpp"

#include "pixie/render/2d/Font.hpp"
#include "pixie/render/gl/GlLoader.hpp"
#include "pixie/render/RenderTarget.hpp"
#include "pixie/render/2d/SpriteRenderer.hpp"
#include "pixie/render/2d/TextRenderer.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>
#include <vector>

using pixie::DecodeUtf8;
using pixie::Font;

namespace {

// Inter Regular, from the shared resources, as the particles sample has it.
std::optional<Font> LoadInter() {
    std::string error;
    std::optional<Font> font = Font::Load(PIXIE_TEST_FONT, error);
    if (!font) {
        std::fprintf(stderr, "cannot load %s: %s\n", PIXIE_TEST_FONT, error.c_str());
    }
    return font;
}

bool Near(double a, double b, double tolerance) {
    return std::abs(a - b) <= tolerance;
}

} // namespace

TEST(Utf8DecodesCodePointsAndReplacesWhatIsNotUtf8) {
    CHECK((DecodeUtf8("A\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80") == std::vector<char32_t>{U'A', 0xE9, 0x20AC, 0x1F600}));
    // A lead byte with nothing after it, an overlong encoding, and a surrogate.
    CHECK((DecodeUtf8("\xC3") == std::vector<char32_t>{0xFFFD}));
    CHECK((DecodeUtf8("\xC0\xAF") == std::vector<char32_t>{0xFFFD, 0xFFFD}));
    CHECK((DecodeUtf8("\xED\xA0\x80!") == std::vector<char32_t>{0xFFFD, 0xFFFD, 0xFFFD, U'!'}));
    CHECK(DecodeUtf8("").empty());
}

TEST(AFileThatIsNotAFontIsRefused) {
    std::string error;
    CHECK(!Font::FromBytes({}, error) && !error.empty());
    error.clear();
    CHECK(!Font::FromBytes(std::vector<std::uint8_t>(64, 0x41), error) && !error.empty());
    error.clear();
    CHECK(!Font::Load("no such font.ttf", error) && !error.empty());
}

TEST(TextIsMeasuredByItsAdvancesAndLines) {
    std::optional<Font> font = LoadInter();
    CHECK(font.has_value());
    if (!font) {
        return;
    }
    const pixie::FontAtlas& atlas = font->Atlas(20);
    CHECK(atlas.pixelSize == 20 && atlas.lineHeight > 20.0f && atlas.ascent > 0.0f && atlas.descent > 0.0f);

    const pxVec2 empty = font->Measure("", 20);
    CHECK(empty.x == 0.0 && empty.y == 0.0);
    const pxVec2 one = font->Measure("A", 20);
    CHECK(Near(one.x, atlas.Glyph(U'A').advance, 1e-6) && one.x > 5.0 && Near(one.y, atlas.lineHeight, 1e-6));
    const pxVec2 two = font->Measure("AA", 20);
    CHECK(Near(two.x, 2.0 * one.x, 1e-4) && Near(two.y, one.y, 1e-6));
    const pxVec2 lines = font->Measure("A\nAA", 20);
    CHECK(Near(lines.x, two.x, 1e-4) && Near(lines.y, 2.0 * atlas.lineHeight, 1e-4));
    // Control characters draw nothing.
    CHECK(Near(font->Measure("A\tA\r", 20).x, two.x, 1e-4));
    // Twice the size, twice the width, within a pixel of rounding.
    CHECK(Near(font->Measure("Pixie", 40).x, 2.0 * font->Measure("Pixie", 20).x, 2.0));

    // Latin-1 has its own glyphs; beyond it, the replacement glyph.
    const pixie::AtlasGlyph& cedilla = atlas.Glyph(0xE7);
    CHECK(&cedilla != &atlas.Glyph(0x20AC) && cedilla.y1 > atlas.Glyph(U'c').y1);
    CHECK(&atlas.Glyph(0x20AC) == &atlas.glyphs.back() && &atlas.Glyph(0x1F600) == &atlas.glyphs.back());
}

TEST(AtlasesHoldPremultipliedWhiteGlyphs) {
    std::optional<Font> font = LoadInter();
    if (!font) {
        CHECK(false);
        return;
    }
    CHECK(font->Atlas(1).pixelSize == Font::kMinPixelSize && font->Atlas(9999).pixelSize == Font::kMaxPixelSize);
    const pixie::FontAtlas& atlas = font->Atlas(24);
    CHECK(&font->Atlas(24) == &atlas);
    const pixie::AtlasGlyph& h = atlas.Glyph(U'H');
    CHECK(h.x1 > h.x0 && h.y1 > h.y0 && h.y0 < 0.0f && h.u0 >= 0.0f && h.u1 <= 1.0f && h.v0 >= 0.0f && h.v1 <= 1.0f);
    const pixie::AtlasGlyph& space = atlas.Glyph(U' ');
    CHECK(space.advance > 0.0f);

    bool covered = false;
    bool premultiplied = true;
    const std::vector<std::uint8_t>& pixels = atlas.image.pixels;
    for (std::size_t i = 0; i < pixels.size(); i += 4) {
        covered = covered || pixels[i + 3] == 255;
        premultiplied = premultiplied && pixels[i] == pixels[i + 3] && pixels[i + 1] == pixels[i + 3] &&
                        pixels[i + 2] == pixels[i + 3];
    }
    CHECK(covered && premultiplied);

    // The pens of a layout advance by each glyph, and a line break returns to the left a line lower.
    const std::vector<pixie::PlacedGlyph> placed = font->Layout("HH\nH", 24);
    CHECK(placed.size() == 3);
    if (placed.size() == 3) {
        CHECK(placed[0].x == 0.0f && Near(placed[0].baseline, atlas.ascent, 1e-6));
        CHECK(Near(placed[1].x, h.advance, 1e-6) && placed[1].baseline == placed[0].baseline);
        CHECK(placed[2].x == 0.0f && Near(placed[2].baseline, atlas.ascent + atlas.lineHeight, 1e-4));
    }
}

TEST(TheLadderBoundsTheAtlasesOfWorldText) {
    using pixie::TextRenderer;
    CHECK(TextRenderer::LadderSize(0.5) == 8 && TextRenderer::LadderSize(8.0) == 8 && TextRenderer::LadderSize(8.01) == 12);
    CHECK(TextRenderer::LadderSize(100.0) == 128 && TextRenderer::LadderSize(1e6) == 256);
}

// Text drawn into a render target lands where its layout says, and nowhere else.
TEST(TextIsDrawnWhereItsGlyphsAre) {
    std::optional<Font> font = LoadInter();
    if (!font || !glfwInit()) {
        CHECK(false);
        return;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "engine-tests", nullptr, nullptr);
    CHECK(window != nullptr);
    if (window == nullptr) {
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(window);
    CHECK(pixie::LoadGl());

    constexpr int kSide = 96;
    pixie::RenderTarget target;
    pixie::SpriteRenderer sprites;
    pixie::TextRenderer text;
    CHECK(target.Create({kSide, kSide}) && sprites.Create());
    pixie::Camera2D camera;
    camera.SetViewport({kSide, kSide});
    camera.SetCenter(pxPoint(kSide * 0.5, kSide * 0.5));

    target.Bind();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    text.SubmitPixels(sprites, *font, "H", pxPoint(20.0, 30.0), 32, pxColorRgba(255, 255, 255));
    sprites.Flush(camera.MakeView());
    pixie::RenderTarget::BindDefault();

    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kSide) * kSide * 4);
    glGetTextureImage(target.ColorTexture(), 0, GL_RGBA, GL_UNSIGNED_BYTE, static_cast<GLsizei>(pixels.size()),
                      pixels.data());

    // The glyph's quad, in pixels from the top-left, as SubmitPixels places it.
    const pixie::FontAtlas& atlas = font->Atlas(32);
    const pixie::AtlasGlyph& h = atlas.Glyph(U'H');
    const double left = 20.0 + h.x0;
    const double top = 30.0 + std::round(atlas.ascent) + h.y0;
    int lit = 0;
    int outside = 0;
    for (int y = 0; y < kSide; ++y) {
        for (int x = 0; x < kSide; ++x) {
            // The texture's first row is the image's bottom.
            const std::uint8_t red = pixels[(static_cast<std::size_t>(kSide - 1 - y) * kSide + x) * 4];
            if (red < 128) {
                continue;
            }
            ++lit;
            const bool inside = x + 0.5 > left && x + 0.5 < left + (h.x1 - h.x0) && y + 0.5 > top &&
                                y + 0.5 < top + (h.y1 - h.y0);
            outside += inside ? 0 : 1;
        }
    }
    std::fprintf(stderr, "text pixels: %d lit, %d outside the glyph\n", lit, outside);
    CHECK(lit > 60 && outside == 0);

    text.Destroy();
    sprites.Destroy();
    target.Destroy();
    glfwDestroyWindow(window);
    glfwTerminate();
}

TEST(LinesAreAlignedWithinTheWidest) {
    std::optional<Font> font = LoadInter();
    CHECK(font.has_value());
    if (!font) {
        return;
    }
    const std::string text = "Wide line\nab";
    const double wide = font->Measure("Wide line", 32).x;
    const double narrow = font->Measure("ab", 32).x;
    CHECK(Near(font->Measure(text, 32).x, wide, 1e-3));
    // The second line starts at "ab"'s first glyph, after the 9 glyphs of the first.
    const auto start = [&](pixie::TextAlign align) { return font->Layout(text, 32, align)[9].x; };
    CHECK(Near(start(pixie::TextAlign::Left), 0.0, 1e-3));
    CHECK(Near(start(pixie::TextAlign::Center), (wide - narrow) / 2.0, 1e-3));
    CHECK(Near(start(pixie::TextAlign::Right), wide - narrow, 1e-3));
    // The widest line does not move.
    CHECK(Near(font->Layout(text, 32, pixie::TextAlign::Right)[0].x, 0.0, 1e-3));
}
