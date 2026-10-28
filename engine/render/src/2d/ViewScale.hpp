#pragma once

#include "pixie/render/View.hpp"
#include <cmath>

namespace pixie {

// The 2D path uses an orthographic view with uniform scale. Perspective renderers need
// a position-dependent projected footprint instead of this world-X pixel density.
inline double PixelsPerWorldUnit(const View& view) {
    const auto& matrix = view.ViewProjection();
    const auto size = view.Viewport();
    return std::hypot(matrix[0][0] * size.width * 0.5, matrix[0][1] * size.height * 0.5);
}

} // namespace pixie
