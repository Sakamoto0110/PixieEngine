#pragma once

#include "pixie/SpriteRenderer.hpp"

#include <pixie/math/pxVec.hpp>
#include <pixie/pxColorRgba.hpp>
#include <pixie/pxRect.hpp>

#include <string>

namespace pixie {

// How an entity looks as an image: a PNG, or a part of one, drawn at its Transform2D. One image
// pixel is one world unit, times the transform's scale, so a sprite has no size of its own.
struct Sprite {
    // The PNG, relative to the project folder, with '/' between folders.
    std::string image;
    // The part of the image drawn, in pixels from its top-left; an empty one (zero width or height)
    // is the whole image.
    pxRect region{0.0, 0.0, 0.0, 0.0};
    // Where the entity's position falls within the drawn rectangle: (0, 0) at its top-left, (1, 1)
    // at its bottom-right. The entity also turns around it.
    pxVec2 pivot{0.5, 0.5};
    // Multiplies the image, alpha included.
    pxColorRgba color{255, 255, 255};
    // Drawing order: a higher layer on top; within a layer, the World's order.
    int layer = 0;
    SpriteFilter filter = SpriteFilter::Smooth;
};

} // namespace pixie
