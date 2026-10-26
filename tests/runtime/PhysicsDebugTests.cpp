#include "Check.hpp"
#include "pixie/runtime/PhysicsDebug2D.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/World.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
using namespace pixie;
namespace {
std::uint32_t Flag(PhysicsDebug2D flag) { return static_cast<std::uint32_t>(flag); }
}

TEST(PhysicsDebugUsesBackendShapePoseAndWorldCenterWithoutMutatingState) {
    CollisionWorld2D world;
    BodyShape2D shape;
    shape.halfSize = {4, 6}; shape.offset = {10, 0};
    const auto body = world.AddBody({100, 20}, std::numbers::pi / 2, {}, {&shape, 1});
    world.SetBodyVelocity(body, {8, -4}, 0);
    const auto before = world.BodyMotion(body);
    const auto generation = world.Generation();
    const auto outline = world.DebugGeometry(Flag(PhysicsDebug2D::Colliders));
    CHECK(outline.size() == 4);
    double left = 1e6, right = -1e6, top = 1e6, bottom = -1e6;
    for (const auto& line : outline) {
        CHECK(line.kind == PhysicsDebug2D::Colliders);
        left = std::min(left, line.a.x); right = std::max(right, line.a.x);
        top = std::min(top, line.a.y); bottom = std::max(bottom, line.a.y);
    }
    CHECK(std::abs(left - 94) < 1e-4 && std::abs(right - 106) < 1e-4);
    CHECK(std::abs(top - 26) < 1e-4 && std::abs(bottom - 34) < 1e-4);
    const auto centers = world.DebugGeometry(Flag(PhysicsDebug2D::Centers));
    CHECK(centers.size() == 2);
    for (const auto& line : centers) {
        CHECK(std::abs((line.a.x + line.b.x) / 2 - 100) < 1e-4);
        CHECK(std::abs((line.a.y + line.b.y) / 2 - 30) < 1e-4);
    }
    const auto velocities = world.DebugGeometry(Flag(PhysicsDebug2D::Velocities));
    CHECK(velocities.size() == 1);
    if (!velocities.empty()) {
        CHECK(std::abs(velocities[0].b.x - velocities[0].a.x - 2) < 1e-5);
        CHECK(std::abs(velocities[0].b.y - velocities[0].a.y + 1) < 1e-5);
    }
    CHECK(world.DebugGeometry(Flag(PhysicsDebug2D::Origins)).size() == 2);
    CHECK(world.DebugGeometry(0).empty());
    CHECK(world.BodyMotion(body).position == before.position && world.BodyMotion(body).velocity == before.velocity);
    CHECK(world.Generation() == generation && world.Events().empty());
    world.SetBodyEnabled(body, false);
    CHECK(world.DebugGeometry(kPhysicsDebugAll2D).empty());
    CHECK(outline.size() == 4); // Copied geometry outlives later backend edits.
}

TEST(PhysicsDebugSeparatesSensorsAndShowsSleepingState) {
    CollisionWorld2D world;
    std::array<BodyShape2D, 2> shapes;
    shapes[1].kind = BodyShapeKind2D::Circle; shapes[1].sensor = true;
    const auto body = world.AddBody({}, 0, {}, shapes);
    CHECK(world.DebugGeometry(Flag(PhysicsDebug2D::Colliders)).size() == 4);
    const auto sensors = world.DebugGeometry(Flag(PhysicsDebug2D::Sensors));
    CHECK(sensors.size() == 32);
    for (const auto& line : sensors) CHECK(line.kind == PhysicsDebug2D::Sensors);
    for (int i = 0; i < 120; ++i) world.StepBodies(1.0 / 60);
    CHECK(!world.BodyMotion(body).awake);
    auto markers = world.DebugGeometry(Flag(PhysicsDebug2D::Awake));
    CHECK(markers.size() == 4);
    for (const auto& line : markers) CHECK(line.color == pxColorRgba(130, 130, 150));
    world.SetBodyVelocity(body, {10, 0}, 0);
    markers = world.DebugGeometry(Flag(PhysicsDebug2D::Awake));
    for (const auto& line : markers) CHECK(line.color == pxColorRgba(80, 240, 120));
}

TEST(PhysicsDebugCopiesCurrentContactNormalsOncePerPair) {
    CollisionWorld2D world;
    world.SetGravity({0, 600});
    world.AddBox({0, 60}, {100, 10});
    BodyShape2D shape;
    const auto body = world.AddBody({}, 0, {}, {&shape, 1});
    for (int i = 0; i < 180; ++i) world.StepBodies(1.0 / 60);
    CHECK(world.BodyMotion(body).position.y < 40);
    const auto eventCount = world.Events().size();
    const auto normals = world.DebugGeometry(Flag(PhysicsDebug2D::Contacts));
    CHECK(normals.size() >= 1 && normals.size() <= 2);
    for (const auto& line : normals) {
        CHECK(line.kind == PhysicsDebug2D::Contacts);
        CHECK(std::abs(std::hypot(line.b.x - line.a.x, line.b.y - line.a.y) - 12) < 1e-4);
        CHECK(std::abs(line.a.y - 50) < 2);
    }
    CHECK(world.Events().size() == eventCount);
}

TEST(ScenePhysicsDebugReadsCharacterStateWithoutSteppingOrInventingMass) {
    World world;
    CHECK(ScenePhysicsDebug2D(world, 0).empty());
    CHECK(SceneCollisionWorld2D(world) == nullptr);
    auto actor = world.Create("debug character");
    actor.Add<Transform2D>(); actor.Add<CharacterMotor2D>();
    InstallSceneMovement2D(world);
    world.Step(1.0 / 60, {});
    const auto steps = world.Steps();
    const auto velocity = CharacterMotion2D(world, actor.Id())->velocity;
    const auto lines = ScenePhysicsDebug2D(world, Flag(PhysicsDebug2D::Velocities));
    CHECK(lines.size() == 1);
    if (!lines.empty()) CHECK(std::abs(lines[0].b.y - lines[0].a.y - velocity.y * 0.25) < 1e-6);
    CHECK(ScenePhysicsDebug2D(world, Flag(PhysicsDebug2D::Centers)).empty());
    const auto original = ScenePhysicsDebug2D(world, Flag(PhysicsDebug2D::Colliders));
    CHECK(original.size() == 34);
    actor.Get<CharacterMotor2D>().radius = 24;
    const auto unsynchronized = ScenePhysicsDebug2D(world, Flag(PhysicsDebug2D::Colliders));
    CHECK(unsynchronized.size() == original.size());
    if (!original.empty() && !unsynchronized.empty()) CHECK(original[0].a == unsynchronized[0].a);
    CHECK(world.Steps() == steps);
    actor.Get<CharacterMotor2D>().radius = -1;
    SyncSceneMovement2D(world);
    CHECK(ScenePhysicsDebug2D(world, kPhysicsDebugAll2D).empty());
}
