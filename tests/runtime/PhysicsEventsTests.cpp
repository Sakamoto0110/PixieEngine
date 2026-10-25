#include "Check.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/World.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include <array>
#include <cstdlib>
#include <cmath>
#include <stdexcept>
using namespace pixie;
namespace {
BodyHandle2D Body(CollisionWorld2D& world, pxPoint at = {}, BodyType2D type = BodyType2D::Dynamic,
                  bool sensor = false, pxVec2 velocity = {0, 0}) {
    BodyShape2D shape;
    shape.sensor = sensor;
    RigidBody2D body;
    body.type = type; body.initialVelocity = velocity;
    return world.AddBody(at, 0, body, {&shape, 1});
}
BodyHandle2D Sensor(CollisionWorld2D& world) { return Body(world, {}, BodyType2D::Static, true); }
void Tick(CollisionWorld2D& world) { world.StepBodies(1.0 / 60); }
Entity SceneShape(World& world, bool sensor = false) {
    auto e = world.Create(sensor ? "sensor" : "visitor");
    e.Add<Transform2D>();
    e.Add<Collider2D>().sensor = sensor;
    AddToScene(e);
    return e;
}
bool OmitSensorFixture() {
#ifdef _MSC_VER
    char* value = nullptr;
    std::size_t count = 0;
    if (_dupenv_s(&value, &count, "PIXIE_TEST_OMIT_SENSOR") != 0) return false;
    const bool present = value != nullptr;
    std::free(value);
    return present;
#else
    return std::getenv("PIXIE_TEST_OMIT_SENSOR") != nullptr;
#endif
}
}
TEST(SensorMassIsExcludedFromCompoundDistribution) {
    CollisionWorld2D world;
    std::array<BodyShape2D, 2> shapes;
    shapes[1].sensor = true; shapes[1].density = 100; shapes[1].offset = {100, 0};
    auto body = world.AddBody({}, 0, {}, shapes);
    CHECK(std::abs(world.BodyMass(body).mass - 1) < 0.0001);
    CHECK(std::abs(world.BodyMass(body).localCenter.x) < 0.0001);
    bool rejected = false;
    try { Body(world, {}, BodyType2D::Dynamic, true); } catch (const std::invalid_argument&) { rejected = true; }
    CHECK(rejected);
    CHECK(world.Stats().sensors == 1 && world.Stats().shapes == 2);
}
TEST(SensorNonblockingFixtureDetectsAuthoredSensorOmission) {
    CollisionWorld2D world;
    // Opt-in test-only fault makes this sensor a blocking wall; the same assertions must fail.
    Body(world, {}, BodyType2D::Static, !OmitSensorFixture());
    auto visitor = Body(world, {-80, 0}, BodyType2D::Dynamic, false, {160, 0});
    int begins = 0, ends = 0;
    const auto generation = world.Generation();
    for (int i = 0; i < 60; ++i) {
        Tick(world);
        for (const auto& event : world.Events()) {
            CHECK(event.generation == generation && event.step == static_cast<std::uint64_t>(i + 1));
            if (event.pair != PhysicsPairKind2D::Sensor) continue;
            CHECK(event.a.sensor && !event.b.sensor);
            if (event.kind == PhysicsEventKind2D::Begin) ++begins; else ++ends;
        }
    }
    CHECK(begins == 1 && ends == 1);
    CHECK(world.BodyMotion(visitor).position.x > 79);
    CHECK(world.Stats().sensorPairs == 0);
}
TEST(SensorStaticVisitorsAndDirectionalSensorPairsHaveDefinedLifetime) {
    CollisionWorld2D world;
    Sensor(world);
    Body(world, {}, BodyType2D::Static);
    Tick(world);
    CHECK(world.Events().size() == 1);
    CHECK(world.Stats().sensorPairs == 1);
    Tick(world);
    CHECK(world.Events().empty());
    CollisionWorld2D twoSensors;
    Sensor(twoSensors); Sensor(twoSensors);
    Tick(twoSensors);
    CHECK(twoSensors.Events().size() == 2);
    CHECK(twoSensors.Stats().sensorPairs == 2);
}
TEST(SensorRemovalPublishesOneEndWithoutInvalidatingLastSnapshot) {
    CollisionWorld2D world;
    Sensor(world);
    auto visitor = Body(world);
    Tick(world);
    CHECK(world.Events().size() == 1);
    if (world.Events().empty()) return;
    const auto snapshot = world.Events();
    const auto oldShape = world.BodyColliders(visitor).front();
    CHECK(world.RemoveBody(visitor));
    CHECK(snapshot.front().kind == PhysicsEventKind2D::Begin);
    CHECK(!world.Contains(oldShape));
    Tick(world);
    CHECK(world.Events().size() == 1);
    if (!world.Events().empty()) {
        CHECK(world.Events().front().kind == PhysicsEventKind2D::End);
        CHECK(world.Events().front().reason == PhysicsEndReason2D::Removed);
        CHECK(world.Events().front().b.collider == oldShape);
    }
    Tick(world);
    CHECK(world.Events().empty());
}
TEST(SensorDisableEnableAndFilterChangeClosePairsWithoutDuplicateEnds) {
    CollisionWorld2D world;
    Sensor(world);
    auto visitor = Body(world);
    auto shape = world.BodyColliders(visitor).front();
    Tick(world);
    world.SetBodyEnabled(visitor, false);
    Tick(world);
    CHECK(world.Events().size() == 1);
    if (!world.Events().empty()) CHECK(world.Events().front().reason == PhysicsEndReason2D::Disabled);
    Tick(world); CHECK(world.Events().empty());
    world.SetBodyEnabled(visitor, true);
    Tick(world);
    CHECK(world.Events().size() == 1 && world.Stats().sensorPairs == 1);
    world.SetCollisionFilter(shape, {1, 0});
    Tick(world);
    CHECK(world.Events().size() == 1 && world.Stats().sensorPairs == 0);
    if (!world.Events().empty()) CHECK(world.Events().front().reason == PhysicsEndReason2D::FilterChanged);
    Tick(world); CHECK(world.Events().empty());
    world.SetCollisionFilter(shape, {1, ~std::uint64_t{0}});
    Tick(world);
    CHECK(world.Events().size() == 1 && world.Stats().sensorPairs == 1);
}
TEST(SensorTeleportAndResetUseOldIdentitiesAndFreshGeneration) {
    CollisionWorld2D world;
    Sensor(world);
    auto visitor = Body(world);
    Tick(world);
    world.TeleportBody(visitor, {100, 0}, 0);
    Tick(world);
    CHECK(world.Events().size() == 1);
    if (!world.Events().empty()) CHECK(world.Events().front().reason == PhysicsEndReason2D::Teleported);
    world.TeleportBody(visitor, {}, 0);
    Tick(world);
    CHECK(world.Stats().sensorPairs == 1);
    const auto generation = world.Generation();
    world.Reset();
    CHECK(world.Generation() != generation && world.Stats().bodies == 0);
    CHECK(world.Events().size() == 1);
    if (!world.Events().empty()) {
        CHECK(world.Events().front().reason == PhysicsEndReason2D::Reset);
        CHECK(world.Events().front().generation == generation);
    }
    Sensor(world); auto replacement = Body(world);
    CHECK(replacement != visitor);
    Tick(world);
    CHECK(world.Events().size() == 1);
    if (!world.Events().empty()) CHECK(world.Events().front().generation == world.Generation());
}
TEST(ContactBeginAndNaturalSeparationAreNotSensorEvents) {
    CollisionWorld2D world;
    world.AddBox({0, 25}, {100, 10});
    auto body = Body(world);
    int begins = 0, ends = 0;
    for (int i = 0; i < 30; ++i) {
        Tick(world);
        for (auto event : world.Events()) {
            CHECK(event.pair == PhysicsPairKind2D::Contact);
            if (event.kind == PhysicsEventKind2D::Begin) ++begins;
        }
    }
    CHECK(begins == 1 && world.Stats().contactPairs == 1);
    world.SetBodyVelocity(body, {0, -200}, 0);
    for (int i = 0; i < 10; ++i) {
        Tick(world);
        for (auto event : world.Events()) if (event.kind == PhysicsEventKind2D::End) {
            CHECK(event.reason == PhysicsEndReason2D::Separated); ++ends;
        }
    }
    CHECK(ends == 1 && world.Stats().contactPairs == 0);
}
TEST(CompoundVisitorsProduceShapePairsRatherThanEntityAggregation) {
    CollisionWorld2D world;
    BodyShape2D trigger;
    trigger.sensor = true; trigger.halfSize = {100, 100};
    RigidBody2D fixed; fixed.type = BodyType2D::Static;
    world.AddBody({}, 0, fixed, {&trigger, 1});
    std::array<BodyShape2D, 2> shapes;
    shapes[0].offset = {-25, 0}; shapes[1].offset = {25, 0};
    world.AddBody({}, 0, {}, shapes);
    Tick(world);
    CHECK(world.Events().size() == 2 && world.Stats().sensorPairs == 2);
    if (world.Events().size() == 2) {
        CHECK(world.Events()[0].b.body == world.Events()[1].b.body);
        CHECK(world.Events()[0].b.collider != world.Events()[1].b.collider);
    }
}
TEST(SensorQueryInclusionAndBothFilterDirectionsAreExplicit) {
    CollisionWorld2D world;
    auto sensor = Sensor(world);
    auto solid = world.AddBox({0, 60}, {16, 16});
    auto excluded = world.RayCast({0, -100}, {0, 200});
    auto included = world.RayCast({0, -100}, {0, 200}, {}, true);
    CHECK(excluded && excluded->collider == solid && !excluded->sensor);
    CHECK(included && included->sensor && included->collider == world.BodyColliders(sensor).front());
    CHECK(world.OverlapBox({}, {2, 2}).empty());
    CHECK(world.OverlapBox({}, {2, 2}, {}, true).size() == 1);
    world.SetCollisionFilter(world.BodyColliders(sensor).front(), {2, 4});
    CHECK(world.OverlapBox({}, {2, 2}, {4, 2}, true).size() == 1);
    CHECK(world.OverlapBox({}, {2, 2}, {8, 2}, true).empty());
    CHECK(world.OverlapBox({}, {2, 2}, {4, 8}, true).empty());
}
TEST(SensorsNeverBlockCapsuleQueriesOrChangeSolidGrounding) {
    CollisionWorld2D baseline, sensors;
    baseline.AddBox({0, 110}, {500, 10}); sensors.AddBox({0, 110}, {500, 10});
    BodyShape2D shape; shape.sensor = true; shape.halfSize = {10, 100};
    RigidBody2D fixed; fixed.type = BodyType2D::Static;
    sensors.AddBody({40, 50}, 0, fixed, {&shape, 1});
    CharacterController2D a, b;
    for (int i = 0; i < 90; ++i) {
        a.Step(baseline, 1, false, 1.0 / 60); b.Step(sensors, 1, false, 1.0 / 60);
        CHECK(std::abs(a.State().position.x - b.State().position.x) < 0.001);
        CHECK(std::abs(a.State().position.y - b.State().position.y) < 0.001);
        CHECK(a.State().grounded == b.State().grounded);
    }
}
TEST(SceneStaticSensorEventsSurviveDisableAndThenClearOnTheNextTick) {
    World world;
    auto sensor = SceneShape(world, true), visitor = SceneShape(world);
    InstallSceneMovement2D(world);
    world.Step(1.0 / 60, {});
    CHECK(ScenePhysicsEvents2D(world).size() == 1);
    if (!ScenePhysicsEvents2D(world).empty()) {
        CHECK(ScenePhysicsEvents2D(world).front().entityA == sensor.Id());
        CHECK(ScenePhysicsEvents2D(world).front().entityB == visitor.Id());
    }
    sensor.Get<Collider2D>().enabled = false;
    world.Step(1.0 / 60, {});
    CHECK(ScenePhysicsEvents2D(world).size() == 1);
    if (!ScenePhysicsEvents2D(world).empty()) CHECK(ScenePhysicsEvents2D(world).front().event.reason == PhysicsEndReason2D::Disabled);
    world.Step(1.0 / 60, {});
    CHECK(ScenePhysicsEvents2D(world).empty());
}
TEST(SceneRemovedEntityIdentityIsNotReplacedByAReusedSlot) {
    World world;
    SceneShape(world, true); auto visitor = SceneShape(world);
    InstallSceneMovement2D(world);
    world.Step(1.0 / 60, {});
    const auto old = visitor.Id(); visitor.Destroy();
    auto replacement = world.Create("replacement");
    CHECK(old != replacement.Id());
    world.Step(1.0 / 60, {});
    CHECK(ScenePhysicsEvents2D(world).size() == 1);
    if (!ScenePhysicsEvents2D(world).empty()) {
        CHECK(ScenePhysicsEvents2D(world).front().entityB == old);
        CHECK(ScenePhysicsEvents2D(world).front().event.reason == PhysicsEndReason2D::Removed);
    }
}
TEST(SceneFilterAndGeometryEditsPublishOldEndsBeforeNewBegins) {
    World world;
    auto sensor = SceneShape(world, true); SceneShape(world);
    InstallSceneMovement2D(world);
    world.Step(1.0 / 60, {});
    sensor.Get<Collider2D>().mask = 0;
    world.Step(1.0 / 60, {});
    CHECK(ScenePhysicsEvents2D(world).size() == 1);
    if (!ScenePhysicsEvents2D(world).empty()) CHECK(ScenePhysicsEvents2D(world).front().event.reason == PhysicsEndReason2D::FilterChanged);
    sensor.Get<Collider2D>().mask = 1;
    world.Step(1.0 / 60, {});
    CHECK(ScenePhysicsEvents2D(world).size() == 1);
    sensor.Get<Collider2D>().halfSize.x += 1;
    world.Step(1.0 / 60, {});
    CHECK(ScenePhysicsEvents2D(world).size() == 2);
    if (ScenePhysicsEvents2D(world).size() == 2) {
        const auto& old = ScenePhysicsEvents2D(world)[0];
        const auto& next = ScenePhysicsEvents2D(world)[1];
        CHECK(old.entityA == sensor.Id() && next.entityA == sensor.Id());
        CHECK(old.event.kind == PhysicsEventKind2D::End && old.event.reason == PhysicsEndReason2D::Reconfigured);
        CHECK(next.event.kind == PhysicsEventKind2D::Begin);
        CHECK(old.event.a.collider != next.event.a.collider);
    }
}
TEST(SceneInvalidSettingsResetClosesActivePairsWithEntityIdentity) {
    World world;
    auto sensor = SceneShape(world, true); SceneShape(world);
    InstallSceneMovement2D(world);
    world.Step(1.0 / 60, {});
    const auto generation = ScenePhysicsEvents2D(world).empty() ? 0 : ScenePhysicsEvents2D(world).front().event.generation;
    world.Create("settings 1").Add<PhysicsSettings2D>();
    world.Create("settings 2").Add<PhysicsSettings2D>();
    world.Step(1.0 / 60, {});
    CHECK(ScenePhysicsEvents2D(world).size() == 1);
    if (!ScenePhysicsEvents2D(world).empty()) {
        CHECK(ScenePhysicsEvents2D(world).front().entityA == sensor.Id());
        CHECK(ScenePhysicsEvents2D(world).front().event.reason == PhysicsEndReason2D::Reset);
        CHECK(ScenePhysicsEvents2D(world).front().event.generation == generation);
    }
    world.Step(1.0 / 60, {}); CHECK(ScenePhysicsEvents2D(world).empty());
}
TEST(SceneSensorRoundTripsWithoutSavingActivePairs) {
    World source;
    SceneShape(source, true); SceneShape(source);
    InstallSceneMovement2D(source); source.Step(1.0 / 60, {});
    ComponentTypes types; DescribeRuntimeComponents(types);
    const auto text = WriteScene(source, types);
    CHECK(text.find("sensor") != std::string::npos && text.find("generation") == std::string::npos);
    World loaded;
    CHECK(ReadScene(loaded, types, text).ok);
    CHECK(ScenePhysicsEvents2D(loaded).empty());
    InstallSceneMovement2D(loaded); loaded.Step(1.0 / 60, {});
    CHECK(ScenePhysicsEvents2D(loaded).size() == 1);
}
