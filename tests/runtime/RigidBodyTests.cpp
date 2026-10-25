#include "Check.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/World.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/Velocity2D.hpp"
#include "pixie/runtime/Acceleration2D.hpp"
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
using namespace pixie;
namespace {
template<class F> bool Invalid(F run) {
    try { run(); } catch (const std::invalid_argument&) { return true; }
    return false;
}
BodyHandle2D Box(CollisionWorld2D& world, pxPoint position = {}, RigidBody2D body = {}) {
    BodyShape2D shape;
    return world.AddBody(position, 0, body, {&shape, 1});
}
void Step(CollisionWorld2D& world, int n) { for (int i = 0; i < n; ++i) world.StepBodies(1.0 / 60); }
Entity SceneBox(World& world) {
    auto box = world.Create("box");
    box.Add<Transform2D>();
    box.Add<Collider2D>().halfSize = {16, 16};
    box.Add<RigidBody2D>();
    AddToScene(box);
    return box;
}
}
TEST(BodyMassAndTorqueUseSquaredLengthConversion) {
    for (double scale : {16.0, 32.0, 64.0}) {
        CollisionWorld2D world(scale);
        BodyShape2D shape;
        shape.halfSize = {scale / 2, scale / 2};
        shape.density = 2;
        auto body = world.AddBody({}, 0, {}, {&shape, 1});
        const auto mass = world.BodyMass(body);
        CHECK(std::abs(mass.mass - 2) < 1e-5);
        CHECK(std::abs(mass.inertia - 2 * scale * scale / 6) < 0.001);
        world.ApplyTorque(body, mass.inertia * 3);
        world.StepBodies(1.0 / 60);
        CHECK(std::abs(world.BodyMotion(body).angularVelocity - 3.0 / 60) < 0.0001);
        world.ApplyAngularImpulse(body, mass.inertia * 2);
        CHECK(std::abs(world.BodyMotion(body).angularVelocity - (2 + 3.0 / 60)) < 0.0001);
    }
}
TEST(BodyCompoundShapesHaveDistinctQueryIdentityAndComputedCenter) {
    CollisionWorld2D world;
    std::array<BodyShape2D, 2> shapes;
    shapes[0].offset = {-32, 0};
    shapes[1].offset = {32, 0};
    shapes[1].density = 3;
    auto body = world.AddBody({}, 0, {}, shapes);
    const auto mass = world.BodyMass(body);
    CHECK(std::abs(mass.mass - 4) < 1e-5);
    CHECK(std::abs(mass.localCenter.x - 16) < 1e-5);
    CHECK(std::abs(mass.inertia - (4.0 * 1024 / 6 + 3072)) < 0.01);
    auto left = world.RayCast({-32, -100}, {0, 200});
    auto right = world.RayCast({32, -100}, {0, 200});
    CHECK(left && right);
    if (left && right) CHECK(left->collider != right->collider);
    auto handles = world.BodyColliders(body);
    CHECK(world.RemoveBody(body));
    CHECK(!world.Contains(handles[0]));
    CHECK(!world.ContainsBody(body));
    CHECK(Invalid([&] { world.BodyMass(body); }));
}
TEST(BodyMassOverrideScalesInertiaAndImpulseResponse) {
    CollisionWorld2D world;
    RigidBody2D definition;
    definition.massOverride = 10;
    auto body = Box(world, {}, definition);
    auto mass = world.BodyMass(body);
    CHECK(std::abs(mass.mass - 10) < 0.001);
    CHECK(std::abs(mass.inertia - 10.0 * 1024 / 6) < 0.01);
    world.ApplyImpulse(body, {100, 0}, {0, 16});
    CHECK(std::abs(world.BodyMotion(body).velocity.x - 10) < 0.001);
    CHECK(world.BodyMotion(body).angularVelocity < -0.9);
}
TEST(BodyGravityTypesAndScaleRemainDistinct) {
    CollisionWorld2D world;
    world.SetGravity({0, 600});
    RigidBody2D definition;
    auto dynamic = Box(world, {-100, 0});
    definition.gravityScale = 0;
    auto floating = Box(world, {0, 0}, definition);
    definition.type = BodyType2D::Kinematic;
    definition.initialVelocity = {30, 0};
    auto kinematic = Box(world, {100, 0}, definition);
    definition.type = BodyType2D::Static;
    auto fixed = Box(world, {200, 0}, definition);
    Step(world, 30);
    CHECK(std::abs(world.BodyMotion(dynamic).velocity.y - 300) < 0.01);
    CHECK(world.BodyMotion(dynamic).position.y > 74);
    CHECK(std::abs(world.BodyMotion(floating).position.y) < 0.001);
    CHECK(std::abs(world.BodyMotion(kinematic).position.x - 115) < 0.001);
    CHECK(std::abs(world.BodyMotion(fixed).position.x - 200) < 0.001);
}
TEST(BodyGravityChangesWakeRestingBodiesButUnchangedSyncPreservesSleep) {
    CollisionWorld2D world;
    auto body = Box(world);
    for (int i = 0; i < 90; ++i) {
        world.SetGravity({0, 0});
        world.StepBodies(1.0 / 60);
    }
    CHECK(!world.BodyMotion(body).awake);
    world.SetGravity({0, 600});
    CHECK(world.BodyMotion(body).awake);
    world.StepBodies(1.0 / 60);
    CHECK(std::abs(world.BodyMotion(body).velocity.y - 10) < 0.001);
}
TEST(BodyRotationLockAndTeleportAreExplicit) {
    CollisionWorld2D world;
    RigidBody2D definition;
    definition.fixedRotation = true;
    auto body = Box(world, {}, definition);
    world.ApplyImpulse(body, {100, 0}, {0, 16});
    Step(world, 10);
    CHECK(std::abs(world.BodyMotion(body).rotation) < 1e-8);
    world.TeleportBody(body, {30, 40}, 0.7);
    CHECK(std::abs(world.BodyMotion(body).velocity.x) < 1e-8);
    CHECK(std::abs(world.BodyMotion(body).position.x - 30) < 1e-8);
    CHECK(std::abs(world.BodyMotion(body).rotation - 0.7) < 0.0001);
}
TEST(BodyInvalidDataDoesNotCreateGhostGeometry) {
    CollisionWorld2D world;
    BodyShape2D shape;
    shape.halfSize.x = -1;
    CHECK(Invalid([&] { world.AddBody({}, 0, {}, {&shape, 1}); }));
    CHECK(world.Size() == 0);
    shape.halfSize.x = 16;
    shape.density = 0;
    CHECK(Invalid([&] { world.AddBody({}, 0, {}, {&shape, 1}); }));
    shape.density = 1;
    shape.restitution = 2;
    CHECK(Invalid([&] { world.AddBody({}, 0, {}, {&shape, 1}); }));
    CHECK(Invalid([&] { world.StepBodies(0.5); }));
    CHECK(Invalid([&] { world.SetGravity({0, std::numeric_limits<double>::quiet_NaN()}); }));
    CHECK(world.Size() == 0);
    RigidBody2D underflow;
    underflow.massOverride = 1e-30;
    CHECK(Invalid([&] { Box(world, {}, underflow); }));
    CollisionWorld2D other;
    auto foreign = Box(other);
    CHECK(Invalid([&] { world.ApplyImpulse(foreign, {1, 0}, {}); }));
}
TEST(BodyCircleCapsuleMassAndOneStepForceHavePhysicalUnits) {
    CollisionWorld2D world;
    BodyShape2D shape;
    shape.kind = BodyShapeKind2D::Circle;
    shape.radius = 16;
    auto circle = world.AddBody({-100, 0}, 0, {}, {&shape, 1});
    const double pi = std::acos(-1.0);
    CHECK(std::abs(world.BodyMass(circle).mass - pi / 4) < 0.0001);
    shape.kind = BodyShapeKind2D::Capsule;
    shape.halfSegment = 16;
    auto capsule = world.AddBody({100, 0}, 0, {}, {&shape, 1});
    CHECK(std::abs(world.BodyMass(capsule).mass - (1 + pi / 4)) < 0.0001);
    const auto mass = world.BodyMass(circle).mass;
    world.ApplyForce(circle, {mass * 60, 0}, {-100, 0});
    world.StepBodies(1.0 / 60);
    CHECK(std::abs(world.BodyMotion(circle).velocity.x - 1) < 0.001);
    world.StepBodies(1.0 / 60);
    CHECK(std::abs(world.BodyMotion(circle).velocity.x - 1) < 0.001);
}
TEST(BodyRampContactsProduceRotationWithoutScriptedAngularMotion) {
    CollisionWorld2D world;
    world.SetGravity({0, 600});
    world.AddBox({0, 100}, {160, 10}, 0.3);
    auto box = Box(world, {-50, 0});
    Step(world, 35);
    const auto motion = world.BodyMotion(box);
    CHECK(std::abs(motion.rotation) > 0.05);
    CHECK(motion.position.y < 100);
    CHECK(std::isfinite(motion.angularVelocity));
}
TEST(SceneBodyUsesSharedQueryWorldAndExclusiveIntegration) {
    World world;
    auto box = SceneBox(world);
    box.Add<Velocity2D>().linear = {10000, 10000};
    box.Add<Acceleration2D>().linear = {10000, 10000};
    auto settings = world.Create("physics");
    settings.Add<PhysicsSettings2D>().gravity = {0, 600};
    InstallSceneMovement2D(world);
    world.Step(1.0 / 60, {});
    const auto position = box.Get<Transform2D>().position;
    CHECK(std::abs(position.x) < 0.001);
    CHECK(position.y > 0 && position.y < 0.2);
    auto* physics = ScenePhysicsWorld2D(world);
    CHECK(physics != nullptr);
    CHECK(physics == SceneCollisionWorld2D(world));
    if (!physics) return;
    auto hit = physics->RayCast({0, -100}, {0, 200});
    CHECK(hit.has_value());
    if (hit) CHECK(SceneColliderEntity2D(world, hit->collider) == box.Id());
    physics->ApplyImpulse(SceneBody2D(world, box.Id()), {60, 0}, position);
    world.Step(1.0 / 60, {});
    CHECK(box.Get<Transform2D>().position.x > 0.9);
    CHECK(box.Get<Velocity2D>().linear.x == 10000);
}
TEST(SceneBodyRetuneAndTeleportHaveDifferentVelocityPolicy) {
    World world;
    auto box = SceneBox(world);
    box.Get<RigidBody2D>().initialVelocity = {60, 0};
    InstallSceneMovement2D(world);
    world.Step(1.0 / 60, {});
    box.Get<Collider2D>().density = 2;
    SyncSceneMovement2D(world);
    auto* physics = ScenePhysicsWorld2D(world);
    CHECK(std::abs(physics->BodyMotion(SceneBody2D(world, box.Id())).velocity.x - 60) < 0.001);
    CHECK(std::abs(physics->BodyMass(SceneBody2D(world, box.Id())).mass - 2) < 0.001);
    box.Get<Transform2D>().position = {100, 100};
    SyncSceneMovement2D(world);
    CHECK(std::abs(physics->BodyMotion(SceneBody2D(world, box.Id())).velocity.x) < 0.001);
    box.Get<Collider2D>().halfSize.x = -1;
    SyncSceneMovement2D(world);
    CHECK(!SceneMovementErrors2D(world).empty());
    CHECK(physics->Size() == 0);
    box.Get<Collider2D>().halfSize.x = 16;
    SyncSceneMovement2D(world);
    CHECK(SceneMovementErrors2D(world).empty());
    CHECK(physics->Size() == 1);
    box.Destroy();
    SyncSceneMovement2D(world);
    CHECK(physics->Size() == 0);
}
TEST(SceneWorldGravityScalesMotorsAndDetectsDuplicateSettings) {
    World world;
    auto actor = world.Create("motor");
    actor.Add<Transform2D>();
    actor.Add<CharacterMotor2D>().gravityScale = 0.5;
    auto config = world.Create("physics");
    config.Add<PhysicsSettings2D>().gravity = {0, 600};
    InstallSceneMovement2D(world);
    world.Step(1.0 / 60, {});
    CHECK(std::abs(CharacterMotion2D(world, actor.Id())->velocity.y - 5) < 0.001);
    auto duplicate = world.Create("duplicate");
    duplicate.Add<PhysicsSettings2D>();
    SyncSceneMovement2D(world);
    CHECK(!SceneMovementErrors2D(world).empty());
    CHECK(ScenePhysicsWorld2D(world) == nullptr);
    duplicate.Destroy();
    SyncSceneMovement2D(world);
    CHECK(SceneMovementErrors2D(world).empty());
    config.Get<PhysicsSettings2D>().gravity = {100, 600};
    SyncSceneMovement2D(world);
    CHECK(!SceneMovementErrors2D(world).empty());
    CHECK(CharacterMotion2D(world, actor.Id()) == nullptr);
}
TEST(SceneBodyAuthoredStateRoundTripsWithoutBackendMotion) {
    World source;
    auto body = SceneBox(source);
    body.Get<RigidBody2D>().massOverride = 5;
    auto config = source.Create("physics");
    config.Add<PhysicsSettings2D>().gravity = {0, 400};
    AddToScene(config);
    ComponentTypes types;
    DescribeRuntimeComponents(types);
    const auto text = WriteScene(source, types);
    World copy;
    CHECK(ReadScene(copy, types, text).ok);
    CHECK(copy.Registry().view<RigidBody2D>().size() == 1);
    CHECK(copy.Registry().view<PhysicsSettings2D>().size() == 1);
    InstallSceneMovement2D(copy);
    SyncSceneMovement2D(copy);
    CHECK(SceneMovementErrors2D(copy).empty());
    auto id = *copy.Registry().view<RigidBody2D>().begin();
    CHECK(copy.Registry().get<RigidBody2D>(id).massOverride == 5);
    CHECK(copy.Registry().get<Transform2D>(id).position == pxPoint(0, 0));
    CHECK(text.find("awake") == std::string::npos);
}
TEST(SceneScaleChangeRequiresResetAndCannotResumeOnTheFollowingTick) {
    World world;
    auto body = SceneBox(world);
    auto settings = world.Create("settings");
    settings.Add<PhysicsSettings2D>();
    InstallSceneMovement2D(world);
    world.Step(1.0 / 60, {});
    settings.Get<PhysicsSettings2D>().unitsPerMeter = 64;
    world.Step(1.0 / 60, {});
    CHECK(ScenePhysicsWorld2D(world) == nullptr);
    world.Step(1.0 / 60, {});
    CHECK(ScenePhysicsWorld2D(world) == nullptr);
    CHECK(!SceneMovementErrors2D(world).empty());
    CHECK(SceneBody2D(world, body.Id()).value == 0);
}
TEST(SceneBodiesDoNotCoupleTwoWorldsAndOldHandlesNeverRevive) {
    World a, b;
    auto bodyA = SceneBox(a), bodyB = SceneBox(b);
    bodyA.Get<RigidBody2D>().initialAngularVelocity = 1;
    InstallSceneMovement2D(a);
    InstallSceneMovement2D(b);
    a.Step(1.0 / 60, {});
    SyncSceneMovement2D(b);
    CHECK(std::abs(bodyA.Get<Transform2D>().rotation) > 0.01);
    CHECK(bodyB.Get<Transform2D>().rotation == 0);
    auto handle = SceneBody2D(a, bodyA.Id());
    auto duplicate = a.Create("invalid settings 1"); duplicate.Add<PhysicsSettings2D>();
    auto duplicate2 = a.Create("invalid settings 2"); duplicate2.Add<PhysicsSettings2D>();
    SyncSceneMovement2D(a);
    duplicate2.Destroy();
    SyncSceneMovement2D(a);
    CHECK(SceneBody2D(a, bodyA.Id()) != handle);
    CHECK(!ScenePhysicsWorld2D(a)->ContainsBody(handle));
}
