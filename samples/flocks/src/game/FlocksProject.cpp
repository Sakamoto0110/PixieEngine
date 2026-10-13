#include "FlocksProject.hpp"

#include "FlockControls.hpp"
#include "FlocksScene.hpp"

#include "pixie/Log.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/World.hpp"

#include <algorithm>
#include <format>

namespace {

constexpr int kCountStep = 500;
constexpr int kMaxCount = 100000;
constexpr double kWeightStep = 0.1;
constexpr pxColorRgba kAttractColor{110, 230, 140, 110};
constexpr pxColorRgba kRepelColor{250, 110, 110, 110};

// Raises the weight with one key and lowers it with another; returns true if it changed.
bool Tune(const pixie::Input& input, pixie::Key up, pixie::Key down, double& weight) {
    const double before = weight;
    if (input.WasPressed(up)) {
        weight += kWeightStep;
    }
    if (input.WasPressed(down)) {
        weight = std::max(0.0, weight - kWeightStep);
    }
    return weight != before;
}

} // namespace

void FlocksProject::Describe(pixie::ComponentTypes& types) const {
    types.Add<FlocksScenario>("FlocksScenario")
        .Scenario()
        .Field<&FlocksScenario::count>("count")
        .Tooltip("How many boids the scene starts with. Rebuild the scene to apply.")
        .Range(0.0, 100000.0, 1.0)
        .Scrub(10.0);
    types.Add<FlockControls>("FlockControls")
        .Field<&FlockControls::neighbourRadius>("neighbourRadius")
        .Label("neighbour radius")
        .Tooltip("Boids within this distance are neighbours, in world units.")
        .Range(1.0, 300.0)
        .Scrub(0.5)
        .Field<&FlockControls::separationRadius>("separationRadius")
        .Label("separation radius")
        .Tooltip("Neighbours closer than this push the boid away, in world units.")
        .Range(0.0, 200.0)
        .Scrub(0.5)
        .Field<&FlockControls::separationWeight>("separationWeight")
        .Label("separation")
        .Tooltip("Weight of steering away from close neighbours.")
        .Range(0.0, 10.0, 0.05)
        .Scrub(0.01)
        .Field<&FlockControls::alignmentWeight>("alignmentWeight")
        .Label("alignment")
        .Tooltip("Weight of matching the neighbours' heading.")
        .Range(0.0, 10.0, 0.05)
        .Scrub(0.01)
        .Field<&FlockControls::cohesionWeight>("cohesionWeight")
        .Label("cohesion")
        .Tooltip("Weight of steering toward the neighbours' center.")
        .Range(0.0, 10.0, 0.05)
        .Scrub(0.01)
        .Field<&FlockControls::pointerWeight>("pointerWeight")
        .Label("pointer")
        .Tooltip("Weight of steering toward or away from the pointer.")
        .Range(0.0, 10.0, 0.05)
        .Scrub(0.01)
        .Field<&FlockControls::pointerRadius>("pointerRadius")
        .Label("pointer radius")
        .Tooltip("How far from the pointer a boid feels it, in world units.")
        .Range(0.0, 1000.0)
        .Scrub(1.0)
        .Field<&FlockControls::pointerMode>("pointerMode", {"Attract", "Repel"})
        .Label("pointer mode")
        .Tooltip("Whether the pointer attracts or repels.")
        .Field<&FlockControls::minSpeed>("minSpeed")
        .Label("min speed")
        .Tooltip("Slowest a boid flies, in world units per second.")
        .Range(0.0, 1000.0)
        .Scrub(1.0)
        .Field<&FlockControls::maxSpeed>("maxSpeed")
        .Label("max speed")
        .Tooltip("Fastest a boid flies, in world units per second.")
        .Range(0.0, 1000.0)
        .Scrub(1.0)
        .Field<&FlockControls::maxForce>("maxForce")
        .Label("max force")
        .Tooltip("Largest change of velocity per second for each rule, in world units per second squared.")
        .Range(0.0, 5000.0)
        .Scrub(2.0);
    types.Add<Pointer>("Pointer")
        .Field<&Pointer::position>("position")
        .Tooltip("The cursor in world units; the project writes it before every step.")
        .ReadOnly()
        .Field<&Pointer::active>("active")
        .Tooltip("Whether the cursor is over the view.")
        .ReadOnly();
    types.Add<Bounds>("Bounds")
        .Field<&Bounds::area>("area")
        .Tooltip("The visible part of the world, in world units; boids wrap at its edges. The project "
                 "writes it before every step.")
        .ReadOnly();
}

void FlocksProject::Build(pixie::World& world, pixie::Camera2D& camera) {
    scenarioEntity_ = world.Create("scenario");
    scenarioEntity_.Add<FlocksScenario>(scenario_);
    flock_ = SetUpFlocks(world, camera.VisibleWorld(), scenario_.count);
    pixie::LogInfo(std::format(
        "flocks: {} boids; Q/A separation, W/S alignment, E/D cohesion, Space attract or repel, "
        "Up and Down change the count",
        FlockSize(world)));
}

void FlocksProject::Capture(pixie::World&) {
    if (const auto* edited = scenarioEntity_.TryGet<FlocksScenario>()) {
        scenario_ = *edited;
    }
}

void FlocksProject::BeforeStep(pixie::World& world, pixie::Camera2D& camera, const pixie::Input& input) {
    flock_.Get<Bounds>().area = camera.VisibleWorld();
    const pixie::FramebufferSize viewport = camera.Viewport();
    const pxPoint cursor = input.Cursor();
    Pointer& pointer = flock_.Get<Pointer>();
    pointer.position = camera.ScreenToWorld(cursor);
    pointer.active = cursor.x >= 0.0 && cursor.y >= 0.0 && cursor.x < viewport.width &&
                     cursor.y < viewport.height;

    FlockControls& c = flock_.Get<FlockControls>();
    bool changed = Tune(input, pixie::Key::Q, pixie::Key::A, c.separationWeight);
    changed |= Tune(input, pixie::Key::W, pixie::Key::S, c.alignmentWeight);
    changed |= Tune(input, pixie::Key::E, pixie::Key::D, c.cohesionWeight);
    if (changed) {
        pixie::LogInfo(std::format("flocks: separation {:.1f}, alignment {:.1f}, cohesion {:.1f}",
                                   c.separationWeight, c.alignmentWeight, c.cohesionWeight));
    }
    if (input.WasPressed(pixie::Key::Space)) {
        c.pointerMode = c.pointerMode == PointerMode::Attract ? PointerMode::Repel : PointerMode::Attract;
        pixie::LogInfo(c.pointerMode == PointerMode::Attract ? "flocks: attracting" : "flocks: repelling");
    }
    int count = FlockSize(world);
    if (input.WasPressed(pixie::Key::Up)) {
        count = std::min(count + kCountStep, kMaxCount);
    }
    if (input.WasPressed(pixie::Key::Down)) {
        count = std::max(count - kCountStep, 0);
    }
    if (count != FlockSize(world)) {
        SetFlockSize(world, flock_, count);
        pixie::LogInfo(std::format("flocks: {} boids", count));
    }
}

void FlocksProject::DrawOverlay(pixie::World&, pixie::ShapeRenderer& shapes, const pixie::Camera2D&) {
    const Pointer& pointer = flock_.Get<Pointer>();
    const FlockControls& c = flock_.Get<FlockControls>();
    if (!pointer.active) {
        return;
    }
    const auto radius = static_cast<float>(c.pointerRadius);
    const pxPointf at(static_cast<float>(pointer.position.x), static_cast<float>(pointer.position.y));
    shapes.Submit(pixie::ShapeKind::Ring,
                  {at, 0.0f, {radius, radius},
                   c.pointerMode == PointerMode::Attract ? kAttractColor : kRepelColor});
}

std::unique_ptr<pixie::Project> pixie::CreateProject() {
    return std::make_unique<FlocksProject>();
}
