#include "Check.hpp"
#include "pixie/runtime/ParticlePhysics2D.hpp"
#include "pixie/runtime/ParticleSystem.hpp"
#include "pixie/runtime/ParticleSpawner.hpp"
#include "pixie/runtime/ParticleStep.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/World.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>

using namespace pixie;
namespace {
void Advance(ParticleSystem& pool, const ParticlePhysics2D& settings, const CollisionWorld2D* world,
             pxVec2 gravity, double dt, ParticlePhysicsStats2D& stats) {
    for (auto& p : pool.particles) { p.previous = p.position; p.age += static_cast<float>(dt); }
    MoveDynamicParticles2D(pool, settings, world, gravity, dt, stats);
}
}
TEST(DynamicParticlesPreserveLayoutPreviousAndStaticBallisticMotion) {
    CHECK(sizeof(Particle) == 40);
    ParticleSystem pool;
    pool.Emit({0, 0}, {10, 0}, 2);
    pool.EmitDynamic({0, 0}, {10, 0}, 2);
    ParticlePhysicsStats2D stats;
    Advance(pool, {}, nullptr, {0, 600}, 0.05, stats);
    CHECK(pool.particles[0].position.y == 0 && pool.particles[0].position.x == 0.5f);
    CHECK(std::abs(pool.particles[1].velocity.y-30) < 0.001);
    CHECK(std::abs(pool.particles[1].position.y-1.5f) < 0.001);
    CHECK(pool.particles[1].previous == pxPointf(0, 0));
    CHECK(stats.dynamic == 1 && stats.queries == 0);
}
TEST(DynamicParticleCircleSweepBouncesAndUsesSurfaceFriction) {
    for (double friction : {0.0, 1.0}) {
        CollisionWorld2D world;
        BodyShape2D floor; floor.halfSize = {200, 10}; floor.friction = friction;
        RigidBody2D body; body.type = BodyType2D::Static;
        world.AddBody({0, 110}, 0, body, {&floor, 1});
        ParticlePhysics2D settings; settings.restitution = 0.5; settings.friction = 1; settings.bounceThreshold = 0;
        ParticleSystem pool; pool.EmitDynamic({0, 90}, {40, 300}, 2);
        ParticlePhysicsStats2D stats;
        Advance(pool, settings, &world, {0, 0}, 0.05, stats);
        CHECK(stats.contacts > 0 && pool.particles.size() == 1);
        CHECK(std::abs(pool.particles[0].velocity.y+150) < 0.01);
        CHECK(pool.particles[0].position.y < 99);
        CHECK(friction == 0 ? std::abs(pool.particles[0].velocity.x-40) < 0.01 : std::abs(pool.particles[0].velocity.x) < 0.01);
        CHECK(pool.particles[0].previous == pxPointf(0, 90));
        CHECK(world.Stats().bodies == 1);
    }
}
TEST(DynamicParticleIgnoresSensorsAndHonorsMaskAndFastWalls) {
    for (int mode=0; mode<3; ++mode) {
        CollisionWorld2D world;
        BodyShape2D wall; wall.halfSize = {0.5, 200}; wall.sensor = mode == 0;
        RigidBody2D body; body.type = BodyType2D::Static;
        world.AddBody({50, 0}, 0, body, {&wall, 1});
        ParticlePhysics2D settings; settings.mask = mode == 1 ? 0 : 0x7fffffff;
        ParticleSystem pool; pool.EmitDynamic({0, 0}, {10000, 0}, 2);
        ParticlePhysicsStats2D stats;
        Advance(pool, settings, &world, {0, 0}, 1.0/60, stats);
        CHECK(mode == 2 ? pool.particles[0].position.x < 49 : pool.particles[0].position.x > 100);
        CHECK(mode == 2 ? stats.contacts > 0 : stats.contacts == 0);
    }
}
TEST(DynamicParticlesUseMovingSurfaceVelocityAndSurfaceRestitution) {
    CollisionWorld2D world;
    BodyShape2D floor; floor.halfSize = {200, 10}; floor.friction = 1; floor.restitution = 0.8;
    RigidBody2D body; body.type = BodyType2D::Kinematic; body.initialVelocity = {60, 0};
    world.AddBody({0, 110}, 0, body, {&floor, 1});
    ParticlePhysics2D settings; settings.restitution = 0; settings.friction = 1; settings.bounceThreshold = 0;
    ParticleSystem pool; pool.EmitDynamic({0, 90}, {0, 300}, 2);
    ParticlePhysicsStats2D stats;
    Advance(pool, settings, &world, {0, 0}, 0.05, stats);
    CHECK(std::abs(pool.particles[0].velocity.x-60) < 0.01);
    CHECK(std::abs(pool.particles[0].velocity.y+240) < 0.01);
    CHECK(world.Stats().bodies == 1);
}
TEST(OrdinaryParticlesMakeNoPhysicsQueriesInAMixedCapablePool) {
    CollisionWorld2D world; world.AddBox({0, 110}, {200, 10});
    ParticleSystem pool;
    for (int i=0; i<10000; ++i) pool.Emit({0, 0}, {10, 0}, 2);
    ParticlePhysicsStats2D stats;
    Advance(pool, {}, &world, {0, 1100}, 1.0/60, stats);
    CHECK(stats.dynamic == 0 && stats.queries == 0 && stats.contacts == 0);
    CHECK(pool.particles.back().position.y == 0);
}
TEST(DynamicParticleSettlesAndCornerWorkIsBounded) {
    CollisionWorld2D floor;
    floor.AddBox({0, 110}, {200, 10});
    ParticleSystem pool; pool.EmitDynamic({0, 80}, {0, 0}, 0);
    ParticlePhysicsStats2D stats;
    for (int i=0; i<300; ++i) Advance(pool, {}, &floor, {0, 1100}, 1.0/60, stats);
    CHECK(pool.particles.size() == 1);
    if (pool.particles.empty()) return;
    CHECK(pool.particles[0].position.y > 98 && pool.particles[0].position.y <= 99.1);
    CHECK(std::abs(pool.particles[0].velocity.y) < 1);
    CollisionWorld2D corridor;
    corridor.AddBox({-4, 0}, {1, 100}); corridor.AddBox({4, 0}, {1, 100});
    ParticleSystem fast; fast.EmitDynamic({0, 0}, {10000, 0}, 1);
    ParticlePhysics2D settings; settings.restitution = 1; settings.friction = 0;
    ParticlePhysicsStats2D bounded;
    Advance(fast, settings, &corridor, {0, 0}, 0.05, bounded);
    CHECK(bounded.contacts <= 4 && bounded.exhausted == 1);
    CHECK(std::abs(fast.particles[0].position.x) < 3);
}
TEST(DynamicParticleUnrecoverableAndNonfiniteBirthsExpireWithoutLeakingBodies) {
    CollisionWorld2D world;
    world.AddBox({0, 0}, {100, 100});
    ParticleSystem pool; pool.EmitDynamic({0, 0}, {0, 0}, 0);
    pool.EmitDynamic({std::numeric_limits<float>::quiet_NaN(), 0}, {0, 0}, 0);
    ParticlePhysicsStats2D stats;
    Advance(pool, {}, &world, {0, 0}, 1.0/60, stats);
    CHECK(pool.particles.empty() && stats.expired == 2 && world.Stats().bodies == 1);
}
TEST(DynamicSpawnerBirthsIntegrateOnlyTheirOwnFractionOfTheTick) {
    World world;
    world.Create().Add<PhysicsSettings2D>().gravity = {0, 600};
    auto pool = world.Create(); pool.Add<ParticleSystem>(); pool.Add<ParticlePhysics2D>();
    auto emitter = world.Create(); emitter.Add<Transform2D>();
    auto& spawner = emitter.Add<ParticleSpawner>(); spawner.system = pool.Id(); spawner.rate = 120; spawner.dynamic = true;
    AddParticleStep(world); AddParticleStep(world);
    world.Step(1.0/60, {});
    const auto& particles = pool.Get<ParticleSystem>().particles;
    CHECK(particles.size() == 2);
    CHECK(std::abs(particles[0].position.y-600.0/120/120) < 0.0001);
    CHECK(std::abs(particles[1].position.y) < 0.0001);
    CHECK(particles[0].previous == pxPointf(0, 0));
}
TEST(DynamicParticleSettingsAndSpawnerKindRoundTripThroughNativeDescriptors) {
    World world;
    auto pool = world.Create(); pool.Add<ParticleSystem>(); pool.Add<ParticlePhysics2D>().restitution = 0.75; AddToScene(pool);
    auto emitter = world.Create(); emitter.Add<Transform2D>();
    auto& s = emitter.Add<ParticleSpawner>(); s.system = pool.Id(); s.dynamic = true; AddToScene(emitter);
    ComponentTypes types; DescribeRuntimeComponents(types);
    const auto text = WriteScene(world, types);
    World loaded; CHECK(ReadScene(loaded, types, text).ok);
    CHECK(WriteScene(loaded, types) == text);
    for (auto [id, settings] : loaded.Registry().view<ParticlePhysics2D>().each()) {
        (void)id; CHECK(settings.restitution == 0.75);
    }
    for (auto [id, spawner] : loaded.Registry().view<ParticleSpawner>().each()) { (void)id; CHECK(spawner.dynamic); }
}

TEST(OrdinaryParticlesDoNotInstallScenePhysicsAndLateInstallationIsOrdered) {
    World world; auto pool = world.Create(); pool.Add<ParticleSystem>().Emit({0,0}, {60,0}, 2);
    AddParticleStep(world); world.Step(1.0/60, {});
    CHECK(ScenePhysicsWorld2D(world) == nullptr && pool.Get<ParticleSystem>().particles[0].position.x == 1);
    for (const auto& metric : world.GetTelemetry().Snapshot())
        CHECK(metric.name != "system.pre.scene.collision.sync" && metric.name != "system.physics.scene.character.move");
    auto floor = world.Create(); floor.Add<Transform2D>().position = {0,100}; floor.Add<Collider2D>().halfSize = {100,5};
    auto body = world.Create(); body.Add<Transform2D>(); body.Add<Collider2D>(); body.Add<RigidBody2D>();
    bool observedSolvedBody = false;
    world.AddSystem(SystemPhase::PostUpdate, "observe", [&](World&, const FrameTime&, const Input&) {
        observedSolvedBody = body.Get<Transform2D>().position.y > 0;
    });
    InstallSceneMovement2D(world); world.Step(1.0/60, {}); CHECK(observedSolvedBody);
}
TEST(InvalidParticlePoolReportsErrorFallsBackAndRecoversWithoutBlockingOtherPools) {
    World world; auto floor = world.Create(); floor.Add<Transform2D>().position = {0,100}; floor.Add<Collider2D>().halfSize = {300,5};
    auto bad = world.Create(); bad.Add<ParticleSystem>().EmitDynamic({0,0}, {60,0}, 2); bad.Add<ParticlePhysics2D>().collisionRadius = 0.2;
    auto good = world.Create(); good.Add<ParticleSystem>().EmitDynamic({100,0}, {0,0}, 2); good.Add<ParticlePhysics2D>().collisionRadius = 1.5;
    AddParticleStep(world); InstallSceneMovement2D(world); world.Step(1.0/60, {});
    CHECK(!world.IsStepping() && ParticlePhysicsErrors2D(world).size() == 1);
    CHECK(bad.Get<ParticleSystem>().particles[0].position.x == 1 && bad.Get<ParticleSystem>().particles[0].position.y == 0);
    CHECK(good.Get<ParticleSystem>().particles[0].position.y > 0);
    bad.Get<ParticlePhysics2D>().collisionRadius = 1.5; world.Step(1.0/60, {});
    CHECK(ParticlePhysicsErrors2D(world).empty() && bad.Get<ParticleSystem>().particles[0].position.y > 0);
    bool threw = false;
    auto invalid = bad.Get<ParticlePhysics2D>(); invalid.collisionRadius = 0.2; ParticlePhysicsStats2D stats;
    try { MoveDynamicParticles2D(bad.Get<ParticleSystem>(), invalid, SceneCollisionWorld2D(world), {0,1100}, 1.0/60, stats); }
    catch (const std::invalid_argument&) { threw = true; }
    CHECK(threw);
}
TEST(NativeManifoldsExposeFloorAndBoxImpulsesWithoutAdvancingOrWakingBodies) {
    CollisionWorld2D world; world.SetGravity({0,1000}); world.AddBox({0,120},{200,5});
    BodyShape2D shape; shape.halfSize = {20,20};
    RigidBody2D body; body.massOverride = 2;
    const auto lower = world.AddBody({0,94}, 0, body, {&shape,1});
    shape.halfSize = {12,12}; const auto upper = world.AddBody({0,0}, 0.3, body, {&shape,1});
    bool floorContact = false, boxContact = false; double peak = 0;
    for (int i=0; i<240; ++i) {
        world.StepBodies(1.0/60);
        for (const auto& c : world.BodyContacts(lower)) {
            CHECK(c.a.collider.value < c.b.collider.value);
            CHECK(std::abs(std::hypot(c.normal.x,c.normal.y)-1) < 0.001 && std::isfinite(c.point.y));
            if (c.normalImpulse <= 0) continue;
            peak = std::max(peak,c.normalImpulse);
            if (c.a.body == upper || c.b.body == upper) boxContact = true;
            if (world.BodyType(c.a.body) == BodyType2D::Static || world.BodyType(c.b.body) == BodyType2D::Static) floorContact = true;
        }
    }
    const auto before = world.BodyMotion(lower); const auto generation = world.Generation(); const auto events = world.Events().size();
    (void)world.BodyContacts(lower);
    CHECK(world.BodyMotion(lower).awake == before.awake && world.Generation() == generation && world.Events().size() == events);
    CHECK(floorContact && boxContact && peak > 100);
}
