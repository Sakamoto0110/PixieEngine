#include "PhysicsWorkload2D.hpp"
#include <algorithm>
#include <random>
#include <string>
#include <stdexcept>

namespace pixie::benchmark {
namespace {
// Ten lanes, each a static floor with one kinematic platform carrying a rider, a pusher walking into
// a row of light/heavy/compound boxes and a stander loading a dynamic crate. Lanes cycle translating,
// rising-into-a-ceiling (crush, then recovery) and rotating platforms. Awake reverses the platforms
// every 120 ticks, commands the motors and, every 480 ticks, when each platform is back at its start,
// teleports the riders onto them again, so crushing and riding recur instead of happening once.
// Still leaves the platforms and motors idle.
PhysicsWorkload2D BuildCoupledWorkload2D(World& world, int count, bool awake) {
    constexpr int lanes = 10;
    PhysicsWorkload2D result;
    result.gravityY = 1100;
    result.expectedMotors = 3 * lanes;
    auto& settings = world.Create("physics settings").Add<PhysicsSettings2D>();
    settings.gravity = {0, result.gravityY};
    settings.unitsPerMeter = 32;
    settings.substeps = 4;
    std::size_t statics = 0, compounds = 0;
    std::vector<entt::entity> platforms, riders, pushers;
    auto motor = [&](const char* name, pxPoint at) {
        auto actor = world.Create(name);
        actor.Add<Transform2D>().position = at;
        actor.Add<CharacterMotor2D>();
        actor.Add<CharacterCommand2D>();
        result.motors.push_back(actor.Id());
        return actor.Id();
    };
    for (int lane = 0; lane < lanes; ++lane) {
        const double y = 400.0 * lane;
        auto floor = world.Create("coupled floor");
        floor.Add<Transform2D>().position = {1400, y + 16};
        floor.Add<Collider2D>().halfSize = {3000, 16};
        ++statics;
        auto platform = world.Create("coupled platform");
        platform.Add<Transform2D>().position = {-1000, y - 150};
        platform.Add<Collider2D>().halfSize = {60, 8};
        auto& body = platform.Add<RigidBody2D>();
        body.type = BodyType2D::Kinematic;
        if (awake && lane % 3 == 0) body.initialVelocity = {60, 0};
        else if (awake && lane % 3 == 1) body.initialVelocity = {0, -40};
        else if (awake) body.initialAngularVelocity = 0.3;
        platforms.push_back(platform.Id());
        if (lane % 3 == 1) {
            // The rider's top starts 50 units below this ceiling, so the rising platform crushes it.
            auto ceiling = world.Create("coupled ceiling");
            ceiling.Add<Transform2D>().position = {-1000, y - 264.5};
            ceiling.Add<Collider2D>().halfSize = {40, 8};
            ++statics;
        }
        riders.push_back(motor("coupled rider", {-1000, y - 182.5}));
        pushers.push_back(motor("coupled pusher", {100, y - 24.5}));
        motor("coupled stander", {-200, y - (lane < count ? 56.5 : 24.5)});
    }
    result.dynamicBodies.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        const int lane = i % lanes, slot = i / lanes;
        auto box = world.Create("coupled box");
        box.Add<Transform2D>().position = {slot == 0 ? -200.0 : 160.0 + 40.0 * slot, 400.0 * lane - 16};
        auto& body = box.Add<RigidBody2D>();
        if (slot != 0 && slot % 3 == 0) {
            Collider2D left, right;
            left.halfSize = right.halfSize = {8, 16};
            left.offset = {-8, 0}; right.offset = {8, 0};
            box.Add<CompoundCollider2D>().shapes = {{left, {}}, {right, {}}};
            ++compounds;
        } else {
            box.Add<Collider2D>().halfSize = {16, 16};
            body.massOverride = slot % 3 == 1 ? 1 : 10; // Crates (slot 0) and heavy boxes keep a 10:1 ratio.
        }
        result.dynamicBodies.push_back(box.Id());
    }
    result.expectedBodies = statics + platforms.size() + static_cast<std::size_t>(count);
    result.expectedShapes = result.expectedBodies + compounds;
    InstallSceneMovement2D(world);
    if (!awake) return result;
    result.driver = "coupled-cycle";
    const auto commandMetric = world.GetTelemetry().RegisterTiming("benchmark.commands");
    std::vector<pxPoint> starts;
    for (auto id : riders) starts.push_back(world.Registry().get<Transform2D>(id).position);
    world.AddSystem(SystemPhase::PreUpdate, "benchmark.physics.commands",
        [platforms, riders, pushers, starts, commandMetric, tick = std::uint64_t{0}](World& w, const FrameTime&, const Input&) mutable {
            const TelemetryScope measure(w.GetTelemetry(), commandMetric);
            const std::uint64_t now = tick++;
            const double sign = (now / 120) % 2 == 0 ? 1.0 : -1.0;
            if (now > 0 && now % 480 == 0)
                for (std::size_t i = 0; i < riders.size(); ++i) w.Registry().get<Transform2D>(riders[i]).position = starts[i];
            if (auto* physics = ScenePhysicsWorld2D(w)) { // Reacquire after the preceding scene sync.
                for (std::size_t lane = 0; lane < platforms.size(); ++lane) {
                    const auto handle = SceneBody2D(w, platforms[lane]);
                    if (lane % 3 == 0) physics->SetBodyVelocity(handle, {60 * sign, 0}, 0);
                    else if (lane % 3 == 1) physics->SetBodyVelocity(handle, {0, -40 * sign}, 0);
                    else physics->SetBodyVelocity(handle, {0, 0}, 0.3 * sign);
                }
            }
            for (auto id : riders) w.Registry().get<CharacterCommand2D>(id).jumpPressed = now % 150 == 75;
            for (auto id : pushers) w.Registry().get<CharacterCommand2D>(id).horizontal = (now / 240) % 2 == 0 ? 1 : -1;
        });
    return result;
}
}

bool IsPhysicsWorkload2D(std::string_view name) {
    return name == "sparse" || name == "compound" || name == "stack" || name == "sensors" || name == "coupled";
}

PhysicsWorkload2D BuildPhysicsWorkload2D(World& world, int count, std::string_view name, bool awake) {
    if (count < 0 || count > 500 || !IsPhysicsWorkload2D(name))
        throw std::invalid_argument("unknown physics workload or population outside 0..500");
    if (name == "coupled") return BuildCoupledWorkload2D(world, count, awake);
    PhysicsWorkload2D result;
    const bool stack = name == "stack", sensors = name == "sensors", compound = name == "compound";
    result.gravityY = stack ? 1100 : 0;
    result.expectedBodies = static_cast<std::size_t>(count) * (sensors ? 2 : 1) + (stack ? 1 : 0);
    result.expectedShapes = static_cast<std::size_t>(count) * (sensors || compound ? 2 : 1) + (stack ? 1 : 0);
    result.expectedSensors = sensors ? static_cast<std::size_t>(count) : 0;
    result.dynamicBodies.reserve(static_cast<std::size_t>(count));
    auto& settings = world.Create("physics settings").Add<PhysicsSettings2D>();
    settings.gravity = {0, result.gravityY};
    settings.unitsPerMeter = 32;
    settings.substeps = 4;
    if (stack) {
        auto floor = world.Create("qualification floor");
        floor.Add<Transform2D>().position = {0, 16};
        floor.Add<Collider2D>().halfSize = {8192, 16};
    }
    std::mt19937 random(12345);
    const int columns = std::max(1, std::min(count, 10));
    for (int i = 0; i < count; ++i) {
        const pxPoint origin = stack
            ? pxPoint{32.25 * (i % columns) - 16.125 * (columns - 1) + (random() % 4) * 0.01,
                      -16.0 - 32.25 * (i / columns)}
            : pxPoint{128.0 * (i % 25) + random() % 16, 128.0 * (i / 25)};
        auto body = world.Create("qualification body");
        body.Add<Transform2D>().position = {origin.x - (sensors && awake ? 40 : 0), origin.y};
        if (compound) {
            Collider2D left, right;
            left.halfSize = right.halfSize = {8, 16};
            left.offset = {-8, 0}; right.offset = {8, 0};
            body.Add<CompoundCollider2D>().shapes = {{left, {}}, {right, {}}};
        } else {
            auto& collider = body.Add<Collider2D>();
            collider.halfSize = {16, 16};
            if (sensors) { collider.category = 1; collider.mask = 2; }
        }
        body.Add<RigidBody2D>().initialVelocity = {awake ? (sensors ? 80.0 : 10.0) : 0.0, 0};
        result.dynamicBodies.push_back(body.Id());
        if (sensors) {
            auto sensor = world.Create("qualification sensor");
            sensor.Add<Transform2D>().position = origin;
            auto& collider = sensor.Add<Collider2D>();
            collider.halfSize = {16, 16};
            collider.sensor = true;
            collider.category = 2;
            collider.mask = 1;
        }
    }
    InstallSceneMovement2D(world);
    if (awake && (sensors || stack)) {
        result.driver = sensors ? "velocity-wave" : "zero-force-wake";
        const auto commandMetric = world.GetTelemetry().RegisterTiming("benchmark.commands");
        // The callback owns a setup-time copy of the IDs; it never borrows this function's locals.
        const auto ids = result.dynamicBodies;
        world.AddSystem(SystemPhase::PreUpdate, "benchmark.physics.commands",
            [ids, sensors, commandMetric, tick = std::uint64_t{0}](World& w, const FrameTime&, const Input&) mutable {
                const TelemetryScope measure(w.GetTelemetry(), commandMetric);
                auto* physics = ScenePhysicsWorld2D(w); // Reacquire after the preceding scene sync.
                const double speed = (tick++ / 60) % 2 == 0 ? 80.0 : -80.0;
                if (!physics) return;
                for (auto id : ids) {
                    const auto handle = SceneBody2D(w, id);
                    if (sensors) physics->SetBodyVelocity(handle, {speed, 0}, 0);
                    else physics->ApplyForce(handle, {0, 0}, physics->BodyMotion(handle).position);
                }
            });
    }
    return result;
}

bool OnlyCouplingStatus2D(World& world, const PhysicsWorkload2D& setup) {
    std::size_t reporting = 0;
    for (auto id : setup.motors) {
        const auto* coupling = CharacterCoupling2D(world, id);
        if (coupling && (coupling->unsafePlacement || coupling->blocked != CharacterBlocked2D::None)) ++reporting;
    }
    const auto errors = SceneMovementErrors2D(world);
    return errors.size() == reporting && std::ranges::all_of(errors, [](const std::string& error) {
        return error.ends_with("character support movement was blocked")
            || error.ends_with("could not establish a safe placement; last completed pose retained")
            || error.ends_with("exceeds maxCarryStep; last completed pose retained");
    });
}
}
