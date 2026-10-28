#pragma once

#include "pixie/render/2d/Camera2D.hpp"
#include "pixie/render/2d/PathRenderer.hpp"
#include "pixie/render/2d/ShapeRenderer.hpp"
#include "pixie/render/2d/SpriteRenderer.hpp"
#include "pixie/render/2d/TextRenderer.hpp"

#include <pixie/math/pxVec.hpp>
#include <pixie/pxColorRgba.hpp>
#include <pixie/pxPoint.hpp>

#include <span>
#include <string_view>

namespace pixie {

class Fonts;
class SpriteImages;

// How text is drawn: the font's path, as an image's (relative to the project folder), its size in
// pixels (the em size, as CSS's font-size; rounded to whole pixels), and its color.
struct TextStyle {
    std::string_view font;
    double size = 16.0;
    pxColorRgba color{255, 255, 255};
};

// What a project draws over its World, in pixels of the view: the origin at the top-left, Y down, and
// no camera, so it stays put however the World's camera moves. A game's interface goes here (a
// hotbar, tooltips, a menu), and is drawn the same in its player and in an editor's viewport. The
// cursor's position (Input) is in the same pixels, so deciding what it is over stays with the game.
//
// Whatever is drawn is drawn in a fixed order: shapes, then paths, then sprites and text together,
// in the order they were drawn, so panels lie under the icons and labels on them.
class ScreenCanvas {
public:
    ScreenCanvas(const Camera2D& worldCamera, ShapeRenderer& shapes, PathRenderer& paths, SpriteRenderer& sprites,
                 TextRenderer& text, SpriteImages& images, Fonts& fonts);

    // The view's size, in pixels.
    pxVec2 Size() const;
    // Where a point of the World is on the screen, and the reverse, through the World's camera: to
    // pin a label to something in the World.
    pxPoint WorldToScreen(pxPoint world) const { return worldCamera_.WorldToScreen(world); }
    pxPoint ScreenToWorld(pxPoint screen) const { return worldCamera_.ScreenToWorld(screen); }

    // A filled rectangle from its top-left corner.
    void Rect(pxPoint topLeft, pxVec2 size, pxColorRgba color);
    // A filled disc, and a circle's outline.
    void Circle(pxPoint center, double radius, pxColorRgba color);
    void Ring(pxPoint center, double radius, pxColorRgba color);
    // Any shape, in pixels.
    void Shape(ShapeKind kind, const ShapeInstance& instance);

    // The image at `image`, as a Sprite names it, from its top-left corner at `size` pixels; without
    // a size, at the image's own. `tint` multiplies it.
    void Sprite(std::string_view image, pxPoint topLeft, pxVec2 size, pxColorRgba tint = {255, 255, 255},
                SpriteFilter filter = SpriteFilter::Smooth);
    void Sprite(std::string_view image, pxPoint topLeft);

    // A polyline `width` pixels wide, in `fillColor` up to `fill` (0 to 1) of its length and in
    // `color` after it, as a Path draws.
    void Path(std::span<const pxPoint> points, double width, pxColorRgba color, pxColorRgba fillColor = {},
              double fill = 0.0);

    // `text` from the top-left of its first line: UTF-8, left-aligned, a line break starting a new
    // line. Nothing is drawn when the style's font cannot be read (it is logged once).
    void Text(std::string_view text, pxPoint topLeft, const TextStyle& style);
    // The size `text` takes in `style`, in pixels: its widest line, and its lines' height; zero when
    // the font cannot be read.
    pxVec2 MeasureText(std::string_view text, const TextStyle& style);

    // The camera the screen layer is drawn through: one unit per pixel, the origin at the top-left.
    static Camera2D PixelCamera(FramebufferSize viewport);

private:
    const Camera2D& worldCamera_;
    ShapeRenderer& shapes_;
    PathRenderer& paths_;
    SpriteRenderer& sprites_;
    TextRenderer& text_;
    SpriteImages& images_;
    Fonts& fonts_;
};

} // namespace pixie
