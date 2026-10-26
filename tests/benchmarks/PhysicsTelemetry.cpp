#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/World.hpp"
#include <array>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <new>
#include <random>
#include <string>
#include <vector>

namespace {
std::atomic<bool> countAllocations{false};
std::atomic<std::uint64_t> allocations{0};
}
// Counts ordinary C++ new/new[] calls in this executable. Backend malloc/aligned
// allocations are not covered, so zero is not proof of zero total heap activity.
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
constexpr std::array<const char*, 7> names{
    "physics.scene.sync", "physics.scene.rebuild", "physics.solve", "physics.scene.motors",
    "physics.events.collect", "physics.scene.events.publish", "physics.scene.publish"};
struct Sample {
    const char* phase;
    int index;
    double wall;
    std::array<double, names.size()> milliseconds;
    std::uint64_t newCalls, rebuildScopes;
    pixie::PhysicsStats2D population;
};
int Run(int argc, char** argv) {
    if (argc != 6) {
        std::cerr << "usage: physics-benchmark <0..500 bodies> <awake|sleeping> <on|off> <warm steps> <output.csv>\n";
        return 2;
    }
    const int count = std::stoi(argv[1]), steps = std::stoi(argv[4]);
    const std::string mode = argv[2], collection = argv[3];
    if (count < 0 || count > 500 || steps < 100 || steps > 100000
        || (mode != "awake" && mode != "sleeping") || (collection != "on" && collection != "off")) return 2;
    pixie::World world(12345);
    auto settings = world.Create("physics settings");
    settings.Add<pixie::PhysicsSettings2D>().gravity = {0, 0};
    std::mt19937 random(12345);
    for (int i = 0; i < count; ++i) {
        auto body = world.Create("benchmark body");
        body.Add<pixie::Transform2D>().position = {128.0 * (i % 25) + random() % 16, 128.0 * (i / 25)};
        body.Add<pixie::Collider2D>().halfSize = {16, 16};
        body.Add<pixie::RigidBody2D>().initialVelocity = mode == "awake" ? pxVec2{10, 0} : pxVec2{0, 0};
    }
    pixie::InstallSceneMovement2D(world);
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
        samples.push_back(sample);
    };
    for (int i = 0; i < 60; ++i) measure("cold", i);
    for (int i = 0; i < 300; ++i) world.Step(1.0 / 60, {});
    for (int i = 0; i < steps; ++i) measure("warm", i);
    if (!pixie::SceneMovementErrors2D(world).empty()) return 1;
    std::ofstream output(argv[5]);
    if (!output) return 1;
    output << std::setprecision(17);
    output << "configuration,seed,update_hz,substeps,render,requested_bodies,mode,telemetry,phase,index,wall_ms";
    for (const auto* name : names) output << ',' << name << "_ms";
    output << ",cpp_new_calls,rebuild_scopes,bodies,shapes,awake_bodies,contacts,sensor_pairs\n";
    for (const auto& sample : samples) {
#ifdef NDEBUG
        output << "Release";
#else
        output << "Debug";
#endif
        output << ",12345,60,4,none," << count << ',' << mode << ',' << collection << ',' << sample.phase << ',' << sample.index << ',' << sample.wall;
        for (const auto value : sample.milliseconds) output << ',' << value;
        output << ',' << sample.newCalls << ',' << sample.rebuildScopes << ',' << sample.population.bodies << ',' << sample.population.shapes
            << ',' << sample.population.awakeBodies << ',' << sample.population.contactPairs << ',' << sample.population.sensorPairs << '\n';
    }
    std::ofstream metrics(std::string(argv[5]) + ".metrics.csv");
    telemetry.WriteCsv(metrics);
    std::cout << count << ' ' << mode << ' ' << collection << ": " << samples.size() << " samples\n";
    return output && metrics ? 0 : 1;
}
}
int main(int argc, char** argv) {
    try { return Run(argc, argv); }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
