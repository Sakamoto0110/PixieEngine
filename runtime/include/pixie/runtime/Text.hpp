#pragma once

#include "pixie/Font.hpp"

#include <pixie/math/pxVec.hpp>
#include <pixie/pxColorRgba.hpp>

#include <string>

namespace pixie {

// Text in the scene: lines drawn at the entity's Transform2D, among the sprites in their layer, and
// saved with the scene. Its size is in world units, so it zooms with the World and scales, turns,
// and mirrors with the transform. (Text of a game's interface, fixed on screen, is drawn by code on
// the screen layer instead: ScreenCanvas::Text.)
struct Text {
    // UTF-8; a line break starts a new line.
    std::string text = "Text";
    // A TrueType font (.ttf), relative to the project folder, with '/'. Without a readable one,
    // nothing is drawn.
    std::string font;
    // The em size, in world units, before the transform's scale.
    double size = 24.0;
    pxColorRgba color{255, 255, 255};
    // How the lines line up within the widest.
    TextAlign align = TextAlign::Left;
    // Where the entity's position falls on the measured block of lines: (0, 0) its top-left, (1, 1)
    // its bottom-right. The text also turns around it.
    pxVec2 pivot{0.5, 0.5};
    // Drawing order among sprites: a higher layer on top; within a layer, the World's order.
    int layer = 0;
};

} // namespace pixie
