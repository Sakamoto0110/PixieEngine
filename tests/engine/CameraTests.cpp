#include "Check.hpp"

#include "pixie/render/2d/Camera2D.hpp"

#include <cmath>

using pixie::Camera2D;

namespace {

bool Near(double a, double b, double tolerance) {
    return std::abs(a - b) <= tolerance;
}

bool Near(pxPoint a, pxPoint b, double tolerance) {
    return Near(a.x, b.x, tolerance) && Near(a.y, b.y, tolerance);
}

Camera2D MakeCamera() {
    Camera2D camera;
    camera.SetViewport({1280, 720});
    camera.SetCenter(pxPoint(150.0, -40.0));
    camera.SetZoom(2.5);
    return camera;
}

// Where the GPU puts a world point, in framebuffer pixels (Y down), through ViewProjection.
pxPoint ThroughGpu(const Camera2D& camera, pxPoint world) {
    const pxVec4f clip = camera.ViewProjection() *
                         pxVec4f(static_cast<float>(world.x), static_cast<float>(world.y), 0.0f, 1.0f);
    const pixie::FramebufferSize viewport = camera.Viewport();
    return pxPoint((clip.x / clip.w + 1.0) * 0.5 * viewport.width,
                   (1.0 - clip.y / clip.w) * 0.5 * viewport.height);
}

} // namespace

TEST(CenterIsAtTheMiddleOfTheViewport) {
    const Camera2D camera = MakeCamera();
    CHECK(Near(camera.WorldToScreen(camera.Center()), pxPoint(640.0, 360.0), 1e-9));
}

TEST(ScreenAndWorldConversionsAreInverse) {
    const Camera2D camera = MakeCamera();
    for (const pxPoint screen : {pxPoint(0, 0), pxPoint(1280, 720), pxPoint(333.5, 97.25)}) {
        CHECK(Near(camera.WorldToScreen(camera.ScreenToWorld(screen)), screen, 1e-9));
    }
}

TEST(GpuMatrixAgreesWithScreenToWorld) {
    // The cursor marker is drawn at ScreenToWorld(cursor); the GPU must put it back on the cursor.
    Camera2D camera = MakeCamera();
    for (const double zoom : {0.05, 0.3, 1.0, 2.5, 40.0}) {
        camera.SetZoom(zoom);
        for (const pxPoint cursor : {pxPoint(0, 0), pxPoint(1279, 719), pxPoint(400, 610)}) {
            CHECK(Near(ThroughGpu(camera, camera.ScreenToWorld(cursor)), cursor, 0.01));
        }
    }
}

TEST(ZoomAtKeepsThePointUnderTheCursor) {
    Camera2D camera = MakeCamera();
    const pxPoint cursor(900.0, 150.0);
    const pxPoint before = camera.ScreenToWorld(cursor);
    camera.ZoomAt(cursor, 3.0);
    CHECK(Near(camera.Zoom(), 7.5, 1e-12));
    CHECK(Near(camera.ScreenToWorld(cursor), before, 1e-9));
    camera.ZoomAt(cursor, 1.0 / 30.0);
    CHECK(Near(camera.ScreenToWorld(cursor), before, 1e-9));
}

TEST(PanMovesTheWorldWithTheCursor) {
    Camera2D camera = MakeCamera();
    const pxPoint grabbed = camera.ScreenToWorld(pxPoint(100.0, 100.0));
    camera.Pan(pxVec2(30.0, -12.0));
    CHECK(Near(camera.WorldToScreen(grabbed), pxPoint(130.0, 88.0), 1e-9));
}

TEST(VisibleWorldSpansTheViewport) {
    const Camera2D camera = MakeCamera();
    const pxRect visible = camera.VisibleWorld();
    CHECK(Near(visible.width, 1280.0 / 2.5, 1e-9));
    CHECK(Near(visible.height, 720.0 / 2.5, 1e-9));
    CHECK(Near(pxPoint(visible.x, visible.y), camera.ScreenToWorld(pxPoint(0, 0)), 1e-9));
}
