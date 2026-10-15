#include "BoidsProject.hpp"

#include "BoidControls.hpp"
#include "BoidsScene.hpp"

#include "pixie/Log.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/World.hpp"

#include <algorithm>
#include <format>

namespace {

constexpr int kCountStep = 100;
constexpr int kMaxCount = 200000;
constexpr pxColorRgba kAttractColor{110, 230, 140, 110};
constexpr pxColorRgba kRepelColor{250, 110, 110, 110};

} // namespace

void BoidsProject::Describe(pixie::ComponentTypes& types) const {
    types.Add<BoidsScenario>("BoidsScenario")
        .Scenario()
        .Field<&BoidsScenario::count>("count")
        .Tooltip("How many boids the scene starts with. Rebuild the scene to apply.")
        .Range(0.0, 200000.0, 1.0)
        .Scrub(10.0);
    types.Add<BoidControls>("BoidControls")
        .Field<&BoidControls::mode>("mode", {"Attract", "Repel"})
        .Tooltip("Whether boids within reach steer toward the pointer or away from it.")
        .Field<&BoidControls::radius>("radius")
        .Tooltip("How far from the pointer a boid feels it, in world units.")
        .Range(0.0, 1000.0)
        .Scrub(1.0)
        .Field<&BoidControls::maxSpeed>("maxSpeed")
        .Label("max speed")
        .Tooltip("Fastest a boid flies, in world units per second.")
        .Range(0.0, 2000.0)
        .Scrub(1.0)
        .Field<&BoidControls::cruiseSpeed>("cruiseSpeed")
        .Label("cruise speed")
        .Tooltip("Speed a boid settles to when the pointer is out of reach, in world units per second.")
        .Range(0.0, 2000.0)
        .Scrub(1.0)
        .Field<&BoidControls::maxForce>("maxForce")
        .Label("max force")
        .Tooltip("Largest change of velocity per second, in world units per second squared.")
        .Range(0.0, 10000.0)
        .Scrub(5.0)
        .Field<&BoidControls::wander>("wander")
        .Tooltip("Largest random turn out of reach, in degrees per second.")
        .Degrees("°/s")
        .Range(0.0, 1080.0)
        .Scrub(1.0);
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

void BoidsProject::Build(pixie::World& world, pixie::Camera2D& camera) {
    scenarioEntity_ = pixie::SceneEntityWith<BoidsScenario>(world, "scenario", scenario_);
    controls_ = pixie::SceneEntityWith<BoidControls>(world, "controls");
    SetUpBoids(world, controls_, camera.VisibleWorld(), scenarioEntity_.Get<BoidsScenario>().count);
    pixie::LogInfo(std::format("boids: {} boids, attracting; Space switches, Up and Down change the count",
                               BoidCount(world)));
}

void BoidsProject::Capture(pixie::World&) {
    if (const auto* edited = scenarioEntity_.TryGet<BoidsScenario>()) {
        scenario_ = *edited;
    }
}

void BoidsProject::BeforeStep(pixie::World& world, pixie::Camera2D& camera, const pixie::Input& input) {
    controls_.Get<Bounds>().area = camera.VisibleWorld();
    const pixie::FramebufferSize viewport = camera.Viewport();
    const pxPoint cursor = input.Cursor();
    Pointer& pointer = controls_.Get<Pointer>();
    pointer.position = camera.ScreenToWorld(cursor);
    pointer.active = cursor.x >= 0.0 && cursor.y >= 0.0 && cursor.x < viewport.width &&
                     cursor.y < viewport.height;

    BoidControls& controls = controls_.Get<BoidControls>();
    if (input.WasPressed(pixie::Key::Space)) {
        controls.mode = controls.mode == SteerMode::Attract ? SteerMode::Repel : SteerMode::Attract;
        pixie::LogInfo(controls.mode == SteerMode::Attract ? "boids: attracting" : "boids: repelling");
    }
    int count = BoidCount(world);
    if (input.WasPressed(pixie::Key::Up)) {
        count = std::min(count + kCountStep, kMaxCount);
    }
    if (input.WasPressed(pixie::Key::Down)) {
        count = std::max(count - kCountStep, 0);
    }
    if (count != BoidCount(world)) {
        SetBoidCount(world, controls_, count);
        pixie::LogInfo(std::format("boids: {} boids", count));
    }
}

bool BoidsProject::SpawnAt(pixie::World& world, pxPoint at) {
    AddBoidAt(world, controls_, at);
    return true;
}

void BoidsProject::DrawOverlay(pixie::World&, pixie::ShapeRenderer& shapes, const pixie::Camera2D&) {
    const Pointer& pointer = controls_.Get<Pointer>();
    const BoidControls& controls = controls_.Get<BoidControls>();
    if (!pointer.active) {
        return;
    }
    const auto radius = static_cast<float>(controls.radius);
    const pxPointf at(static_cast<float>(pointer.position.x), static_cast<float>(pointer.position.y));
    shapes.Submit(pixie::ShapeKind::Ring,
                  {at, 0.0f, {radius, radius},
                   controls.mode == SteerMode::Attract ? kAttractColor : kRepelColor});
}

std::unique_ptr<pixie::Project> pixie::CreateProject() {
    return std::make_unique<BoidsProject>();
}
