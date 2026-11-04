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
#include <array>
#include <limits>
#include <stdexcept>

using namespace pixie;
namespace {
void Advance(ParticleSystem& pool, const ParticlePhysics2D& settings, const CollisionWorld2D* world,
             pxVec2 gravity, double dt, ParticlePhysicsStats2D& stats) {
    for (auto& p : pool.particles) { p.previous = p.position; p.age += static_cast<float>(dt); }
    MoveDynamicParticles2D(pool, settings, world, gravity, dt, stats);
}
Entity ScopedGravityPool(World& world, pxVec2 gravity, bool backend) {
    world.Create().Add<PhysicsSettings2D>().gravity = gravity;
    auto pool = world.Create();
    pool.Add<ParticleSystem>().EmitDynamic({0, 0}, {0, 0}, 20);
    pool.Add<ParticlePhysics2D>().gravityScale = 0.5;
    if (backend) InstallSceneMovement2D(world);
    AddParticleStep(world);
    return pool;
}
}
TEST(SceneParticlePoolsMatchSeparateGlobalReferencesWithAndWithoutBackend) {
    for (const bool backend : {false, true}) {
        World lateral, zero, scoped;
        auto a = ScopedGravityPool(lateral, {90, 0}, backend);
        auto b = ScopedGravityPool(zero, {0, 0}, backend);
        auto first = ScopedGravityPool(scoped, {0, 600}, backend);
        auto second = scoped.Create();
        second.Add<ParticleSystem>().EmitDynamic({0, 0}, {0, 0}, 20);
        second.Add<ParticlePhysics2D>().gravityScale = 0.5;
        SceneGravity2D sideways; sideways.mode = SceneGravityMode2D::Override; sideways.value = {90, 0};
        SceneGravity2D stopped; stopped.mode = SceneGravityMode2D::Override; stopped.value = {0, 0};
        const ScopeId left{101}, right{202};
        CHECK(scoped.CreateScope(left, "Lateral pool", sideways) && scoped.CreateScope(right, "Zero pool", stopped));
        CHECK(scoped.SetScope(first, left) && scoped.SetScope(second, right));
        for (int tick = 1; tick <= 600; ++tick) {
            lateral.Step(1.0 / 60, {}); zero.Step(1.0 / 60, {}); scoped.Step(1.0 / 60, {});
            if (tick != 1 && tick != 60 && tick != 600) continue;
            CHECK(ParticlePhysicsErrors2D(scoped).empty());
            for (const auto pair : {std::pair{a, first}, std::pair{b, second}}) {
                const auto& native = pair.first.Get<ParticleSystem>().particles;
                const auto& local = pair.second.Get<ParticleSystem>().particles;
                CHECK(native.size() == 1 && local.size() == 1);
                if (native.size() == 1 && local.size() == 1) {
                    CHECK(native[0].position == local[0].position);
                    CHECK(native[0].velocity == local[0].velocity);
                }
            }
        }
        if (!backend) CHECK(SceneCollisionWorld2D(scoped) == nullptr);
    }
}

TEST(SceneParticleTransferUsesOldMembershipThroughParticlesThenNewGravityNextTick) {
    World world;
    auto pool = ScopedGravityPool(world, {0, 600}, false);
    pool.Get<ParticleSystem>().Emit({0, 0}, {0, 0}, 20); // Ordinary particle remains ballistic.
    SceneGravity2D lateral; lateral.mode = SceneGravityMode2D::Override; lateral.value = {90, 0};
    const ScopeId room{101};
    CHECK(world.CreateScope(room, "Lateral", lateral));
    bool physicsBeforeParticles = false, oldMembership = false;
    world.AddSystem(SystemPhase::Physics, "qualification.transfer.pool", [&](World& current, const FrameTime&, const Input&) {
        if (current.Steps() != 0) return;
        physicsBeforeParticles = pool.Get<ParticleSystem>().particles[0].velocity == pxVec2f(0, 0);
        CHECK(current.SetScope(pool, room));
        oldMembership = current.ScopeOf(pool) == ScopeId{};
    });
    world.Step(1.0 / 60, {});
    CHECK(physicsBeforeParticles && oldMembership && world.ScopeOf(pool) == room);
    const auto previous = pool.Get<ParticleSystem>().particles[0].velocity;
    CHECK(previous.x == 0 && std::abs(previous.y - 5) < 0.001);
    world.Step(1.0 / 60, {});
    const auto& particles = pool.Get<ParticleSystem>().particles;
    CHECK(std::abs(particles[0].velocity.x - 0.75) < 0.001 && particles[0].velocity.y == previous.y);
    CHECK(particles[1].velocity == pxVec2f(0, 0) && particles[1].position == pxPointf(0, 0));
    CHECK(SceneCollisionWorld2D(world) == nullptr);
}

TEST(SceneParticleMultiplyAndInactiveOverrideApplyScaleExactlyOnce) {
    for (const auto mode : {SceneGravityMode2D::Inherit, SceneGravityMode2D::Multiply}) {
        World world;
        auto pool = ScopedGravityPool(world, {0, 600}, false);
        SceneGravity2D gravity; gravity.mode = mode; gravity.multiplier = 0.5; gravity.value = {900, 0};
        const ScopeId room{101};
        CHECK(world.CreateScope(room, "Configured", gravity));
        CHECK(world.SetScope(pool, room));
        world.Step(1.0 / 60, {});
        const auto& particle = pool.Get<ParticleSystem>().particles[0];
        CHECK(particle.velocity.x == 0);
        CHECK(std::abs(particle.velocity.y - (mode == SceneGravityMode2D::Inherit ? 5.0 : 2.5)) < 0.001);
        CHECK(SceneCollisionWorld2D(world) == nullptr);
    }
}

TEST(SceneParticleInheritanceUsesTheSameEffectiveBackendGravityAsWorldOnlyPools) {
    for (const bool authoredSettings : {false, true}) {
        World world;
        if (authoredSettings) world.Create().Add<PhysicsSettings2D>().gravity = {0, 300};
        auto remote = world.Create();
        remote.Add<Transform2D>().position = {100000, 100000};
        remote.Add<Collider2D>(); // Allocate the normal scene backend, away from the pools.
        const std::array<ScopeId, 5> scopes{{{}, {11}, {12}, {13}, {14}}};
        CHECK(world.CreateScope(scopes[1], "No override"));
        SceneGravity2D configured; configured.mode = SceneGravityMode2D::Inherit;
        configured.value = {900, 0}; configured.multiplier = 0.5;
        CHECK(world.CreateScope(scopes[2], "Inherit", configured));
        configured.mode = SceneGravityMode2D::Multiply;
        CHECK(world.CreateScope(scopes[3], "Half", configured));
        configured.mode = SceneGravityMode2D::Override; configured.value = {0, 0};
        CHECK(world.CreateScope(scopes[4], "Explicit zero", configured));
        std::array<Entity, 5> pools;
        std::array<ParticleSystem, 5> references;
        const std::array<double, 5> expectedGravity{{600, 600, 600, 300, 0}};
        for (std::size_t i = 0; i < pools.size(); ++i) {
            pools[i] = world.Create();
            CHECK(pools[i].Add<ParticleSystem>().EmitDynamic({0, 0}, {0, 0}, 20));
            pools[i].Add<ParticlePhysics2D>();
            CHECK(world.SetScope(pools[i], scopes[i]));
            CHECK(references[i].EmitDynamic({0, 0}, {0, 0}, 20));
        }
        InstallSceneMovement2D(world);
        world.AddSystem(SystemPhase::PostUpdate, "qualification.effective.backend.gravity",
            [](World& current, const FrameTime&, const Input&) {
                auto* backend = ScenePhysicsWorld2D(current);
                CHECK(backend != nullptr);
                if (backend) backend->SetGravity({0, 600});
            });
        AddParticleStep(world); // The custom game command runs after sync and before particles.
        for (int tick = 1; tick <= 600; ++tick) {
            world.Step(1.0 / 60, {});
            for (std::size_t i = 0; i < references.size(); ++i) {
                ParticlePhysicsStats2D stats;
                Advance(references[i], {}, nullptr, {0, expectedGravity[i]}, 1.0 / 60, stats);
            }
            if (tick != 1 && tick != 60 && tick != 600) continue;
            CHECK(ParticlePhysicsErrors2D(world).empty());
            for (std::size_t i = 0; i < pools.size(); ++i) {
                const auto& actual = pools[i].Get<ParticleSystem>().particles;
                CHECK(actual.size() == 1 && references[i].particles.size() == 1);
                if (actual.size() == 1 && references[i].particles.size() == 1) {
                    CHECK(actual[0].position == references[i].particles[0].position);
                    CHECK(actual[0].velocity == references[i].particles[0].velocity);
                }
            }
            // The existing overload remains an authored-value query. It does not
            // silently replace authored defaults with the current backend command.
            CHECK(world.ResolveGravity(pools[2]).value == pxVec2(0, authoredSettings ? 300 : 1100));
        }
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
