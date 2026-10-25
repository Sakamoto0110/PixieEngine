#include "Check.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/ShapeRenderer.hpp"
#include "pixie/Camera2D.hpp"
#include <nlohmann/json.hpp>
#include <cmath>
#include <numbers>

using namespace pixie;
namespace {
Entity Compound(World& world, bool dynamic = true) {
    auto entity = world.Create("compound");
    entity.Add<Transform2D>();
    auto& shapes = entity.Add<CompoundCollider2D>().shapes;
    shapes.resize(2);
    shapes[0].value.halfSize = {16, 8};
    shapes[0].value.density = 2;
    shapes[1].value.kind = ColliderKind2D::Circle;
    shapes[1].value.radius = 8;
    shapes[1].value.offset = {32, 0};
    if (dynamic) entity.Add<RigidBody2D>();
    AddToScene(entity);
    return entity;
}
}

TEST(CompoundSceneUsesOneBodyWithIndependentShapeMassAndIdentity) {
    World world;
    auto entity = Compound(world);
    world.Create().Add<PhysicsSettings2D>().gravity = {};
    InstallSceneMovement2D(world);
    SyncSceneMovement2D(world);
    CHECK(SceneMovementErrors2D(world).empty());
    const auto body = SceneBody2D(world, entity.Id());
    auto* physics = ScenePhysicsWorld2D(world);
    const auto colliders = physics->BodyColliders(body);
    CHECK(colliders.size() == 2 && colliders[0] != colliders[1]);
    CHECK(SceneColliderEntity2D(world, colliders[1]) == entity.Id());
    const auto mass = physics->BodyMass(body);
    const double circleMass = std::numbers::pi / 16;
    CHECK(std::abs(mass.mass - 1 - circleMass) < 1e-5);
    CHECK(std::abs(mass.localCenter.x - 32 * circleMass / (1 + circleMass)) < 1e-4);
    physics->ApplyImpulse(body, {12, 0}, {0, 20});
    CHECK(std::abs(physics->BodyMotion(body).angularVelocity) > 0.01);
    for (int i = 0; i < 60; ++i) world.Step(1.0 / 60, {});
    CHECK(SceneBody2D(world, entity.Id()) == body); // no per-step geometry rebuild
    CHECK(std::abs(entity.Get<Transform2D>().rotation) > 0.01);
}

TEST(StaticCompoundHandlesEmptyDisabledAndInvalidReplacementWithoutGhosts) {
    World world;
    auto entity = Compound(world, false);
    InstallSceneMovement2D(world);
    SyncSceneMovement2D(world);
    auto* physics = ScenePhysicsWorld2D(world);
    const auto original = SceneBody2D(world, entity.Id());
    CHECK(physics->BodyColliders(original).size() == 2);
    auto& compound = entity.Get<CompoundCollider2D>();
    compound.shapes[1].value.radius = -1;
    SyncSceneMovement2D(world);
    CHECK(!physics->ContainsBody(original));
    CHECK(SceneBody2D(world, entity.Id()).value == 0 && !SceneMovementErrors2D(world).empty());
    compound.shapes[1].value.radius = 8;
    SyncSceneMovement2D(world);
    CHECK(SceneMovementErrors2D(world).empty());
    const auto recovered = SceneBody2D(world, entity.Id());
    CHECK(recovered.value != 0 && recovered != original);
    compound.shapes[1].value.enabled = false;
    SyncSceneMovement2D(world);
    CHECK(physics->BodyColliders(SceneBody2D(world, entity.Id())).size() == 1);
    compound.shapes.clear();
    SyncSceneMovement2D(world);
    CHECK(SceneBody2D(world, entity.Id()).value == 0 && SceneMovementErrors2D(world).empty());
    compound.shapes.resize(1);
    SyncSceneMovement2D(world);
    CHECK(SceneBody2D(world, entity.Id()).value != 0);
    compound.enabled = false;
    SyncSceneMovement2D(world);
    CHECK(SceneBody2D(world, entity.Id()).value == 0);
}

TEST(CompoundGizmosAndQueriesRespectLocalRotationScaleAndGaps) {
    World world;
    auto entity = Compound(world, false);
    auto& t = entity.Get<Transform2D>();
    t.position = {100, 50};
    t.scale = {-2, 1};
    auto& shapes = entity.Get<CompoundCollider2D>().shapes;
    shapes[0].value.halfSize = {12, 4};
    shapes[0].value.rotation = std::numbers::pi / 2;
    shapes[1].value.kind = ColliderKind2D::Box;
    shapes[1].value.halfSize = {4, 4};
    shapes[1].value.offset = {40, 0};
    InstallSceneMovement2D(world);
    SyncSceneMovement2D(world);
    CHECK(SceneMovementErrors2D(world).empty());
    const auto outlines = MovementOutlines2D(world, entity.Id());
    CHECK(outlines.size() == 2 && outlines[0].size() == 4);
    CHECK(PickMovementGizmo2D(world, {100, 60}, 0) == entity.Id());
    CHECK(PickMovementGizmo2D(world, {20, 50}, 0) == entity.Id());
    CHECK(PickMovementGizmo2D(world, {60, 50}, 0) == entt::null);
    CHECK(PickMovementGizmosIn2D(world, {10, 35, 100, 30}).size() == 1);
    CHECK(PickMovementGizmosIn2D(world, {90, 35, 20, 30}).empty());
    const auto* physics = SceneCollisionWorld2D(world);
    CHECK(physics->RayCast({100, 20}, {0, 60}).has_value());
    CHECK(!physics->RayCast({60, 20}, {0, 60}).has_value());
}

TEST(CompoundRebuildPreservesMotionAndSensorMassIsZero) {
    World world;
    auto entity = Compound(world);
    world.Create().Add<PhysicsSettings2D>().gravity = {};
    InstallSceneMovement2D(world);
    SyncSceneMovement2D(world);
    auto* physics = ScenePhysicsWorld2D(world);
    auto before = SceneBody2D(world, entity.Id());
    physics->SetBodyVelocity(before, {30, 5}, 0.5);
    entity.Get<CompoundCollider2D>().shapes[1].value.sensor = true;
    SyncSceneMovement2D(world);
    auto after = SceneBody2D(world, entity.Id());
    CHECK(before != after && !physics->ContainsBody(before));
    CHECK(std::abs(physics->BodyMass(after).mass - 1) < 1e-5);
    CHECK(std::abs(physics->BodyMotion(after).velocity.x - 30) < 1e-5);
    CHECK(std::abs(physics->BodyMotion(after).angularVelocity - 0.5) < 1e-5);
}

TEST(RecordListsRoundTripUnknownFieldsWithTheirItemsAndRejectMalformedListsAtomically) {
    World source;
    ComponentTypes types;
    DescribeRuntimeComponents(types);
    auto entity = Compound(source);
    using Json = nlohmann::json;
    auto document = Json::parse(WriteScene(source, types));
    auto& items = document["entities"][0]["components"]["CompoundCollider2D"]["shapes"];
    items[0]["futureMaterial"] = { {"coating", "first"} };
    items[1]["futureMaterial"] = { {"coating", "second"} };
    items[1]["v"] = 3;
    World loaded;
    CHECK(ReadScene(loaded, types, document.dump()).ok);
    auto& records = loaded.Registry().view<CompoundCollider2D>().get<CompoundCollider2D>(*loaded.Registry().view<CompoundCollider2D>().begin()).shapes;
    records.erase(records.begin());
    auto written = Json::parse(WriteScene(loaded, types));
    CHECK(written["entities"][0]["components"]["CompoundCollider2D"]["shapes"][0]["futureMaterial"]["coating"] == "second");
    CHECK(written["entities"][0]["components"]["CompoundCollider2D"]["shapes"][0]["v"] == 3);
    items[1]["radius"] = "bad number";
    World invalid;
    const auto report = ReadScene(invalid, types, document.dump());
    CHECK(report.ok && !report.warnings.empty());
    // The whole malformed list stays at its default; no partially decoded item leaks through.
    const auto& fallback = invalid.Registry().get<CompoundCollider2D>(*invalid.Registry().view<CompoundCollider2D>().begin());
    CHECK(fallback.shapes.size() == 1 && fallback.shapes[0].value.halfSize.x == 80);
    (void)entity;
}

TEST(CompoundAndSingleGeometryOwnershipAdmissionIsSymmetric) {
    World world;
    ComponentTypes types;
    DescribeRuntimeComponents(types);
    auto entity = world.Create();
    const auto& compound = types.All()[types.IndexOf("CompoundCollider2D")];
    const auto& collider = types.All()[types.IndexOf("Collider2D")];
    const auto& motor = types.All()[types.IndexOf("CharacterMotor2D")];
    entity.Add<Collider2D>();
    CHECK(!compound.AdditionError(world, entity.Id()).empty());
    entity.Remove<Collider2D>();
    entity.Add<CompoundCollider2D>();
    CHECK(!collider.AdditionError(world, entity.Id()).empty() && !motor.AdditionError(world, entity.Id()).empty());
    entity.Add<Collider2D>(); // invalid imported data is still diagnosable
    entity.Add<Transform2D>();
    InstallSceneMovement2D(world);
    SyncSceneMovement2D(world);
    CHECK(!SceneMovementErrors2D(world).empty());
    CHECK(SceneBody2D(world, entity.Id()).value == 0);
}

TEST(CompoundFilterAndRemovalPublishCoherentPerShapeSensorEvents) {
    World world;
    auto entity = Compound(world);
    world.Create().Add<PhysicsSettings2D>().gravity = {};
    auto sensor = world.Create();
    sensor.Add<Transform2D>();
    auto& collider = sensor.Add<Collider2D>();
    collider.halfSize = {80, 40};
    collider.sensor = true;
    InstallSceneMovement2D(world);
    world.Step(1.0 / 60, {});
    CHECK(ScenePhysicsEvents2D(world).size() == 2);
    entity.Get<CompoundCollider2D>().shapes[0].value.mask = 0;
    world.Step(1.0 / 60, {});
    int ended = 0, began = 0;
    for (const auto& event : ScenePhysicsEvents2D(world)) {
        CHECK(event.entityA == sensor.Id() && event.entityB == entity.Id());
        if (event.event.kind == PhysicsEventKind2D::End) {
            ++ended;
            CHECK(event.event.reason == PhysicsEndReason2D::FilterChanged);
        } else ++began;
    }
    CHECK(ended == 2 && began == 1);
    entity.Remove<CompoundCollider2D>();
    world.Step(1.0 / 60, {});
    CHECK(SceneBody2D(world, entity.Id()).value == 0);
    CHECK(ScenePhysicsEvents2D(world).size() == 1);
    CHECK(ScenePhysicsEvents2D(world)[0].event.reason == PhysicsEndReason2D::Removed);
    world.Step(1.0 / 60, {});
    CHECK(ScenePhysicsEvents2D(world).empty());
}

TEST(ReflectedRecordListsSupportTextAndPointFieldsWithoutPhysicsSpecificSerialization) {
    struct Item { std::string title; std::vector<pxPoint> points; };
    struct Collection { std::vector<ComponentRecord<Item>> items; };
    ComponentTypes types;
    types.Add<Item>("Item").Field<&Item::title>("title").Field<&Item::points>("points");
    types.Add<Collection>("Collection").Field<&Collection::items>("items");
    World source;
    auto entity = source.Create("collection");
    entity.Add<Collection>().items.push_back({{"authored item", {{1, 2}, {3, 4}}}, ""});
    AddToScene(entity);
    World loaded;
    CHECK(ReadScene(loaded, types, WriteScene(source, types)).ok);
    const auto& items = loaded.Registry().get<Collection>(*loaded.Registry().view<Collection>().begin()).items;
    CHECK(items.size() == 1 && items[0].value.title == "authored item");
    CHECK(items[0].value.points.size() == 2 && items[0].value.points[1] == pxPoint(3, 4));
}

TEST(StaticCompoundsDoNotIntroduceSolverWorkWithoutMotionSensorsOrPendingEvents) {
    World world;
    Compound(world, false);
    auto& telemetry = world.GetTelemetry();
    telemetry.SetEnabled(true);
    InstallSceneMovement2D(world);
    for (int i = 0; i < 60; ++i) world.Step(1.0 / 60, {});
    CHECK(telemetry.Timing(telemetry.RegisterTiming("physics.solve")).samples == 0);
}

TEST(CompoundGizmoCullingDoesNotHideVisibleShapesAfterAnOffscreenShape) {
    World world;
    auto entity = Compound(world, false);
    auto& shapes = entity.Get<CompoundCollider2D>().shapes;
    shapes[0].value.offset = {1000, 0};
    shapes[1].value.offset = {};
    Camera2D camera;
    camera.SetViewport({200, 200});
    ShapeRenderer renderer; // CPU submission only; no GL context is needed.
    DrawMovementGizmos2D(world, renderer, camera);
    CHECK(renderer.PendingCount() == 32); // only the visible circle's 32 outline edges
}
