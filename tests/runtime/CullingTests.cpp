#include "Check.hpp"

#include "pixie/Camera2D.hpp"
#include "pixie/PathRenderer.hpp"
#include "pixie/ShapeRenderer.hpp"
#include "pixie/runtime/Culling.hpp"
#include "pixie/runtime/Path.hpp"
#include "pixie/runtime/PathDrawing.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/ShapeDrawing.hpp"
#include "pixie/runtime/Sprite.hpp"
#include "pixie/runtime/Text.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

#include <cmath>
#include <numbers>

using namespace pixie;

namespace {

const pxRect kView(-100.0, -100.0, 200.0, 200.0);

Entity Disc(World& world, pxPoint at, double radius) {
    Entity e = world.Create();
    e.Add<Transform2D>(Transform2D{at});
    e.Add<Shape>(Shape{ShapeKind::Circle, pxVec2(radius, radius)});
    return e;
}

Entity Line(World& world, pxPoint from, pxPoint to) {
    Entity e = world.Create();
    e.Add<Transform2D>();
    Path& path = e.Add<Path>();
    path.points = {from, to};
    path.width = 4.0;
    return e;
}

} // namespace

TEST(PaddedViewWidensTheVisibleAreaByViewPixels) {
    Camera2D camera;
    camera.SetViewport({1280, 720});
    camera.SetZoom(2.0);
    const pxRect visible = camera.VisibleWorld();
    const pxRect padded = PaddedView(camera, 32.0);
    // 32 view pixels are 16 world units at a zoom of 2.
    CHECK(std::abs(padded.x - (visible.x - 16.0)) < 1e-9 && std::abs(padded.width - (visible.width + 32.0)) < 1e-9);
    CHECK(std::abs(padded.y - (visible.y - 16.0)) < 1e-9 && std::abs(padded.height - (visible.height + 32.0)) < 1e-9);
}

TEST(BoundsKeepWhatMayShowAndSkipWhatCannot) {
    const Shape disc{ShapeKind::Circle, pxVec2(10.0, 10.0)};
    CHECK(ShapeInView(disc, Transform2D{pxPoint(0.0, 0.0)}, kView));
    // Its center outside, its edge inside: kept.
    CHECK(ShapeInView(disc, Transform2D{pxPoint(108.0, 0.0)}, kView));
    CHECK(!ShapeInView(disc, Transform2D{pxPoint(150.0, 0.0)}, kView));
    // Scaled up, it reaches in again.
    CHECK(ShapeInView(disc, Transform2D{pxPoint(150.0, 0.0), 0.0, pxVec2(5.0, 5.0)}, kView));

    // A sprite of 64 pixels at a quarter, pivoted at a corner and turned: still held by its diagonal.
    Sprite sprite;
    sprite.pivot = pxVec2(0.0, 0.0);
    const Transform2D turned{pxPoint(112.0, 0.0), std::numbers::pi, pxVec2(0.25, 0.25)};
    CHECK(SpriteInView(sprite, turned, pxVec2(64.0, 64.0), kView));
    CHECK(!SpriteInView(sprite, Transform2D{pxPoint(140.0, 0.0), 0.0, pxVec2(0.25, 0.25)}, pxVec2(64.0, 64.0), kView));

    const Text text;
    CHECK(TextInView(text, Transform2D{pxPoint(-120.0, 0.0)}, pxVec2(48.0, 24.0), kView));
    CHECK(!TextInView(text, Transform2D{pxPoint(-200.0, 0.0)}, pxVec2(48.0, 24.0), kView));

    // A path whose points all lie outside, but whose line crosses the view: kept.
    Path across;
    across.points = {pxPoint(-500.0, 0.0), pxPoint(500.0, 0.0)};
    CHECK(PathInView(across, Transform2D{}, kView));
    Path away;
    away.points = {pxPoint(300.0, 300.0), pxPoint(400.0, 300.0)};
    CHECK(!PathInView(away, Transform2D{}, kView));
    // Moved into view by its transform.
    CHECK(PathInView(away, Transform2D{pxPoint(-300.0, -300.0)}, kView));
}

TEST(OffScreenShapesAndPathsAreNotSubmitted) {
    World world;
    Disc(world, pxPoint(0.0, 0.0), 10.0);
    Disc(world, pxPoint(500.0, 0.0), 10.0);
    Line(world, pxPoint(-50.0, 0.0), pxPoint(50.0, 0.0));
    Line(world, pxPoint(300.0, 300.0), pxPoint(400.0, 300.0));

    ShapeRenderer shapes;
    SubmitShapes(world, shapes, 1.0);
    CHECK(shapes.PendingCount() == 2);
    ShapeRenderer culled;
    SubmitShapes(world, culled, 1.0, &kView);
    CHECK(culled.PendingCount() == 1);

    PathRenderer paths;
    SubmitPaths(world, paths, 1.0);
    PathRenderer culledPaths;
    SubmitPaths(world, culledPaths, 1.0, &kView);
    // What is on screen is submitted as without culling: the same segments as the one line alone.
    World alone;
    Line(alone, pxPoint(-50.0, 0.0), pxPoint(50.0, 0.0));
    PathRenderer onlyThat;
    SubmitPaths(alone, onlyThat, 1.0);
    CHECK(culledPaths.PendingCount() == onlyThat.PendingCount() && paths.PendingCount() > culledPaths.PendingCount());
}

TEST(HiddenAndCulledAreBothSkipped) {
    World world;
    Entity disc = Disc(world, pxPoint(0.0, 0.0), 10.0);
    disc.Get<Shape>().visible = false;
    ShapeRenderer shapes;
    SubmitShapes(world, shapes, 1.0, &kView);
    CHECK(shapes.PendingCount() == 0);
}
