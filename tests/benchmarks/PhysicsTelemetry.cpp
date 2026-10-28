#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/World.hpp"
#include "PhysicsWorkload2D.hpp"
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::atomic<bool> countAllocations{false};
std::atomic<std::uint64_t> allocations{0};
}
// Counts ordinary C++ new/new[] calls in this executable. The shared engine has
// its own allocation call sites; neither those nor backend malloc/aligned calls
// are observed. Zero is not proof of zero engine or total heap activity.
void* operator new(std::size_t size) {
    if (countAllocations.load(std::memory_order_relaxed)) allocations.fetch_add(1, std::memory_order_relaxed);
    if (void* p = std::malloc(size == 0 ? 1 : size)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

namespace {
using Clock = std::chrono::steady_clock;
constexpr std::array<const char*, 8> names{
    "physics.scene.sync", "physics.scene.rebuild", "physics.solve", "physics.scene.motors",
    "physics.events.collect", "physics.scene.events.publish", "physics.scene.publish", "benchmark.commands"};
struct Sample {
    const char* phase;
    int index;
    double wall;
    std::array<double, names.size()> milliseconds;
    std::uint64_t newCalls, rebuildScopes;
    pixie::PhysicsStats2D population;
    std::array<std::size_t, 4> events{}; // contact begin/end, sensor begin/end
};
int Run(int argc, char** argv) {
    if (argc != 6 && argc != 7) {
        std::cerr << "usage: physics-benchmark <0..500 bodies> <awake|sleeping> <on|off> <warm steps> <output.csv> [sparse|compound|stack|sensors]\n";
        return 2;
    }
    const int count = std::stoi(argv[1]), steps = std::stoi(argv[4]);
    const std::string mode = argv[2], collection = argv[3];
    const std::string scenario = argc == 7 ? argv[6] : "sparse";
    if (count < 0 || count > 500 || steps < 100 || steps > 100000
        || (mode != "awake" && mode != "sleeping") || (collection != "on" && collection != "off")
        || !pixie::benchmark::IsPhysicsWorkload2D(scenario)) return 2;
    std::cerr << "Allocation scope: host executable new/new[] only; shared-engine and backend allocations are not observed.\n";
    pixie::World world(12345);
    const auto setup = pixie::benchmark::BuildPhysicsWorkload2D(world, count, scenario, mode == "awake");
    auto& telemetry = world.GetTelemetry();
    std::array<const pixie::TimingStats*, names.size()> timing{};
    for (std::size_t i = 0; i < names.size(); ++i) timing[i] = &telemetry.Timing(telemetry.RegisterTiming(names[i]));
    telemetry.SetEnabled(collection == "on");
    std::vector<Sample> samples;
    samples.reserve(static_cast<std::size_t>(steps) + 60);
    auto measure = [&](const char* phase, int index) {
        std::array<double, names.size()> previous{};
        for (std::size_t i = 0; i < names.size(); ++i) previous[i] = timing[i]->totalMs;
        const auto rebuilds = timing[1]->samples;
        allocations.store(0, std::memory_order_relaxed);
        countAllocations.store(true, std::memory_order_relaxed);
        const auto start = Clock::now();
        world.Step(1.0 / 60, {});
        const auto end = Clock::now();
        countAllocations.store(false, std::memory_order_relaxed);
        Sample sample{phase, index, std::chrono::duration<double, std::milli>(end - start).count(), {},
            allocations.load(std::memory_order_relaxed), timing[1]->samples - rebuilds, {}};
        for (std::size_t i = 0; i < names.size(); ++i) sample.milliseconds[i] = timing[i]->totalMs - previous[i];
        if (const auto* physics = pixie::SceneCollisionWorld2D(world)) sample.population = physics->Stats();
        for (const auto& event : pixie::ScenePhysicsEvents2D(world)) {
            const std::size_t pair = event.event.pair == pixie::PhysicsPairKind2D::Contact ? 0 : 2;
            ++sample.events[pair + (event.event.kind == pixie::PhysicsEventKind2D::Begin ? 0 : 1)];
        }
        if (sample.population.bodies != setup.expectedBodies || sample.population.shapes != setup.expectedShapes
            || sample.population.sensors != setup.expectedSensors || !pixie::SceneMovementErrors2D(world).empty())
            throw std::runtime_error("physics workload population/configuration mismatch");
        samples.push_back(sample);
    };
    for (int i = 0; i < 60; ++i) measure("cold", i);
    for (int i = 0; i < 300; ++i) world.Step(1.0 / 60, {});
    for (int i = 0; i < steps; ++i) measure("warm", i);
    if (!pixie::SceneMovementErrors2D(world).empty()) return 1;
    // Audit the retained final poses outside the measured tick; this is not a penetration oracle.
    const auto* physics = pixie::SceneCollisionWorld2D(world);
    for (auto id : setup.dynamicBodies) {
        const auto motion = physics->BodyMotion(pixie::SceneBody2D(world, id));
        if (!std::isfinite(motion.position.x) || !std::isfinite(motion.position.y) || !std::isfinite(motion.rotation)
            || !std::isfinite(motion.velocity.x) || !std::isfinite(motion.velocity.y) || !std::isfinite(motion.angularVelocity))
            throw std::runtime_error("physics workload ended with a non-finite motion");
    }
    std::ofstream output(argv[5]);
    if (!output) return 1;
    output << std::setprecision(17);
    output << "schema_version,scenario,driver,gravity_y,units_per_meter,configuration,seed,update_hz,substeps,render,requested_bodies,mode,telemetry,phase,index,wall_ms";
    for (const auto* name : names) output << ',' << name << "_ms";
    output << ",cpp_new_calls,rebuild_scopes,bodies,shapes,sensors,awake_bodies,contacts,sensor_pairs,contact_begin,contact_end,sensor_begin,sensor_end\n";
    for (const auto& sample : samples) {
        output << "2," << scenario << ',' << setup.driver << ',' << setup.gravityY << ",32,";
#ifdef NDEBUG
        output << "Release";
#else
        output << "Debug";
#endif
        output << ",12345,60,4,none," << count << ',' << mode << ',' << collection << ',' << sample.phase << ',' << sample.index << ',' << sample.wall;
        for (const auto value : sample.milliseconds) output << ',' << value;
        output << ',' << sample.newCalls << ',' << sample.rebuildScopes << ',' << sample.population.bodies << ',' << sample.population.shapes
            << ',' << sample.population.sensors << ',' << sample.population.awakeBodies << ',' << sample.population.contactPairs << ',' << sample.population.sensorPairs;
        for (auto events : sample.events) output << ',' << events;
        output << '\n';
    }
    std::ofstream metrics(std::string(argv[5]) + ".metrics.csv");
    telemetry.WriteCsv(metrics);
    std::cout << scenario << ' ' << count << ' ' << mode << ' ' << collection << ": " << samples.size() << " samples\n";
    return output && metrics ? 0 : 1;
}
}
int main(int argc, char** argv) {
    try { return Run(argc, argv); }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
