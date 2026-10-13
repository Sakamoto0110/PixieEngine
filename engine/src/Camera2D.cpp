#include "pixie/Camera2D.hpp"

#include <algorithm>

namespace pixie {

void Camera2D::SetZoom(double zoom) {
    zoom_ = std::clamp(zoom, kMinZoom, kMaxZoom);
}

pxVec2 Camera2D::HalfViewport() const {
    return pxVec2(viewport_.width, viewport_.height) * 0.5;
}

pxPoint Camera2D::ScreenToWorld(pxPoint screen) const {
    const pxVec2 fromMiddle = pxToVec2(screen) - HalfViewport();
    return center_ + fromMiddle / zoom_;
}

pxPoint Camera2D::WorldToScreen(pxPoint world) const {
    const pxVec2 fromCenter = world - center_;
    return pxToPoint(fromCenter * zoom_ + HalfViewport());
}

void Camera2D::Pan(pxVec2 screenDelta) {
    center_ = center_ - screenDelta / zoom_;
}

void Camera2D::ZoomAt(pxPoint screen, double factor) {
    const pxPoint anchor = ScreenToWorld(screen);
    SetZoom(zoom_ * factor);
    // Move the center so that the anchor maps back to the same screen point.
    center_ = center_ + (anchor - ScreenToWorld(screen));
}

pxRect Camera2D::VisibleWorld() const {
    const pxPoint topLeft = ScreenToWorld(pxPoint(0.0, 0.0));
    return pxRect(topLeft.x, topLeft.y, viewport_.width / zoom_, viewport_.height / zoom_);
}

pxMat4f Camera2D::ViewProjection() const {
    // clip.x = (world.x - center.x) * zoom / (width / 2); clip.y the same with Y flipped, since
    // clip space has Y up and the world has Y down.
    const pxVec2 half = HalfViewport();
    const double sx = half.x > 0.0 ? zoom_ / half.x : 0.0;
    const double sy = half.y > 0.0 ? -zoom_ / half.y : 0.0;
    pxMat4 m(1.0);
    m[0][0] = sx;
    m[1][1] = sy;
    m[3][0] = -center_.x * sx;
    m[3][1] = -center_.y * sy;
    return pxMat4f(m);
}

} // namespace pixie
