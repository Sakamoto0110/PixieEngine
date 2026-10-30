#pragma once

#include "pixie/render/Export.hpp"

#include "pixie/render/View.hpp"
#include "pixie/render/gl/GlBuffer.hpp"
#include "pixie/render/gl/GlVertexArray.hpp"
#include "pixie/render/gl/ShaderProgram.hpp"

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
PIXIE_RENDER_API std::vector<pxPoint> RoundCorners(std::span<const pxPoint> points, double radius);

// The length of a polyline.
PIXIE_RENDER_API double PolylineLength(std::span<const pxPoint> points);

// Draws polylines as antialiased lines with round joins and caps, from instanced quads cut in the
// shader. A line is never thinner than one pixel on screen, so it stays visible when zoomed out.
class PathRenderer {
public:
    // Needs a current GL context; returns false if the shaders do not build.
    PIXIE_RENDER_API bool Create();
    PIXIE_RENDER_API void Destroy();

    // Adds the segments of `points`: `width` wide, in `fillColor` up to `fill` (0 to 1) of its
    // length and in `color` after it.
    PIXIE_RENDER_API void Submit(std::span<const pxPoint> points, double width, pxColorRgba color, pxColorRgba fillColor, double fill);
    // Draws what was submitted since the last flush, seen through `view`, then forgets it.
    PIXIE_RENDER_API void Flush(const View& view);
    // How many segments wait for the next flush.
    std::size_t PendingCount() const { return pending_.size(); }

private:
    ShaderProgram program_;
    GlVertexArray vertexArray_;
    GlBuffer segments_;
    std::vector<PathSegment> pending_;
};

} // namespace pixie
