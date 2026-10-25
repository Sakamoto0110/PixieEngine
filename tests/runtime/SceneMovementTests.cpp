#include "Check.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/World.hpp"
#include "pixie/runtime/Velocity2D.hpp"
#include "pixie/runtime/Acceleration2D.hpp"
#include <cmath>

using namespace pixie;
namespace {
Entity Floor(World& world, double y = 110) {
    auto entity = world.Create("floor");
    entity.Add<Transform2D>().position = {0, y};
    entity.Add<Collider2D>().halfSize = {500, 10};
    AddToScene(entity);
    return entity;
}
Entity Actor(World& world, pxPoint position = {}) {
    auto entity = world.Create("character");
    entity.Add<Transform2D>().position = position;
    entity.Add<CharacterMotor2D>();
    AddToScene(entity);
    return entity;
}
void Steps(World& world, int count, const Input& input = {}) {
    for (int i = 0; i < count; ++i) world.Step(1.0 / 60, input);
}
}
TEST(PhysicsAuthoringAdmissionIsNativeSymmetricAndDoesNotDropLegacyData) {
    World world;
    ComponentTypes types;
    DescribeRuntimeComponents(types);
    const auto& motor = types.All()[types.IndexOf("CharacterMotor2D")];
    const auto& collider = types.All()[types.IndexOf("Collider2D")];
    const auto& body = types.All()[types.IndexOf("RigidBody2D")];
    const auto& settings = types.All()[types.IndexOf("PhysicsSettings2D")];
    CHECK(collider.fields[1].name == "halfSize");
    CHECK(collider.fields[1].metadata.label == "size" && collider.fields[1].metadata.unitFactor == 2);
    auto entity = world.Create();
    CHECK(motor.AdditionError(world, entity.Id()).empty());
    entity.Add<CharacterMotor2D>();
    CHECK(!body.AdditionError(world, entity.Id()).empty());
    CHECK(!collider.AdditionError(world, entity.Id()).empty());
    CHECK(motor.AdditionError(world, entity.Id()).empty()); // idempotent re-add
    entity.Remove<CharacterMotor2D>();
    entity.Add<Collider2D>();
    CHECK(!motor.AdditionError(world, entity.Id()).empty());
    CHECK(body.AdditionError(world, entity.Id()).empty());
    entity.Remove<Collider2D>();
    entity.Add<RigidBody2D>();
    CHECK(!motor.AdditionError(world, entity.Id()).empty());
    // Low-level native creation stays permissive: imported invalid data can be repaired.
    entity.Add<CharacterMotor2D>();
    entity.Add<Transform2D>();
    InstallSceneMovement2D(world);
    SyncSceneMovement2D(world);
    CHECK(!SceneMovementErrors2D(world).empty());
    CHECK(entity.Has<RigidBody2D>() && entity.Has<CharacterMotor2D>());
    auto config = world.Create();
    CHECK(settings.singleton && settings.AdditionError(world, config.Id()).empty());
    config.Add<PhysicsSettings2D>();
    CHECK(settings.AdditionError(world, config.Id()).empty());
    CHECK(!settings.AdditionError(world, entity.Id()).empty());
}

TEST(SceneCapsuleMinimumIsDiagnosedInEditAndRecoversAfterCorrection) {
    World world;
    auto actor = Actor(world);
    world.Create().Add<PhysicsSettings2D>().unitsPerMeter = 1000;
    actor.Get<CharacterMotor2D>().radius = 5;
    InstallSceneMovement2D(world);
    SyncSceneMovement2D(world);
    CHECK(CharacterMotion2D(world, actor.Id()) == nullptr);
    const auto errors = SceneMovementErrors2D(world);
    CHECK(!errors.empty() && errors[0].find("10 world units") != std::string::npos);
    actor.Get<CharacterMotor2D>().radius = 12;
    SyncSceneMovement2D(world);
    CHECK(SceneMovementErrors2D(world).empty());
    CHECK(CharacterMotion2D(world, actor.Id()) != nullptr);
}

TEST(SceneMovementComponentsRoundTripAuthoredSettingsOnly) {
    World source;
    ComponentTypes types;
    DescribeRuntimeComponents(types);
    Floor(source);
    auto actor = Actor(source);
    actor.Get<CharacterMotor2D>().maxSpeed = 317;
    actor.Add<KeyboardControl2D>();
    InstallSceneMovement2D(source);
    Steps(source, 100);
    CHECK(CharacterMotion2D(source, actor.Id())->grounded);
    auto text = WriteScene(source, types);
    CHECK(text.find("CharacterMotor2D") != std::string::npos);
    CHECK(text.find("grounded") == std::string::npos);
    World loaded;
    CHECK(ReadScene(loaded, types, text).ok);
    CHECK(loaded.Registry().view<CharacterMotor2D>().size() == 1);
    for (auto [id, motor] : loaded.Registry().view<CharacterMotor2D>().each()) {
        CHECK(motor.maxSpeed == 317);
        CHECK(loaded.Registry().all_of<KeyboardControl2D>(id));
    }
    CHECK(CharacterMotion2D(loaded, *loaded.Registry().view<CharacterMotor2D>().begin()) == nullptr);
}
TEST(SceneMovementInstallationIsIdempotentAndDoesNotDoubleIntegrateVelocity) {
    World world;
    auto actor = Actor(world);
    actor.Add<Velocity2D>().linear = {10000, 10000};
    actor.Add<Acceleration2D>().linear = {10000, 10000};
    InstallSceneMovement2D(world);
    InstallSceneMovement2D(world);
    world.Step(1.0 / 60, {});
    const auto p = actor.Get<Transform2D>().position;
    CHECK(std::abs(p.x) < 1e-9);
    CHECK(std::abs(p.y - 1100.0 / 3600) < 0.001);
    CHECK(actor.Get<Velocity2D>().linear.x == 10000);
}
TEST(SceneMovementKeyboardAndRuntimeCommandsDriveTheSameController) {
    World world;
    Floor(world);
    auto actor = Actor(world);
    actor.Add<KeyboardControl2D>();
    InstallSceneMovement2D(world);
    Steps(world, 90);
    Input input;
    input.SetKey(static_cast<int>(Key::D), true);
    Steps(world, 30, input);
    CHECK(actor.Get<Transform2D>().position.x > 70);
    actor.Get<KeyboardControl2D>().enabled = false;
    actor.Add<CharacterCommand2D>().horizontal = -1;
    Steps(world, 45);
    CHECK(CharacterMotion2D(world, actor.Id())->velocity.x < -190);
    actor.Get<CharacterCommand2D>().jumpPressed = true;
    world.Step(1.0 / 60, {});
    CHECK(CharacterMotion2D(world, actor.Id())->velocity.y < -300);
    CHECK(!actor.Get<CharacterCommand2D>().jumpPressed);
}
TEST(SceneColliderEditsDisableAndRemovalDoNotLeaveGhostGeometry) {
    World world;
    auto floor = Floor(world);
    auto actor = Actor(world);
    InstallSceneMovement2D(world);
    Steps(world, 120);
    CHECK(CharacterMotion2D(world, actor.Id())->grounded);
    floor.Get<Transform2D>().position.y = 210;
    Steps(world, 90);
    CHECK(std::abs(actor.Get<Transform2D>().position.y - 176.16) < 0.01);
    floor.Get<Collider2D>().enabled = false;
    Steps(world, 30);
    CHECK(actor.Get<Transform2D>().position.y > 200);
    floor.Remove<Collider2D>();
    Steps(world, 10);
    CHECK(SceneMovementErrors2D(world).empty());
}
TEST(SceneMotorTuningKeepsMomentumAndAuthoredTeleportResetsIt) {
    World world;
    auto actor = Actor(world);
    actor.Get<CharacterMotor2D>().gravityScale = 0;
    actor.Add<CharacterCommand2D>().horizontal = 1;
    InstallSceneMovement2D(world);
    world.Step(1.0 / 60, {});
    const double first = CharacterMotion2D(world, actor.Id())->velocity.x;
    actor.Get<CharacterMotor2D>().maxSpeed = 400;
    world.Step(1.0 / 60, {});
    CHECK(CharacterMotion2D(world, actor.Id())->velocity.x > first);
    actor.Get<Transform2D>().position = {100, 200};
    world.Step(1.0 / 60, {});
    CHECK(actor.Get<Transform2D>().position.x > 100 && actor.Get<Transform2D>().position.x < 101);
    CHECK(actor.Get<Transform2D>().position.y == 200);
}
TEST(SceneMovementInvalidDataProducesDiagnosticsAndRecovers) {
    World world;
    auto floor = Floor(world);
    floor.Get<Collider2D>().halfSize.x = -1;
    auto actor = Actor(world);
    actor.Get<CharacterMotor2D>().radius = -1;
    InstallSceneMovement2D(world);
    world.Step(1.0 / 60, {});
    CHECK(SceneMovementErrors2D(world).size() == 2);
    CHECK(CharacterMotion2D(world, actor.Id()) == nullptr);
    floor.Get<Collider2D>().halfSize.x = 500;
    actor.Get<CharacterMotor2D>().radius = 12;
    Steps(world, 120);
    CHECK(SceneMovementErrors2D(world).empty());
    CHECK(CharacterMotion2D(world, actor.Id())->grounded);
}
TEST(SceneMovementStateDoesNotFollowDeletedOrReusedEntities) {
    World world;
    auto actor = Actor(world);
    InstallSceneMovement2D(world);
    Steps(world, 3);
    const auto old = actor.Id();
    actor.Destroy();
    auto replacement = Actor(world, {100, 200});
    SyncSceneMovement2D(world);
    CHECK(CharacterMotion2D(world, old) == nullptr);
    CHECK(CharacterMotion2D(world, replacement.Id())->position.x == 100);
    replacement.Remove<CharacterMotor2D>();
    replacement.Add<Velocity2D>().linear = {60, 0};
    world.Step(1.0 / 60, {});
    CHECK(replacement.Get<Transform2D>().position.x == 101);
    CHECK(CharacterMotion2D(world, replacement.Id()) == nullptr);
}
TEST(SceneGizmosRespectTransformedOffsetsAndSupportColliderOnlyPicking) {
    World world;
    auto floor = Floor(world, 0);
    floor.Get<Collider2D>().halfSize = {20, 10};
    floor.Get<Collider2D>().offset = {10, 0};
    floor.Get<Transform2D>().scale = {2, 1};
    auto outline = MovementOutline2D(world, floor.Id());
    CHECK(outline.size() == 4);
    CHECK(outline[0].x == -20 && outline[1].x == 60);
    CHECK(PickMovementGizmo2D(world, {59, 0}, 0) == floor.Id());
    CHECK(PickMovementGizmo2D(world, {70, 0}, 0) == entt::null);
    CHECK(PickMovementGizmosIn2D(world, {-30, -20, 100, 40}).size() == 1);
    floor.Get<Collider2D>().kind = ColliderKind2D::Circle;
    SyncSceneMovement2D(world);
    CHECK(!SceneMovementErrors2D(world).empty());
}
TEST(SceneMovementEditSyncDoesNotAdvanceTimeOrCharacterMotion) {
    World world;
    Floor(world);
    auto actor = Actor(world, {0, 40});
    InstallSceneMovement2D(world);
    for (int i = 0; i < 20; ++i) SyncSceneMovement2D(world);
    CHECK(world.Steps() == 0);
    CHECK(CharacterMotion2D(world, actor.Id())->position.y == 40);
    CHECK(CharacterMotion2D(world, actor.Id())->velocity.y == 0);
}
TEST(SceneQueriesShareColliderStateAndMapHitsBackToSceneEntities) {
    World world;
    CHECK(SceneCollisionWorld2D(world) == nullptr);
    auto floor = Floor(world);
    InstallSceneMovement2D(world);
    SyncSceneMovement2D(world);
    const auto* collision = SceneCollisionWorld2D(world);
    CHECK(collision != nullptr);
    const auto hit = collision->RayCast({0, 0}, {0, 200});
    CHECK(hit.has_value());
    if (hit) CHECK(SceneColliderEntity2D(world, hit->collider) == floor.Id());
    floor.Get<Collider2D>().enabled = false;
    SyncSceneMovement2D(world);
    CHECK(!collision->RayCast({0, 0}, {0, 200}));
    if (hit) CHECK(SceneColliderEntity2D(world, hit->collider) == entt::null);
}
