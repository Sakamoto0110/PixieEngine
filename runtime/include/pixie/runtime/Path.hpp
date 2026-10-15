#pragma once

#include <pixie/pxColorRgba.hpp>
#include <pixie/pxPoint.hpp>

#include <vector>

namespace pixie {

// A line through points, drawn at the entity's Transform2D, with its corners rounded: a wire, a
// route, an outline. `fill` paints it in fillColor from its first point up to that fraction of its
// length, and in color after it, so a front can move along it (a signal running down a wire).
struct Path {
    // In the entity's space: moved, turned, and scaled with its transform.
    std::vector<pxPoint> points{pxPoint(0.0, 0.0), pxPoint(64.0, 0.0)};
    // In world units; the transform's scale does not change it.
    double width = 4.0;
    // Radius of the arc that replaces each inner corner, in world units; 0 keeps sharp corners.
    double cornerRadius = 8.0;
    pxColorRgba color{110, 120, 140};
    pxColorRgba fillColor{255, 210, 74};
    // From 0 (all in color) to 1 (all in fillColor).
    double fill = 0.0;
};

} // namespace pixie
