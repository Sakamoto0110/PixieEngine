#include "LightMachineProject.hpp"
#include "pixie/core/Log.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/Lighting2D.hpp"
#include "pixie/runtime/Name.hpp"
#include "pixie/runtime/ParticleSpawner.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/ScreenCanvas.hpp"
#include "pixie/runtime/RigidBody2D.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"
#include <algorithm>
#include <cmath>
#include <format>

using namespace pixie;

namespace {

PowerSwitch2D* Switch(World& world) {
    auto view = world.Registry().view<PowerSwitch2D>();
    return view.begin() == view.end() ? nullptr : &view.get<PowerSwitch2D>(*view.begin());
}

LightingSettings2D* Lighting(World& world) {
    auto view = world.Registry().view<LightingSettings2D>();
    return view.begin() == view.end() ? nullptr : &view.get<LightingSettings2D>(*view.begin());
}

// A small room for a World built without the scene: the scenario, a dark ambient, one lamp that
// follows the power, a floor, and a ceiling light. Y points down.
void BuildRoom(World& world) {
    auto scenario = world.Create("scenario");
    AddToScene(scenario);
    scenario.Add<PowerSwitch2D>();
    auto lighting = world.Create("lighting");
    AddToScene(lighting);
    auto& settings = lighting.Add<LightingSettings2D>();
    settings.ambient = {70, 80, 105};
    settings.ambientIntensity = 0.25;
    auto floor = world.Create("floor");
    AddToScene(floor);
    floor.Add<Transform2D>().position = {0, 200};
    auto& ground = floor.Add<Shape>();
    ground.kind = ShapeKind::Quad; ground.size = {320, 10}; ground.color = {70, 78, 96};
    auto lamp = world.Create("lamp powered");
    AddToScene(lamp);
    lamp.Add<Transform2D>().position = {-40, 100};
    auto& bulb = lamp.Add<Shape>();
    bulb.kind = ShapeKind::Circle; bulb.size = {9, 9}; bulb.color = {40, 40, 44};
    lamp.Add<Emission2D>() = {{90, 255, 140}, 2.0};
    lamp.Add<PoweredLamp2D>().onIntensity = 2.0;
    auto ceiling = world.Create("ceiling light");
    AddToScene(ceiling);
    ceiling.Add<Transform2D>().position = {80, -100};
    ceiling.Add<Light2D>().range = 360;
}

} // namespace

void LightMachineProject::Describe(ComponentTypes& types) const {
    types.Add<PowerSwitch2D>("PowerSwitch2D").Scenario()
        .Field<&PowerSwitch2D::powered>("powered").Tooltip("The machine's logical power; P switches it while playing.")
        .Field<&PowerSwitch2D::fadeSeconds>("fadeSeconds").Label("fade seconds").Tooltip("How long the lamps take to follow a switch.").Range(0, 10).Unit(" s");
    types.Add<PoweredLamp2D>("PoweredLamp2D")
        .Field<&PoweredLamp2D::onIntensity>("onIntensity").Label("on intensity").Tooltip("Emission2D intensity while powered.").Range(0, 16).Scrub(0.01)
        .Field<&PoweredLamp2D::offIntensity>("offIntensity").Label("off intensity").Tooltip("Emission2D intensity while not powered.").Range(0, 16).Scrub(0.01);
    types.Add<PoweredSpawner2D>("PoweredSpawner2D")
        .Field<&PoweredSpawner2D::onRate>("onRate").Label("on rate").Tooltip("ParticleSpawner rate while powered; 0 while not.").Range(0, 100000).Scrub(1);
}

void LightMachineProject::Build(World& world, Camera2D& /*camera*/) {
    if (world.Registry().view<LightingSettings2D>().empty()) BuildRoom(world);
    Telemetry& telemetry = world.GetTelemetry();
    powered_ = telemetry.RegisterGauge("machine.powered");
    lampIntensity_ = telemetry.RegisterGauge("machine.lamp_intensity");
    fading_ = false;
    world.AddSystem(SystemPhase::PreUpdate, "light-machine.power",
        [this](World& w, const FrameTime& time, const Input&) { Follow(w, time.delta); });
}

void LightMachineProject::BeforeStep(World& world, Camera2D& /*camera*/, const Input& input) {
    if (input.WasPressed(Key::P)) {
        if (PowerSwitch2D* power = Switch(world)) {
            power->powered = !power->powered;
            LogInfo(std::format("light-machine: power {}", power->powered ? "on" : "off"));
        }
    }
    if (input.WasPressed(Key::F)) {
        if (LightingSettings2D* lighting = Lighting(world)) {
            const bool on = !lighting->bloom;
            lighting->ambientOcclusion = on;
            lighting->fog = on;
            lighting->bloom = on;
        }
    }
}

// The lamps move toward the state's intensity at a constant rate, so the transition always ends
// fadeSeconds after a switch and the logical state is never ambiguous; the gauges record both.
void LightMachineProject::Follow(World& world, double delta) {
    const PowerSwitch2D* power = Switch(world);
    if (power == nullptr) return;
    double first = -1;
    fading_ = false;
    for (auto [id, lamp, emission] : world.Registry().view<const PoweredLamp2D, Emission2D>().each()) {
        const double target = power->powered ? lamp.onIntensity : lamp.offIntensity;
        const double step = power->fadeSeconds > 0 ? std::abs(lamp.onIntensity - lamp.offIntensity) * delta / power->fadeSeconds
                                                   : std::abs(target - emission.intensity);
        emission.intensity = emission.intensity < target ? std::min(target, emission.intensity + step)
                                                         : std::max(target, emission.intensity - step);
        fading_ = fading_ || emission.intensity != target;
        if (first < 0) first = emission.intensity;
    }
    for (auto [id, powered, spawner] : world.Registry().view<const PoweredSpawner2D, ParticleSpawner>().each()) {
        spawner.rate = power->powered ? powered.onRate : 0.0;
    }
    world.GetTelemetry().SetGauge(powered_, power->powered ? 1.0 : 0.0);
    world.GetTelemetry().SetGauge(lampIntensity_, std::max(first, 0.0));
}

void LightMachineProject::DrawScreen(World& world, ScreenCanvas& screen) {
    const TextStyle title{"fonts/inter-regular.ttf", 15, {235, 240, 250}};
    const TextStyle line{title.font, 12, {205, 215, 230}};
    const PowerSwitch2D* power = Switch(world);
    const LightingSettings2D* lighting = Lighting(world);
    screen.Rect({8, 8}, {420, 112}, {20, 25, 38, 235});
    screen.Text("A/D move  Space jump  P power  F effects", {14, 12}, line);
    if (power != nullptr) {
        // The logical state, then the visual transition, recorded together.
        screen.Text(std::format("power: {}{}", power->powered ? "ON" : "OFF", fading_ ? "  (lamps fading)" : ""), {14, 32}, title);
    }
    double y = 56;
    for (auto [id, name, lamp, emission] : world.Registry().view<const pixie::Name, const PoweredLamp2D, const Emission2D>().each()) {
        (void)lamp;
        screen.Text(std::format("{}: {:.2f}", name.value, emission.intensity), {14, y}, line);
        y += 16;
    }
    if (lighting != nullptr) {
        screen.Text(std::format("effects: {}", lighting->bloom ? "ambient occlusion, fog, bloom" : "off"), {220, 56}, line);
    }
}

bool LightMachineProject::SpawnAt(World& world, pxPoint at) {
    auto box = world.Create("dropped box");
    box.Add<Transform2D>().position = at;
    auto& shape = box.Add<Shape>();
    shape.kind = ShapeKind::Quad; shape.size = {12, 12}; shape.color = {120, 120, 130};
    box.Add<Collider2D>().halfSize = {12, 12};
    box.Add<RigidBody2D>();
    box.Add<Occluder2D>();
    return true;
}

namespace pixie {
std::unique_ptr<Project> CreateProject() { return std::make_unique<LightMachineProject>(); }
}
