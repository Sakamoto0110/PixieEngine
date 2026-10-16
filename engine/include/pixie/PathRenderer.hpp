#pragma once

#include "pixie/Camera2D.hpp"
#include "pixie/GlBuffer.hpp"
#include "pixie/GlVertexArray.hpp"
#include "pixie/ShaderProgram.hpp"

#include <pixie/pxColorRgba.hpp>
#include <pixie/pxPoint.hpp>

#include <span>
#include <type_traits>
#include <vector>

namespace pixie {

// One straight piece of a path as the GPU reads it, 36 bytes. It is drawn as a capsule: the segment
// widened by halfWidth on every side, with round ends, so consecutive segments join without gaps.
struct PathSegment {
    pxPointf from;
    pxPointf to;
    // Half the line's width, in world units.
    float halfWidth = 1.0f;
    // The path's length up to `from`, in world units.
    float start = 0.0f;
    // The length along the whole path up to which it is drawn in fillColor; beyond, in color.
    float fillTo = 0.0f;
    pxColorRgba color{255, 255, 255};
    pxColorRgba fillColor{255, 255, 255};
};

static_assert(std::is_trivially_copyable_v<PathSegment> && sizeof(PathSegment) == 36);

// The points of a polyline with each inner corner replaced by an arc of `radius` (less where the
// segments beside it are too short for it), so the line turns smoothly. Ends stay where they are.
std::vector<pxPoint> RoundCorners(std::span<const pxPoint> points, double radius);

// The length of a polyline.
double PolylineLength(std::span<const pxPoint> points);

// Draws polylines as antialiased lines with round joins and caps, from instanced quads cut in the
// shader. A line is never thinner than one pixel on screen, so it stays visible when zoomed out.
class PathRenderer {
public:
    // Needs a current GL context; returns false if the shaders do not build.
    bool Create();
    void Destroy();

    // Adds the segments of `points`: `width` wide, in `fillColor` up to `fill` (0 to 1) of its
    // length and in `color` after it.
    void Submit(std::span<const pxPoint> points, double width, pxColorRgba color, pxColorRgba fillColor, double fill);
    // Draws what was submitted since the last flush, seen through `camera`, then forgets it.
    void Flush(const Camera2D& camera);
    // How many segments wait for the next flush.
    std::size_t PendingCount() const { return pending_.size(); }

private:
    ShaderProgram program_;
    GlVertexArray vertexArray_;
    GlBuffer segments_;
    std::vector<PathSegment> pending_;
};

} // namespace pixie
