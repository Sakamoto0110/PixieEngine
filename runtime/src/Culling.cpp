#include "pixie/runtime/Culling.hpp"

#include "pixie/Camera2D.hpp"
#include "pixie/runtime/Path.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Sprite.hpp"
#include "pixie/runtime/Text.hpp"
#include "pixie/runtime/Transform2D.hpp"

#include <algorithm>
#include <cmath>

namespace pixie {

namespace {

// Whether a circle of `radius` around `center` meets `view`.
bool CircleMeets(pxPoint center, double radius, const pxRect& view) {
    const double x = std::clamp(center.x, view.x, view.Right());
    const double y = std::clamp(center.y, view.y, view.Bottom());
    return std::hypot(center.x - x, center.y - y) <= radius;
}

// The farthest a rectangle of `size`, around a pivot anywhere in it, reaches from the pivot, scaled
// by `scale`: its diagonal, which holds every pivot and every turn.
double Reach(pxVec2 size, pxVec2 scale) {
    return std::hypot(size.x * scale.x, size.y * scale.y);
}

} // namespace

pxRect PaddedView(const Camera2D& camera, double pixels) {
    const pxRect view = camera.VisibleWorld();
    const double pad = pixels / std::max(camera.Zoom(), 1e-12);
    return pxRect(view.x - pad, view.y - pad, view.width + 2.0 * pad, view.height + 2.0 * pad);
}

bool SpriteInView(const Sprite&, const Transform2D& transform, pxVec2 pixelSize, const pxRect& view) {
    return CircleMeets(transform.position, Reach(pixelSize, transform.scale), view);
}

bool TextInView(const Text&, const Transform2D& transform, pxVec2 block, const pxRect& view) {
    return CircleMeets(transform.position, Reach(block, transform.scale), view);
}

bool ShapeInView(const Shape& shape, const Transform2D& transform, const pxRect& view) {
    // A shape is centered on its position, `size` being half its extent.
    return CircleMeets(transform.position, Reach(shape.size, transform.scale), view);
}

bool PathInView(const Path& path, const Transform2D& transform, const pxRect& view) {
    if (path.points.empty()) {
        return false;
    }
    const double c = std::cos(transform.rotation);
    const double s = std::sin(transform.rotation);
    double left = 0.0;
    double right = 0.0;
    double top = 0.0;
    double bottom = 0.0;
    bool first = true;
    for (const pxPoint& p : path.points) {
        const double x = p.x * transform.scale.x;
        const double y = p.y * transform.scale.y;
        const double wx = transform.position.x + c * x - s * y;
        const double wy = transform.position.y + s * x + c * y;
        left = first ? wx : std::min(left, wx);
        right = first ? wx : std::max(right, wx);
        top = first ? wy : std::min(top, wy);
        bottom = first ? wy : std::max(bottom, wy);
        first = false;
    }
    // Rounded corners stay inside the points' box; the line reaches half its width beyond it.
    const double half = path.width * 0.5;
    return right + half >= view.x && left - half <= view.Right() && bottom + half >= view.y && top - half <= view.Bottom();
}

} // namespace pixie
