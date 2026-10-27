#include "PhysicsWorkload2D.hpp"
#include <algorithm>
#include <random>
#include <stdexcept>

namespace pixie::benchmark {
bool IsPhysicsWorkload2D(std::string_view name) {
    return name == "sparse" || name == "compound" || name == "stack" || name == "sensors";
}

PhysicsWorkload2D BuildPhysicsWorkload2D(World& world, int count, std::string_view name, bool awake) {
    if (count < 0 || count > 500 || !IsPhysicsWorkload2D(name))
        throw std::invalid_argument("unknown physics workload or population outside 0..500");
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
}
