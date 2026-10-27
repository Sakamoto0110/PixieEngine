#include "Check.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/World.hpp"
#include "pixie/runtime/Scene.hpp"
#include <array>
#include <cmath>
#include <stdexcept>

using namespace pixie;
namespace {
constexpr double dt = 1.0 / 60;
BodyHandle2D Platform(CollisionWorld2D& world, pxVec2 velocity = {60, 0}, double angular = 0) {
    RigidBody2D body;
    body.type = BodyType2D::Kinematic;
    body.initialVelocity = velocity;
    body.initialAngularVelocity = angular;
    BodyShape2D shape;
    shape.halfSize = {200, 10};
    return world.AddBody({0, 110}, 0, body, {&shape, 1});
}
void Tick(CollisionWorld2D& world, CharacterController2D& actor, double input = 0, bool jump = false) {
    actor.PrepareStep(world, input, jump, dt);
    world.StepBodies(dt);
    actor.FinishStep(world);
}
template<class F> bool WrongPhase(F run) {
    try { run(); } catch (const std::logic_error&) { return true; }
    return false;
}
}
TEST(CharacterFollowsTranslationReversalAndVerticalSupportExactlyOnce) {
    for (auto velocity : {pxVec2{60, 0}, pxVec2{0, -60}, pxVec2{0, 60}}) {
        CollisionWorld2D world;
        auto platform = Platform(world, velocity);
        CharacterController2D actor({0, 76});
        for (int i = 0; i < 60; ++i) Tick(world, actor);
        CHECK(std::abs(actor.State().position.x - velocity.x) < 0.4);
        CHECK(std::abs(actor.State().position.y - 76 - velocity.y) < 0.4);
        CHECK(actor.Coupling().support == platform);
        CHECK(!actor.Coupling().unsafePlacement);
        world.SetBodyVelocity(platform, velocity * -1.0, 0);
        for (int i = 0; i < 60; ++i) Tick(world, actor);
        CHECK(std::abs(actor.State().position.x) < 0.4);
        CHECK(std::abs(actor.State().position.y - 76) < 0.4);
    }
}
TEST(CharacterRotatingSupportUsesLocalPointAndInheritsClampedVelocityOnce) {
    CollisionWorld2D world;
    auto platform = Platform(world, {120, 0}, 0.2);
    CharacterSettings2D settings;
    settings.maxInheritedSpeed = 50;
    CharacterController2D actor({40, 76}, settings);
    for (int i = 0; i < 10; ++i) Tick(world, actor);
    const auto support = actor.Coupling();
    CHECK(support.support == platform);
    CHECK(std::abs(support.carried.y) > 0.05);
    const auto point = world.BodyWorldPoint(platform, support.localSupportPoint);
    CHECK(std::hypot(point.x - support.supportPoint.x, point.y - support.supportPoint.y) < 0.001);
    const auto motion = world.BodyMotion(platform);
    const auto center = world.BodyWorldPoint(platform, world.BodyMass(platform).localCenter);
    const auto velocity = world.BodyPointVelocity(platform, point);
    CHECK(std::abs(velocity.x - motion.velocity.x + motion.angularVelocity * (point.y - center.y)) < 0.001);
    CHECK(std::abs(velocity.y - motion.velocity.y - motion.angularVelocity * (point.x - center.x)) < 0.001);
    Tick(world, actor, 0, true);
    CHECK(actor.State().velocity.x > 45 && actor.State().velocity.x <= 50.001);
    CHECK(actor.State().velocity.y < -370);
    CHECK(!actor.State().grounded && !actor.Coupling().support.value);
    Tick(world, actor);
    CHECK(actor.State().velocity.x < 25); // Existing air braking; the support velocity is not added again.
}
TEST(CharacterDropsRemovedDisabledTeleportedOrRebuiltSupport) {
    for (int mutation = 0; mutation < 4; ++mutation) {
        CollisionWorld2D world;
        auto platform = Platform(world);
        CharacterController2D actor({0, 76});
        Tick(world, actor);
        const auto before = actor.State().position;
        if (mutation == 0) world.RemoveBody(platform);
        if (mutation == 1) world.SetBodyEnabled(platform, false);
        if (mutation == 2) world.TeleportBody(platform, {1000, 110}, 0);
        if (mutation == 3) world.Reset();
        Tick(world, actor);
        CHECK(!actor.Coupling().support.value);
        CHECK(std::abs(actor.State().position.x - before.x) < 0.001);
        CHECK(!actor.Coupling().unsafePlacement);
    }
}
TEST(CharacterMidPhaseTeleportAndFilterChangesCannotBecomeCarry) {
    for (bool filtered : {false, true}) {
        CollisionWorld2D world;
        auto platform = Platform(world);
        CharacterController2D actor({0, 76});
        actor.PrepareStep(world, 0, false, dt);
        if (filtered) world.SetCollisionFilter(world.BodyColliders(platform)[0], {1, 0});
        else world.TeleportBody(platform, {5, 110}, 0); // Within the carry limit, but still a teleport.
        world.StepBodies(dt);
        actor.FinishStep(world);
        CHECK(std::abs(actor.State().position.x) < 0.001);
        CHECK(std::abs(actor.Coupling().carried.x) < 0.001);
        CHECK(!actor.Coupling().unsafePlacement);
    }
}
TEST(CharacterPushBudgetDistinguishesLightAndHeavyBoxesWithoutLaunchingThem) {
    std::array<double, 2> distances{};
    for (int heavy = 0; heavy < 2; ++heavy) {
        CollisionWorld2D world;
        world.SetGravity({0, 1100});
        world.AddBox({0, 110}, {500, 10});
        RigidBody2D body;
        body.massOverride = heavy ? 1000 : 1;
        body.fixedRotation = true;
        BodyShape2D shape;
        shape.halfSize = {16, 16};
        shape.friction = 0;
        auto box = world.AddBody({40, 84}, 0, body, {&shape, 1});
        CharacterSettings2D settings;
        settings.maxPushForce = 600;
        settings.maxPushSpeed = 80;
        CharacterController2D actor({0, 76}, settings);
        double total = 0;
        for (int i = 0; i < 120; ++i) {
            Tick(world, actor, 1);
            total += actor.Coupling().pushedImpulse;
            CHECK(actor.Coupling().pushedImpulse <= settings.maxPushForce * dt + 1e-6);
            CHECK(!actor.Coupling().unsafePlacement);
            CHECK(world.BodyMotion(box).velocity.x <= 80.01);
            CHECK(world.BodyMotion(box).velocity.y > -1);
        }
        distances[heavy] = world.BodyMotion(box).position.x - 40;
        CHECK(total > 0);
    }
    std::fprintf(stderr, "coupling push displacement: light=%.6f heavy=%.6f\n", distances[0], distances[1]);
    CHECK(distances[0] > 32); // The light box travels more than its full width in two seconds.
    CHECK(distances[1] >= 0 && distances[1] < distances[0] * 0.05);
}
TEST(CharacterEffectiveSupportLoadIsBoundedAndDoesNotChangeNativeMass) {
    CollisionWorld2D world;
    RigidBody2D body;
    body.massOverride = 10;
    BodyShape2D shape;
    shape.halfSize = {100, 10};
    auto support = world.AddBody({0, 110}, 0, body, {&shape, 1});
    CharacterSettings2D settings;
    settings.maxSupportForce = 50;
    CharacterController2D actor({0, 76}, settings);
    Tick(world, actor);
    CHECK(std::abs(actor.Coupling().supportLoad - 50) < 1e-6);
    CHECK(std::abs(world.BodyMotion(support).velocity.y - 50 * dt / 10) < 0.001);
    CHECK(std::abs(world.BodyMass(support).mass - 10) < 0.001);
}
TEST(CharacterCompoundPushDeduplicatesBodyAndLeavingPlatformDropsCarry) {
    CollisionWorld2D world;
    RigidBody2D definition;
    definition.massOverride = 1000;
    definition.fixedRotation = true;
    std::array<BodyShape2D, 2> shapes;
    shapes[0].offset = {0, -8}; shapes[1].offset = {0, 8};
    for (auto& shape : shapes) shape.halfSize = {16, 8};
    auto box = world.AddBody({40, 76}, 0, definition, shapes);
    CharacterSettings2D settings;
    settings.gravity = 0;
    settings.maxPushForce = 1000000;
    CharacterController2D pusher({12, 76}, settings);
    CHECK(world.CapsuleContacts({12, 76}, 12, 12).size() == 2);
    pusher.PrepareStep(world, 1, false, dt);
    CHECK(std::abs(pusher.Coupling().pushedImpulse - settings.acceleration * dt) < 0.001);
    CHECK(std::abs(world.BodyMotion(box).velocity.x - settings.acceleration * dt / 1000) < 0.001);
    world.StepBodies(dt);
    pusher.FinishStep(world);

    CollisionWorld2D platforms;
    Platform(platforms);
    CharacterController2D rider({0, 76});
    Tick(platforms, rider);
    int ticks = 0;
    while (rider.Coupling().support.value && ++ticks < 180) Tick(platforms, rider, 1);
    CHECK(ticks < 180 && !rider.Coupling().support.value);
    const auto before = rider.State().position;
    for (int i = 0; i < 10; ++i) Tick(platforms, rider, 1);
    CHECK(std::abs(rider.State().position.x - before.x - settings.maxSpeed * dt * 10) < 0.4);
    CHECK(std::hypot(rider.Coupling().carried.x, rider.Coupling().carried.y) < 0.001);
}
TEST(CharacterCarryObstructionIsBlockedWithoutUnsafePlacementOrPlatformReversal) {
    CollisionWorld2D world;
    const auto platform = Platform(world);
    world.AddBox({30, 10}, {5, 80});
    CharacterController2D actor({0, 76});
    for (int i = 0; i < 60; ++i) Tick(world, actor);
    CHECK(actor.Coupling().blocked == CharacterBlocked2D::CarryObstructed);
    CHECK(!actor.Coupling().unsafePlacement);
    CHECK(actor.State().position.x > 12 && actor.State().position.x < 13.3);
    CHECK(world.BodyMotion(platform).velocity.x == 60);
}
TEST(CharacterReceivesMovingBodyWithoutASecondSimulatedCharacter) {
    CollisionWorld2D world;
    RigidBody2D body;
    body.initialVelocity = {120, 0};
    body.massOverride = 1000;
    body.fixedRotation = true;
    BodyShape2D shape;
    auto box = world.AddBody({-40, 0}, 0, body, {&shape, 1});
    CharacterSettings2D settings;
    settings.gravity = 0;
    CharacterController2D actor({}, settings);
    for (int i = 0; i < 60; ++i) {
        Tick(world, actor);
        CHECK(!actor.Coupling().unsafePlacement);
    }
    CHECK(actor.State().position.x > 80);
    CHECK(actor.Coupling().worldVelocity.x > 100);
    CHECK(world.Stats().bodies == 1 && world.BodyMotion(box).velocity.x > 100);
}
TEST(CharacterCrushAndUnsupportedCarryRetainLastPoseAndReportUnsafePlacement) {
    for (bool crush : {false, true}) {
        CollisionWorld2D world;
        auto platform = Platform(world, crush ? pxVec2{0, -60} : pxVec2{1200, 0});
        if (crush) world.AddBox({0, 39.8}, {200, 12});
        CharacterController2D actor({0, 76});
        Tick(world, actor);
        CHECK(actor.Coupling().unsafePlacement);
        CHECK(actor.Coupling().blocked == (crush ? CharacterBlocked2D::NoValidPlacement : CharacterBlocked2D::SupportStepLimit));
        CHECK(actor.State().position.x == 0 && actor.State().position.y == 76);
        CHECK(world.BodyMotion(platform).velocity == (crush ? pxVec2{0, -60} : pxVec2{1200, 0}));
    }
}
TEST(CharacterJumpAgainstBoxDoesNotLaunchTheBox) {
    for (double x : {0.0, -20.0}) {
        CollisionWorld2D world;
        world.AddBox({0, 110}, {200, 10});
        BodyShape2D shape;
        auto box = world.AddBody({0, 35}, 0, {}, {&shape, 1});
        CharacterController2D actor({x, 76});
        Tick(world, actor, 1, true); // Include sideways intent at a lower corner of the box.
        const auto motion = world.BodyMotion(box);
        CHECK(motion.position.x == 0 && motion.position.y == 35);
        CHECK(motion.velocity == pxVec2(0, 0) && motion.angularVelocity == 0);
        CHECK(actor.Coupling().pushedImpulse == 0 && !actor.Coupling().unsafePlacement);
    }
}
TEST(CharacterFallingBodyCrushReportsFailureWithoutReversingTheBody) {
    CollisionWorld2D world;
    world.AddBox({0, 110}, {200, 10});
    RigidBody2D definition;
    definition.initialVelocity = {0, 60};
    definition.massOverride = 1000;
    BodyShape2D shape;
    auto box = world.AddBody({0, 35}, 0, definition, {&shape, 1});
    CharacterSettings2D settings;
    settings.gravity = 0;
    CharacterController2D actor({0, 76}, settings);
    Tick(world, actor);
    CHECK(!actor.Coupling().unsafePlacement);
    const auto lastValid = actor.State().position;
    Tick(world, actor);
    CHECK(actor.Coupling().unsafePlacement && actor.Coupling().blocked == CharacterBlocked2D::NoValidPlacement);
    CHECK(actor.State().position == lastValid);
    CHECK(world.BodyMotion(box).velocity.y == 60);
}
TEST(CharacterCoupledPhaseGuardsResetAndSceneSingleStepPublication) {
    CollisionWorld2D collision;
    CharacterController2D controller;
    CHECK(WrongPhase([&] { controller.FinishStep(collision); }));
    controller.PrepareStep(collision, 0, false, dt);
    CHECK(WrongPhase([&] { controller.PrepareStep(collision, 0, false, dt); }));
    collision.Reset();
    controller.FinishStep(collision);
    CHECK(controller.Coupling().blocked == CharacterBlocked2D::WorldChanged);
    controller.Teleport({0, 0});
    CHECK(!controller.Coupling().unsafePlacement);

    World world;
    auto platform = world.Create("platform");
    platform.Add<Transform2D>().position = {0, 110};
    platform.Add<Collider2D>().halfSize = {200, 10};
    auto& body = platform.Add<RigidBody2D>();
    body.type = BodyType2D::Kinematic;
    body.initialVelocity = {60, 0};
    AddToScene(platform);
    auto actor = world.Create("rider");
    actor.Add<Transform2D>().position = {0, 76};
    actor.Add<CharacterMotor2D>();
    AddToScene(actor);
    auto other = world.Create("second rider");
    other.Add<Transform2D>().position = {40, 76};
    other.Add<CharacterMotor2D>();
    AddToScene(other);
    InstallSceneMovement2D(world);
    for (int i = 0; i < 60; ++i) world.Step(dt, {});
    CHECK(std::abs(platform.Get<Transform2D>().position.x - 60) < 0.01);
    CHECK(std::abs(actor.Get<Transform2D>().position.x - 60) < 0.4);
    CHECK(std::abs(other.Get<Transform2D>().position.x - 100) < 0.4);
    CHECK(SceneCollisionWorld2D(world)->Stats().bodies == 1);
    CHECK(SceneMovementErrors2D(world).empty());
    CHECK(CharacterCoupling2D(world, actor.Id())->worldVelocity.x > 59);
}
