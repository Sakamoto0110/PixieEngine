#pragma once

#include "pixie/render/Export.hpp"

#include "pixie/render/View.hpp"

#include <pixie/math/pxMat.hpp>
#include <pixie/math/pxVec.hpp>
#include <pixie/pxPoint.hpp>
#include <pixie/pxRect.hpp>

namespace pixie {

// A 2D view of the world: which world point sits at the middle of the viewport, and how many
// framebuffer pixels one world unit covers. World and screen both have Y pointing down, as
// PixieLib's pxOrtho2D does, so a positive rotation turns from +X toward +Y, clockwise on screen.
class Camera2D {
public:
    // The framebuffer the camera draws into, in pixels.
    void SetViewport(FramebufferSize size) { viewport_ = size; }
    FramebufferSize Viewport() const { return viewport_; }

    void SetCenter(pxPoint center) { center_ = center; }
    pxPoint Center() const { return center_; }

    // Pixels per world unit; clamped to [kMinZoom, kMaxZoom].
    PIXIE_RENDER_API void SetZoom(double zoom);
    double Zoom() const { return zoom_; }

    PIXIE_RENDER_API pxPoint ScreenToWorld(pxPoint screen) const;
    PIXIE_RENDER_API pxPoint WorldToScreen(pxPoint world) const;

    // Moves the view by a cursor movement in pixels, so the world follows the cursor.
    PIXIE_RENDER_API void Pan(pxVec2 screenDelta);
    // Multiplies the zoom by factor, keeping the world point under the screen point where it is.
    PIXIE_RENDER_API void ZoomAt(pxPoint screen, double factor);

    // The part of the world inside the viewport.
    PIXIE_RENDER_API pxRect VisibleWorld() const;
    // World to clip space, for the GPU.
    PIXIE_RENDER_API pxMat4f ViewProjection() const;
    PIXIE_RENDER_API View MakeView() const;

    static constexpr double kMinZoom = 1e-4;
    static constexpr double kMaxZoom = 1e4;

private:
    PIXIE_RENDER_API pxVec2 HalfViewport() const;

    FramebufferSize viewport_;
    pxPoint center_;
    double zoom_ = 1.0;
};

} // namespace pixie
