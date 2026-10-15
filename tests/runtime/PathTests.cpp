#include "Check.hpp"

#include "pixie/PathRenderer.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/Path.hpp"
#include "pixie/runtime/PathDrawing.hpp"
#include "pixie/runtime/Picking.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Sprite.hpp"
#include "pixie/runtime/SpriteDrawing.hpp"
#include "pixie/runtime/SpriteImages.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

#include <cmath>
#include <numbers>
#include <string>
#include <vector>

using namespace pixie;

namespace {

bool Near(double a, double b, double tolerance = 1e-9) {
    return std::abs(a - b) <= tolerance;
}

bool Near(pxPoint a, pxPoint b, double tolerance = 1e-9) {
    return Near(a.x, b.x, tolerance) && Near(a.y, b.y, tolerance);
}

ComponentTypes Types() {
    ComponentTypes types;
    DescribeRuntimeComponents(types);
    return types;
}

Entity PathThrough(World& world, std::vector<pxPoint> points, double width = 4.0) {
    Entity e = world.Create();
    e.Add<Transform2D>();
    Path path;
    path.points = std::move(points);
    path.width = width;
    e.Add<Path>(path);
    return e;
}

Entity SpriteAt(World& world, pxPoint at, double size, int layer) {
    Entity e = world.Create();
    e.Add<Transform2D>(Transform2D{at});
    Sprite sprite;
    sprite.region = pxRect(0.0, 0.0, size, size);
    sprite.layer = layer;
    e.Add<Sprite>(sprite);
    return e;
}

} // namespace

TEST(RoundCornersReplacesACornerWithAnArcOfTheRadius) {
    const std::vector<pxPoint> corner{pxPoint(0.0, 0.0), pxPoint(64.0, 0.0), pxPoint(64.0, 48.0)};
    const std::vector<pxPoint> round = RoundCorners(corner, 8.0);
    CHECK(round.front() == corner.front() && round.back() == corner.back());
    // The arc starts 8 before the corner and ends 8 after it, every point 8 from its center.
    CHECK(Near(round[1], pxPoint(56.0, 0.0)) && Near(round[round.size() - 2], pxPoint(64.0, 8.0)));
    for (std::size_t i = 1; i + 1 < round.size(); ++i) {
        CHECK(Near(std::hypot(round[i].x - 56.0, round[i].y - 8.0), 8.0, 1e-9));
    }
    // Two straight runs shortened by 8 each, and a quarter circle of radius 8, nearly.
    CHECK(Near(PolylineLength(round), 112.0 - 16.0 + std::numbers::pi / 2.0 * 8.0, 0.05));
    CHECK(PolylineLength(corner) == 112.0);
}

TEST(RoundCornersKeepsStraightLinesAndShrinksArcsOnShortSegments) {
    const std::vector<pxPoint> straight{pxPoint(0.0, 0.0), pxPoint(10.0, 0.0), pxPoint(20.0, 0.0)};
    CHECK(RoundCorners(straight, 8.0) == straight);
    const std::vector<pxPoint> sharp{pxPoint(0.0, 0.0), pxPoint(10.0, 0.0), pxPoint(10.0, 10.0)};
    CHECK(RoundCorners(sharp, 0.0) == sharp);
    // A radius larger than the segments: each lends only half of itself.
    const std::vector<pxPoint> round = RoundCorners(sharp, 100.0);
    CHECK(Near(round[1], pxPoint(5.0, 0.0)) && Near(round[round.size() - 2], pxPoint(10.0, 5.0)));
}

TEST(APathsPointsAreAPointListGoingThroughAScene) {
    const ComponentTypes types = Types();
    const auto& path = types.All()[static_cast<std::size_t>(types.IndexOf("Path"))];
    CHECK(path.fields[0].name == "points" && path.fields[0].kind == FieldKind::PointList);
    CHECK(path.fields[0].read == nullptr && path.fields[0].readPoints != nullptr);

    World world;
    Entity e = SceneEntityWith<Transform2D>(world, "wire");
    Path wire;
    wire.points = {pxPoint(-10.5, 2.0), pxPoint(30.0, 2.0), pxPoint(30.0, 40.25)};
    wire.fill = 0.5;
    e.Add<Path>(wire);
    const std::string text = WriteScene(world, types);
    World read;
    const SceneReport report = ReadScene(read, types, text);
    CHECK(report.ok && report.warnings.empty());
    bool found = false;
    read.Each<const Path>([&](Entity, const Path& p) { found = p.points == wire.points && p.fill == 0.5; });
    CHECK(found);
    CHECK(WriteScene(read, types) == text);

    const std::vector<Entity> chosen{e};
    const EntitiesReport copy = ReadEntities(world, types, WriteEntities(world, types, chosen), SceneIds::New);
    CHECK(copy.ok && copy.entities.size() == 1 && copy.entities[0].Get<Path>().points == wire.points);

    // Not a list of pairs: a warning, and the default.
    std::string wrong = text;
    const auto at = wrong.find("\"points\": [");
    CHECK(at != std::string::npos);
    wrong.replace(at, std::string("\"points\": [").size(), "\"points\": [7, ");
    World bad;
    const SceneReport warned = ReadScene(bad, types, wrong);
    CHECK(warned.ok && warned.warnings.size() == 1 && warned.warnings[0].find("expected [[x, y]") != std::string::npos);
}

TEST(APathIsDrawnAtItsTransformAndPickedAlongItsLine) {
    Path path;
    path.points = {pxPoint(0.0, 0.0), pxPoint(100.0, 0.0)};
    path.width = 10.0;
    Transform2D transform{pxPoint(50.0, 20.0)};
    transform.rotation = std::numbers::pi / 2.0;
    transform.scale = pxVec2(2.0, 2.0);
    const std::vector<pxPoint> line = PathLine(path, transform);
    // Turned a quarter and scaled 2: from (50, 20) down to (50, 220); the width is not scaled.
    CHECK(Near(line.front(), pxPoint(50.0, 20.0)) && Near(line.back(), pxPoint(50.0, 220.0), 1e-9));
    CHECK(PathContains(path, transform, pxPoint(54.0, 120.0), 0.0));
    CHECK(!PathContains(path, transform, pxPoint(56.0, 120.0), 0.0));
    CHECK(PathContains(path, transform, pxPoint(56.0, 120.0), 2.0));
    CHECK(!PathContains(path, transform, pxPoint(50.0, 230.0), 0.0));
}

TEST(PickingFollowsTheDrawingOrderOfShapesSpritesAndPaths) {
    World world;
    SpriteImages images;
    const Entity back = SpriteAt(world, pxPoint(0.0, 0.0), 40.0, -1);
    const Entity wire = PathThrough(world, {pxPoint(-100.0, 0.0), pxPoint(100.0, 0.0)}, 6.0);
    // The path is drawn over a background sprite.
    CHECK(PickAt(world, pxPoint(0.0, 0.0), 0.0, &images) == wire.Id());
    CHECK(PickAt(world, pxPoint(0.0, 10.0), 0.0, &images) == back.Id());
    CHECK(PickAt(world, pxPoint(80.0, 0.0), 0.0, &images) == wire.Id());
    // A sprite of layer 0 is drawn over the path.
    const Entity front = SpriteAt(world, pxPoint(80.0, 0.0), 10.0, 0);
    CHECK(PickAt(world, pxPoint(80.0, 0.0), 0.0, &images) == front.Id());
    // A newer path is drawn over an older one.
    const Entity cross = PathThrough(world, {pxPoint(-60.0, -50.0), pxPoint(-60.0, 50.0)});
    CHECK(PickAt(world, pxPoint(-60.0, 0.0), 0.0, &images) == cross.Id());
    // Shapes are drawn over everything.
    Entity shape = world.Create();
    shape.Add<Transform2D>(Transform2D{pxPoint(-60.0, 0.0)});
    shape.Add<Shape>(Shape{ShapeKind::Circle, pxVec2(3.0, 3.0)});
    CHECK(PickAt(world, pxPoint(-60.0, 0.0), 0.0, &images) == shape.Id());

    // A box selects a path only when its whole line is inside.
    CHECK(PickIn(world, pxRect(-110.0, -5.0, 220.0, 10.0)) ==
          (std::vector<entt::entity>{back.Id(), wire.Id(), front.Id(), shape.Id()}));
    CHECK(PickIn(world, pxRect(-50.0, -5.0, 100.0, 10.0)) == (std::vector<entt::entity>{back.Id()}));
}

TEST(SpritePointAndSpriteLocalFollowTheTransformAndUndoEachOther) {
    Sprite sprite;
    const pxVec2 size(100.0, 50.0);
    Transform2D transform{pxPoint(10.0, 20.0)};
    // The output anchor of a gate, at the middle of its right side.
    const pxPoint output(0.88, 0.5);
    CHECK(Near(SpritePoint(sprite, transform, size, output), pxPoint(10.0 + 38.0, 20.0)));
    // Mirrored horizontally: the output faces left.
    transform.scale = pxVec2(-1.0, 1.0);
    CHECK(Near(SpritePoint(sprite, transform, size, output), pxPoint(10.0 - 38.0, 20.0)));
    // Turned a quarter and scaled: the corners follow, and SpriteLocal undoes SpritePoint.
    transform.rotation = std::numbers::pi / 2.0;
    transform.scale = pxVec2(2.0, -0.5);
    sprite.pivot = pxVec2(0.25, 1.0);
    for (const pxPoint uv : {pxPoint(0.0, 0.0), pxPoint(1.0, 1.0), output, pxPoint(0.3, 0.9)}) {
        const auto back = SpriteLocal(sprite, transform, size, SpritePoint(sprite, transform, size, uv));
        CHECK(back && Near(*back, uv, 1e-9));
    }
    const auto corners = SpriteCorners(sprite, transform, size);
    CHECK(Near(corners[2], SpritePoint(sprite, transform, size, pxPoint(1.0, 1.0))));
    transform.scale = pxVec2(0.0, 1.0);
    CHECK(!SpriteLocal(sprite, transform, size, pxPoint(0.0, 0.0)));
}
