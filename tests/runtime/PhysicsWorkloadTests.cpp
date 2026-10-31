#include "Check.hpp"
#include "../benchmarks/PhysicsWorkload2D.hpp"
#include "pixie/core/FixedStepper.hpp"
#include "pixie/runtime/PhysicsDebug2D.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

namespace {
// Body motions and motor positions after a fixed step, as raw doubles: equal traces are bitwise equal.
std::vector<double> Trace(pixie::World& world, const pixie::benchmark::PhysicsWorkload2D& setup) {
    std::vector<double> values;
    const auto* physics = pixie::SceneCollisionWorld2D(world);
    for (auto id : setup.dynamicBodies) {
        const auto motion = physics->BodyMotion(pixie::SceneBody2D(world, id));
        values.insert(values.end(), {motion.position.x, motion.position.y, motion.rotation, motion.velocity.x, motion.velocity.y});
    }
    for (auto id : setup.motors) {
        const auto& position = world.Registry().get<pixie::Transform2D>(id).position;
        values.insert(values.end(), {position.x, position.y});
    }
    return values;
}
}

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
    for (const auto* name : {"sparse", "compound", "stack", "sensors", "coupled"}) {
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
                CHECK(Trace(on, first) == Trace(off, second));
            }
            const bool driven = awake && std::string_view(name) != "sparse" && std::string_view(name) != "compound";
            CHECK(on.GetTelemetry().Timing(commands).samples == (driven ? 240u : 0u));
        }
    }
}

TEST(CoupledWorkloadMixesPlatformsRidersPushesLoadAndCrushWithoutLosingTopology) {
    pixie::World world(12345);
    const auto setup = pixie::benchmark::BuildPhysicsWorkload2D(world, 60, "coupled", true);
    world.GetTelemetry().SetEnabled(true);
    world.Step(1.0 / 60, {});
    const auto* physics = pixie::SceneCollisionWorld2D(world);
    CHECK(physics != nullptr);
    if (!physics) return;
    CHECK(setup.motors.size() == 30 && setup.expectedMotors == 30);
    const auto rebuild = world.GetTelemetry().RegisterTiming("physics.scene.rebuild");
    const auto prepare = world.GetTelemetry().RegisterTiming("physics.scene.coupling.prepare");
    const auto solve = world.GetTelemetry().RegisterTiming("physics.solve");
    const auto initialRebuilds = world.GetTelemetry().Timing(rebuild).samples;
    const auto initialPrepares = world.GetTelemetry().Timing(prepare).samples;
    const auto initialSolves = world.GetTelemetry().Timing(solve).samples;
    std::size_t contactPeak = 0, platformRiders = 0, unsafe = 0, recovered = 0, pushes = 0, loads = 0, jumps = 0;
    std::vector<bool> wasUnsafe(setup.motors.size());
    for (int tick = 0; tick < 600; ++tick) {
        world.Step(1.0 / 60, {});
        CHECK(pixie::benchmark::OnlyCouplingStatus2D(world, setup));
        const auto now = physics->Stats();
        contactPeak = std::max(contactPeak, now.contactPairs);
        CHECK(now.bodies == setup.expectedBodies && now.shapes == setup.expectedShapes && now.sensors == 0);
        CHECK(world.GetTelemetry().Timing(rebuild).samples == initialRebuilds);
        for (std::size_t i = 0; i < setup.motors.size(); ++i) {
            const auto* coupling = pixie::CharacterCoupling2D(world, setup.motors[i]);
            const auto* motion = pixie::CharacterMotion2D(world, setup.motors[i]);
            CHECK(coupling != nullptr && motion != nullptr);
            if (!coupling || !motion) continue;
            CHECK(std::isfinite(motion->position.x) && std::isfinite(motion->position.y));
            if (coupling->support.value && physics->BodyType(coupling->support) == pixie::BodyType2D::Kinematic) ++platformRiders;
            if (coupling->unsafePlacement) ++unsafe;
            else if (wasUnsafe[i]) ++recovered;
            wasUnsafe[i] = coupling->unsafePlacement;
            if (coupling->pushedImpulse > 0) ++pushes;
            if (coupling->supportLoad > 0) ++loads;
            if (motion->velocity.y < -300) ++jumps;
        }
        for (auto id : setup.dynamicBodies) {
            const auto motion = physics->BodyMotion(pixie::SceneBody2D(world, id));
            CHECK(std::isfinite(motion.position.x) && std::isfinite(motion.position.y) && std::isfinite(motion.rotation));
        }
    }
    // One coupling preparation and one backend solve per tick, and no reconstruction.
    CHECK(world.GetTelemetry().Timing(prepare).samples == initialPrepares + 600);
    CHECK(world.GetTelemetry().Timing(solve).samples == initialSolves + 600);
    CHECK(contactPeak > 0 && platformRiders > 0 && pushes > 0 && loads > 0 && jumps > 0);
    CHECK(unsafe > 0 && recovered > 0); // Rising lanes crush their riders, and the reversal releases them.
}

TEST(FixedStepTraceIsIndependentOfTheRenderRate) {
    // The same 240 fixed steps reached through 30, 60 and 144 render frames per second; every frame
    // also reads interpolated poses and debug geometry, as a renderer would.
    std::vector<std::vector<double>> traces;
    for (const double fps : {30.0, 60.0, 144.0}) {
        pixie::World world(12345);
        const auto setup = pixie::benchmark::BuildPhysicsWorkload2D(world, 20, "coupled", true);
        pixie::FixedStepper stepper(60);
        std::vector<double> trace;
        std::uint64_t frame = 0;
        while (stepper.Steps() < 240) {
            stepper.Advance(1.0 / fps, [&](const pixie::FrameTime& time) {
                world.Step(time.delta, {});
                const auto values = Trace(world, setup);
                trace.insert(trace.end(), values.begin(), values.end());
            });
            const double alpha = stepper.RenderTime(1.0 / fps, frame++).alpha;
            CHECK(alpha >= 0 && alpha < 1);
            for (auto [id, current, previous] : world.Registry().view<pixie::Transform2D, pixie::PreviousTransform2D>().each()) {
                const auto drawn = pixie::Interpolate(previous.value, current, alpha);
                CHECK(std::isfinite(drawn.position.x) && std::isfinite(drawn.position.y) && std::isfinite(drawn.rotation));
                // Shortest-path rotation: never more than half a turn between two completed steps.
                CHECK(std::abs(drawn.rotation - previous.value.rotation) <= std::numbers::pi * alpha + 1e-9);
            }
            if (stepper.Steps() > 0) CHECK(!pixie::ScenePhysicsDebug2D(world, 127).empty()); // Nothing is synchronized before the first step.
        }
        trace.resize(std::min(trace.size(), Trace(world, setup).size() * 240)); // 144 Hz may finish past step 240.
        traces.push_back(std::move(trace));
    }
    CHECK(!traces[0].empty() && traces[0] == traces[1] && traces[1] == traces[2]);
    // Sensitivity: stepping by the frame delta itself makes the result depend on the render rate.
    std::vector<std::vector<double>> variable;
    for (const double fps : {30.0, 144.0}) {
        pixie::World world(12345);
        const auto setup = pixie::benchmark::BuildPhysicsWorkload2D(world, 20, "coupled", true);
        for (int i = 0; i < static_cast<int>(fps) * 4; ++i) world.Step(1.0 / fps, {});
        variable.push_back(Trace(world, setup));
    }
    CHECK(variable[0] != variable[1]);
}
