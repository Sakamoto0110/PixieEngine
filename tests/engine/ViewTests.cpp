#include "Check.hpp"
#include "pixie/render/View.hpp"
#include "pixie/render/2d/Camera2D.hpp"

TEST(ViewRetainsThreeDimensionalTransformsInProjectionViewOrder) {
    pxMat4 view(1.0);
    view[3][2] = -7.0;
    pxMat4 projection(1.0);
    projection[2][2] = 0.5;
    projection[2][3] = -1.0;
    const pixie::View result(view, projection, {900, 600});
    const pxVec4 clip = result.ViewProjection() * pxVec4(1.0, 2.0, 11.0, 1.0);
    CHECK(clip.x == 1.0);
    CHECK(clip.y == 2.0);
    CHECK(clip.z == 2.0);
    CHECK(clip.w == -3.0);
    CHECK(result.ViewMatrix()[3][2] == -7.0);
    CHECK(result.ProjectionMatrix()[2][3] == -1.0);
    CHECK(result.Viewport().width == 900);
    CHECK(result.Viewport().height == 600);
}

TEST(CameraViewPreservesLegacyGpuMatrixAcrossZoomAndTranslation) {
    pixie::Camera2D camera;
    camera.SetViewport({1280, 720});
    for (const double zoom : {0.0001, 0.05, 0.3, 1.0, 2.5, 40.0, 10000.0}) {
        camera.SetZoom(zoom);
        for (const pxPoint center : {pxPoint(0, 0), pxPoint(150.5, -40.25), pxPoint(-10000, 27000)}) {
            camera.SetCenter(center);
            pxMat4 expected(1.0);
            expected[0][0] = zoom / 640.0;
            expected[1][1] = -zoom / 360.0;
            expected[3][0] = -center.x * expected[0][0];
            expected[3][1] = -center.y * expected[1][1];
            const pxMat4f before(expected);
            const pxMat4f after(camera.MakeView().ViewProjection());
            for (int column = 0; column < 4; ++column)
                for (int row = 0; row < 4; ++row)
                    CHECK(before[column][row] == after[column][row]);
        }
    }
}
