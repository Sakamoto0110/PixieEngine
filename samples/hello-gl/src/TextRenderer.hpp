#pragma once

#include "Color.hpp"
#include "Vec2.hpp"

#include "pixie/Gl.hpp"
#include "pixie/Window.hpp"

#include <string_view>

// Draws one line of text with stb_easy_font's quad glyphs. Glyph units are pixels at scale 1.
class TextRenderer {
public:
    bool Create();
    void Destroy();
    // Rebuilds the glyph geometry; cheap enough for text that changes a few times per second.
    void SetText(std::string_view text);
    // widthFraction: share of the viewport width the text spans.
    void DrawCentered(pixie::FramebufferSize viewport, float widthFraction, Color color);
    // topLeft: pixel position of the top-left corner of the text's bounds.
    void DrawAt(pixie::FramebufferSize viewport, Vec2 topLeft, float scale, Color color);

private:
    void Draw(pixie::FramebufferSize viewport, Vec2 origin, float scale, Color color);

    GLuint program_ = 0;
    GLuint vertexArray_ = 0;
    GLuint vertexBuffer_ = 0;
    GLuint indexBuffer_ = 0;
    GLsizei indexCount_ = 0;
    // Bounds of the glyph geometry, in glyph units.
    Vec2 min_;
    Vec2 max_;
};
