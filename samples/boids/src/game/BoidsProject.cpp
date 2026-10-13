#include "BoidsProject.hpp"

#include "BoidControls.hpp"
#include "BoidsScene.hpp"

#include "pixie/Log.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
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
    types.Add<BoidControls>("BoidControls")
        .Field<&BoidControls::mode>("mode", {"Attract", "Repel"})
        .Field<&BoidControls::radius>("radius")
        .Field<&BoidControls::maxSpeed>("maxSpeed")
        .Field<&BoidControls::cruiseSpeed>("cruiseSpeed")
        .Field<&BoidControls::maxForce>("maxForce")
        .Field<&BoidControls::wander>("wander");
    types.Add<Pointer>("Pointer").Field<&Pointer::position>("position").Field<&Pointer::active>("active");
    types.Add<Bounds>("Bounds").Field<&Bounds::area>("area");
}

void BoidsProject::Build(pixie::World& world, pixie::Camera2D& camera) {
    controls_ = SetUpBoids(world, camera.VisibleWorld(), count_);
    pixie::LogInfo(std::format("boids: {} boids, attracting; Space switches, Up and Down change the count",
                               BoidCount(world)));
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
