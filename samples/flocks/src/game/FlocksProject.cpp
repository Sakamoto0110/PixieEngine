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
    types.Add<FlockControls>("FlockControls")
        .Field<&FlockControls::neighbourRadius>("neighbourRadius")
        .Field<&FlockControls::separationRadius>("separationRadius")
        .Field<&FlockControls::separationWeight>("separationWeight")
        .Field<&FlockControls::alignmentWeight>("alignmentWeight")
        .Field<&FlockControls::cohesionWeight>("cohesionWeight")
        .Field<&FlockControls::pointerWeight>("pointerWeight")
        .Field<&FlockControls::pointerRadius>("pointerRadius")
        .Field<&FlockControls::pointerMode>("pointerMode", {"Attract", "Repel"})
        .Field<&FlockControls::minSpeed>("minSpeed")
        .Field<&FlockControls::maxSpeed>("maxSpeed")
        .Field<&FlockControls::maxForce>("maxForce");
    types.Add<Pointer>("Pointer").Field<&Pointer::position>("position").Field<&Pointer::active>("active");
    types.Add<Bounds>("Bounds").Field<&Bounds::area>("area");
}

void FlocksProject::Build(pixie::World& world, pixie::Camera2D& camera) {
    flock_ = SetUpFlocks(world, camera.VisibleWorld(), count_);
    pixie::LogInfo(std::format(
        "flocks: {} boids; Q/A separation, W/S alignment, E/D cohesion, Space attract or repel, "
        "Up and Down change the count",
        FlockSize(world)));
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
