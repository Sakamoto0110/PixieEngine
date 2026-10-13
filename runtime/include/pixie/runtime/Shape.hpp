#pragma once

#include "pixie/ShapeInstance.hpp"

#include <pixie/math/pxVec.hpp>
#include <pixie/pxColorRgba.hpp>

namespace pixie {

// How an entity looks: one of the ShapeRenderer's shapes, drawn at its Transform2D. `size` is the
// half extent in world units before the transform's scale.
struct Shape {
    ShapeKind kind = ShapeKind::Circle;
    pxVec2 size{1.0, 1.0};
    pxColorRgba color{255, 255, 255};
};

} // namespace pixie
