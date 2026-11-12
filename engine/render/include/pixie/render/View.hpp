#pragma once

#include "pixie/core/FramebufferSize.hpp"
#include <pixie/math/pxMat.hpp>

namespace pixie {

/// Value snapshot of view, projection, projection * view and viewport dimensions in pixels.
/// Matrix getters borrow from the snapshot. Camera conventions belong to the producer.
class View {
public:
    View(const pxMat4& view, const pxMat4& projection, FramebufferSize viewport)
        : view_(view), projection_(projection), viewProjection_(projection * view), viewport_(viewport) {}

    const pxMat4& ViewMatrix() const { return view_; }
    const pxMat4& ProjectionMatrix() const { return projection_; }
    const pxMat4& ViewProjection() const { return viewProjection_; }
    FramebufferSize Viewport() const { return viewport_; }

private:
    pxMat4 view_;
    pxMat4 projection_;
    pxMat4 viewProjection_;
    FramebufferSize viewport_;
};

} // namespace pixie
