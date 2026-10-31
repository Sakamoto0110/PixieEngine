// Lighting budgets (PIXIE-TASK-217): named stress rooms drawn through a ProjectSession, as the editor
// and the player draw them, with the lighting telemetry's CPU and GPU timings recorded per frame, the
// first (cold) frame apart from the warm ones. An explicit measurement tool, not a CTest test.
//
//   lighting-budget --out <folder> [--size <w>x<h>] [--frames <n>] [--label <name>]
//
// It writes budget.csv (one row per room: counts, and p50/p95/p99/max of each timing) to the folder.

#include "pixie/render/gl/GlLoader.hpp"
#include "pixie/runtime/Lighting2D.hpp"
#include "pixie/runtime/ParticleSystem.hpp"
#include "pixie/runtime/ProjectSession.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/ScreenCanvas.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

// A room with a panel open on the screen canvas, as the consumer's rooms have.
class BudgetProject : public pixie::Project {
public:
    std::string_view Name() const override { return "lighting-budget"; }
    pxColorRgba Background() const override { return pxColorRgba(120, 120, 128); }
    void Build(pixie::World& /*world*/, pixie::Camera2D& /*camera*/) override {}
    void DrawScreen(pixie::World& /*world*/, pixie::ScreenCanvas& screen) override {
        screen.Rect(pxPoint(24.0, 24.0), pxVec2(360.0, 220.0), pxColorRgba(20, 24, 32, 230));
        screen.Rect(pxPoint(40.0, 40.0), pxVec2(328.0, 24.0), pxColorRgba(90, 160, 220));
    }
};

struct Room {
    std::string name;
    int lights = 0;
    int boxes = 0;
    int particles = 0;
    pixie::LightQuality2D quality = pixie::LightQuality2D::Medium;
};

void Build(pixie::World& world, const Room& room, double width, double height) {
    std::uint32_t state = 7u;
    auto next = [&state]() {
        state = state * 1664525u + 1013904223u;
        return static_cast<double>(state >> 8) / 16777216.0;
    };
    const auto at = [&](pxPoint position, double rotation) {
        pixie::Entity entity = world.Create();
        entity.Add<pixie::Transform2D>(pixie::Transform2D{position, rotation, pxVec2(1.0, 1.0)});
        world.ResetInterpolation(entity);
        return entity;
    };
    pixie::LightingSettings2D settings;
    settings.ambient = pxColorRgba(70, 80, 105);
    settings.ambientIntensity = 0.3;
    settings.toneMapping = pixie::LightToneMapping2D::Aces;
    settings.quality = room.quality;
    at(pxPoint(0.0, 0.0), 0.0).Add<pixie::LightingSettings2D>(settings);
    for (int i = 0; i < room.boxes; ++i) {
        const pxVec2 half(8.0 + 36.0 * next(), 8.0 + 36.0 * next());
        pixie::Entity box = at(pxPoint((next() - 0.5) * 2.0 * width, (next() - 0.5) * 2.0 * height), 6.28 * next());
        box.Add<pixie::Shape>(pixie::Shape{pixie::ShapeKind::Quad, half, pxColorRgba(90, 96, 110), true});
        pixie::Collider2D collider;
        collider.halfSize = half;
        box.Add<pixie::Collider2D>(collider);
        box.Add<pixie::Occluder2D>(pixie::Occluder2D{});
    }
    for (int i = 0; i < room.lights; ++i) {
        pixie::Light2D light;
        light.range = room.lights > 16 ? 220.0 : 300.0;
        light.sourceRadius = 10.0;
        light.color = pxColorRgba(static_cast<std::uint8_t>(150 + 100 * next()), static_cast<std::uint8_t>(150 + 100 * next()), 220);
        at(pxPoint((next() - 0.5) * width, (next() - 0.5) * height), 0.0).Add<pixie::Light2D>(light);
    }
    if (room.particles > 0) {
        pixie::Entity pool = world.Create();
        pixie::ParticleSystem system;
        system.capacity = room.particles;
        system.style.color = pxColorRgba(255, 190, 90);
        system.style.size = 1.6f;
        for (int i = 0; i < room.particles; ++i) {
            system.Emit(pxPointf(static_cast<float>((next() - 0.5) * width), static_cast<float>((next() - 0.5) * height)),
                        pxVec2f(0.0f, 0.0f), 0.0f);
        }
        pool.Add<pixie::ParticleSystem>(std::move(system));
        pixie::ParticleLight2D light;
        light.emissive = true;
        light.illuminates = true;
        light.lightRadius = 40.0;
        light.lightIntensity = 0.4;
        light.lightCap = room.particles;
        pool.Add<pixie::ParticleLight2D>(light);
    }
}

struct Summary {
    double p50 = 0.0, p95 = 0.0, p99 = 0.0, max = 0.0;
};

Summary Summarize(std::vector<double> values) {
    Summary s;
    if (values.empty()) return s;
    std::sort(values.begin(), values.end());
    const auto at = [&](double q) { return values[std::min(values.size() - 1, static_cast<std::size_t>(q * static_cast<double>(values.size() - 1) + 0.5))]; };
    s = {at(0.5), at(0.95), at(0.99), values.back()};
    return s;
}

int Run(int argc, char** argv) {
    std::filesystem::path out = "lighting-budget";
    int width = 1920, height = 1080, frames = 240;
    std::string label = "unnamed";
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--out" && i + 1 < argc) out = argv[++i];
        else if (arg == "--size" && i + 1 < argc) {
            const std::string size = argv[++i];
            const auto x = size.find('x');
            width = std::stoi(size.substr(0, x));
            height = std::stoi(size.substr(x + 1));
        } else if (arg == "--frames" && i + 1 < argc) frames = std::stoi(argv[++i]);
        else if (arg == "--label" && i + 1 < argc) label = argv[++i];
        else {
            std::cerr << "usage: lighting-budget --out <folder> [--size <w>x<h>] [--frames <n>] [--label <name>]\n";
            return 2;
        }
    }
    std::filesystem::create_directories(out);
    if (glfwInit() == GLFW_FALSE) return 1;
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "lighting-budget", nullptr, nullptr);
    if (window == nullptr) {
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    int status = 1;
    if (pixie::LoadGl()) {
        const std::vector<Room> rooms{
            {"dark-room-0-lights", 0, 64, 0, pixie::LightQuality2D::Medium},
            {"lights-16", 16, 128, 0, pixie::LightQuality2D::Medium},
            {"lights-64-high", 64, 128, 0, pixie::LightQuality2D::High},
            {"lights-64-medium-cap", 64, 128, 0, pixie::LightQuality2D::Medium},
            {"lights-16-burst-2000", 16, 128, 2000, pixie::LightQuality2D::Medium},
        };
        std::ofstream csv(out / "budget.csv");
        csv << "label,renderer,width,height,room,lights,boxes,particles,frames,cold_wall_ms";
        for (const char* stage : {"wall", "lighting_cpu", "gpu_lights", "gpu_shadows", "gpu_combine"}) {
            csv << ',' << stage << "_p50," << stage << "_p95," << stage << "_p99," << stage << "_max";
        }
        csv << ",lights_drawn,shadowed_drawn,shadowed_dropped,occluder_edges,particle_lights,particle_lights_dropped\n";
        const std::string renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
        for (const Room& room : rooms) {
            pixie::ProjectSession session(std::make_unique<BudgetProject>(), pixie::SessionSettings{});
            if (!session.Start({width, height})) break;
            Build(session.GetWorld(), room, width, height);
            pixie::Telemetry& telemetry = session.GetWorld().GetTelemetry();
            telemetry.SetEnabled(true);
            std::map<std::string, std::vector<double>> samples;
            std::map<std::string, std::uint64_t> seen;
            double cold = 0.0;
            for (int f = 0; f <= frames; ++f) {
                const auto start = Clock::now();
                session.RenderToTarget(1.0);
                glFinish();
                const double wall = std::chrono::duration<double, std::milli>(Clock::now() - start).count();
                if (f == 0) {
                    cold = wall;
                    continue;
                }
                samples["wall"].push_back(wall);
                for (const auto& metric : telemetry.Snapshot()) {
                    if (metric.kind == pixie::MetricKind::Timing && metric.name.starts_with("lighting.") &&
                        metric.timing.samples != seen[metric.name]) {
                        seen[metric.name] = metric.timing.samples;
                        samples[metric.name].push_back(metric.timing.lastMs);
                    }
                }
            }
            std::map<std::string, double> gauges;
            for (const auto& metric : telemetry.Snapshot()) gauges[metric.name] = metric.gauge;
            csv << label << ",\"" << renderer << "\"," << width << ',' << height << ',' << room.name << ',' << room.lights << ','
                << room.boxes << ',' << room.particles << ',' << frames << ',' << cold;
            for (const char* name : {"wall", "lighting.cpu", "lighting.gpu.lights", "lighting.gpu.shadows", "lighting.gpu.combine"}) {
                const Summary s = Summarize(samples[name]);
                csv << ',' << s.p50 << ',' << s.p95 << ',' << s.p99 << ',' << s.max;
            }
            csv << ',' << gauges["lighting.lights"] << ',' << gauges["lighting.shadowed_lights"] << ','
                << gauges["lighting.shadowed_lights_dropped"] << ',' << gauges["lighting.occluder_edges"] << ','
                << gauges["lighting.particle_lights"] << ',' << gauges["lighting.particle_lights_dropped"] << '\n';
            csv.flush();
            std::cout << room.name << ": cold " << cold << " ms, warm wall p50 " << Summarize(samples["wall"]).p50 << " ms\n";
            session.Stop();
        }
        status = 0;
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return status;
}

} // namespace

int main(int argc, char** argv) {
    return Run(argc, argv);
}
