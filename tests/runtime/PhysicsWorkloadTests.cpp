#include "Check.hpp"
#include "../benchmarks/PhysicsWorkload2D.hpp"
#include <algorithm>
#include <cmath>

TEST(PhysicsWorkloadsKeepDeclaredTopologyAndProduceTheAdvertisedPairs) {
    for (const auto* name : {"sparse", "compound", "stack", "sensors"}) {
        pixie::World world(12345);
        const auto setup = pixie::benchmark::BuildPhysicsWorkload2D(world, 12, name, true);
        world.GetTelemetry().SetEnabled(true);
        world.Step(1.0 / 60, {});
        const auto* physics = pixie::SceneCollisionWorld2D(world);
        CHECK(physics != nullptr);
        if (!physics) continue;
        const auto stats = physics->Stats();
        CHECK(stats.bodies == setup.expectedBodies && stats.shapes == setup.expectedShapes);
        CHECK(stats.sensors == (std::string_view(name) == "sensors" ? 12u : 0u));
        CHECK(stats.shapes == (std::string_view(name) == "compound" || std::string_view(name) == "sensors" ? 24u
                              : std::string_view(name) == "stack" ? 13u : 12u));
        std::vector<pixie::BodyHandle2D> handles;
        for (auto id : setup.dynamicBodies) handles.push_back(pixie::SceneBody2D(world, id));
        const auto rebuild = world.GetTelemetry().RegisterTiming("physics.scene.rebuild");
        const auto initialRebuilds = world.GetTelemetry().Timing(rebuild).samples;
        std::size_t contactPeak = 0, sensorPeak = 0, sensorBegins = 0, sensorEnds = 0;
        for (int tick = 0; tick < 360; ++tick) {
            world.Step(1.0 / 60, {});
            CHECK(pixie::SceneMovementErrors2D(world).empty());
            const auto now = physics->Stats();
            contactPeak = std::max(contactPeak, now.contactPairs);
            sensorPeak = std::max(sensorPeak, now.sensorPairs);
            CHECK(now.bodies == setup.expectedBodies && now.shapes == setup.expectedShapes);
            CHECK(world.GetTelemetry().Timing(rebuild).samples == initialRebuilds);
            for (const auto& event : pixie::ScenePhysicsEvents2D(world)) {
                if (event.event.pair != pixie::PhysicsPairKind2D::Sensor) continue;
                if (event.event.kind == pixie::PhysicsEventKind2D::Begin) ++sensorBegins;
                else ++sensorEnds;
            }
            for (std::size_t i = 0; i < handles.size(); ++i) {
                CHECK(pixie::SceneBody2D(world, setup.dynamicBodies[i]) == handles[i]);
                const auto motion = physics->BodyMotion(handles[i]);
                CHECK(std::isfinite(motion.position.x) && std::isfinite(motion.position.y) && std::isfinite(motion.rotation));
            }
        }
        if (std::string_view(name) == "stack") CHECK(contactPeak > 0);
        else CHECK(contactPeak == 0);
        if (std::string_view(name) == "sensors") {
            CHECK(sensorPeak == 12 && sensorBegins >= 12 && sensorEnds >= 12);
        } else CHECK(sensorPeak == 0 && sensorBegins == 0 && sensorEnds == 0);
    }
}

TEST(PhysicsWorkloadTelemetryRetainsTheSameTraceAndCommandsRunInsideTheTick) {
    for (const auto* name : {"sparse", "compound", "stack", "sensors"}) {
        for (bool awake : {false, true}) {
            pixie::World on(12345), off(12345);
            const auto first = pixie::benchmark::BuildPhysicsWorkload2D(on, 4, name, awake);
            const auto second = pixie::benchmark::BuildPhysicsWorkload2D(off, 4, name, awake);
            on.GetTelemetry().SetEnabled(true);
            const auto commands = on.GetTelemetry().RegisterTiming("benchmark.commands");
            for (int tick = 0; tick < 240; ++tick) {
                on.Step(1.0 / 60, {}); off.Step(1.0 / 60, {});
                for (std::size_t i = 0; i < first.dynamicBodies.size(); ++i) {
                    const auto a = pixie::SceneCollisionWorld2D(on)->BodyMotion(pixie::SceneBody2D(on, first.dynamicBodies[i]));
                    const auto b = pixie::SceneCollisionWorld2D(off)->BodyMotion(pixie::SceneBody2D(off, second.dynamicBodies[i]));
                    CHECK(a.position == b.position && a.rotation == b.rotation && a.velocity == b.velocity);
                }
            }
            const bool driven = awake && (std::string_view(name) == "sensors" || std::string_view(name) == "stack");
            CHECK(on.GetTelemetry().Timing(commands).samples == (driven ? 240u : 0u));
        }
    }
}
