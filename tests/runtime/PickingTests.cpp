#include "Check.hpp"

#include "pixie/runtime/ParticleSystem.hpp"
#include "pixie/runtime/Picking.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

using namespace pixie;

namespace {

// A circle of radius `radius` at `at`, scaled by `scale`.
Entity Disc(World& world, pxPoint at, double radius, double scale = 1.0) {
    Entity e = world.Create();
    Transform2D transform{at};
    transform.scale = pxVec2(scale, scale);
    e.Add<Transform2D>(transform);
    e.Add<Shape>(Shape{ShapeKind::Circle, pxVec2(radius, radius)});
    return e;
}

} // namespace

TEST(PickAtFindsTheShapeUnderThePointNearestFirst) {
    World world;
    const Entity small = Disc(world, pxPoint(0.0, 0.0), 5.0);
    const Entity large = Disc(world, pxPoint(8.0, 0.0), 10.0);
    // Inside both: the nearer center wins.
    CHECK(PickAt(world, pxPoint(1.0, 0.0), 0.0) == small.Id());
    CHECK(PickAt(world, pxPoint(6.0, 0.0), 0.0) == large.Id());
    // Outside every shape, then within the tolerance.
    CHECK(PickAt(world, pxPoint(0.0, 7.0), 0.0) == entt::null);
    CHECK(PickAt(world, pxPoint(0.0, 7.0), 2.5) == small.Id());
    // The transform's scale widens the shape.
    const Entity scaled = Disc(world, pxPoint(100.0, 0.0), 5.0, 3.0);
    CHECK(PickAt(world, pxPoint(114.0, 0.0), 0.0) == scaled.Id());
}

TEST(OnlyEntitiesWithATransformAndAShapeArePicked) {
    World world;
    Entity bare = world.Create();
    bare.Add<Transform2D>();
    Entity shapeOnly = world.Create();
    shapeOnly.Add<Shape>();
    CHECK(PickAt(world, pxPoint(0.0, 0.0), 10.0) == entt::null);
    CHECK(PickIn(world, pxRect(-10.0, -10.0, 20.0, 20.0)).empty());
}

TEST(PickInListsTheCentersInsideTheBoxOldestFirst) {
    World world;
    const Entity a = Disc(world, pxPoint(10.0, 10.0), 50.0);
    Disc(world, pxPoint(200.0, 10.0), 1.0);
    const Entity c = Disc(world, pxPoint(30.0, 40.0), 1.0);
    // The first disc's shape reaches past the box; only centers count.
    const auto picked = PickIn(world, pxRect(0.0, 0.0, 50.0, 50.0));
    CHECK(picked.size() == 2);
    CHECK(picked.size() == 2 && picked[0] == a.Id() && picked[1] == c.Id());
}

TEST(RemoveParticleAtTakesTheNearestWithinTheRadius) {
    World world;
    Entity e = world.Create();
    ParticleSystem& system = e.Add<ParticleSystem>();
    system.Emit(pxPointf(0.0f, 0.0f), pxVec2f(0.0f), 0.0f);
    system.Emit(pxPointf(3.0f, 0.0f), pxVec2f(0.0f), 0.0f);
    system.Emit(pxPointf(50.0f, 0.0f), pxVec2f(0.0f), 0.0f);
    CHECK(!RemoveParticleAt(world, pxPoint(20.0, 0.0), 5.0));
    CHECK(RemoveParticleAt(world, pxPoint(2.0, 0.0), 5.0));
    const auto& particles = e.Get<ParticleSystem>().particles;
    CHECK(particles.size() == 2);
    // The one at 3 was nearer to 2 than the one at 0.
    for (const Particle& p : particles) {
        CHECK(p.position.x != 3.0f);
    }
}
