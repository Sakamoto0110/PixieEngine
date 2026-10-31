#include "LightLabProject.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/Lighting2D.hpp"
#include "pixie/runtime/RigidBody2D.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/World.hpp"

using namespace pixie;

namespace {

// A small lit room for a World built without the scene: the scenario, a dark ambient, one spot light,
// a floor and the door. Y points down.
void BuildRoom(World& world) {
    auto scenario = world.Create("scenario");
    AddToScene(scenario);
    scenario.Add<LightLabScenario>();
    auto lighting = world.Create("lighting");
    AddToScene(lighting);
    auto& settings = lighting.Add<LightingSettings2D>();
    settings.ambient = {70, 80, 105};
    settings.ambientIntensity = 0.35;
    const auto solid = [&](std::string_view name, pxPoint at, pxVec2 half, pxColorRgba color) {
        auto entity = world.Create(name);
        AddToScene(entity);
        entity.Add<Transform2D>().position = at;
        auto& shape = entity.Add<Shape>();
        shape.kind = ShapeKind::Quad; shape.size = half; shape.color = color;
        entity.Add<Collider2D>().halfSize = half;
        entity.Add<Occluder2D>();
        return entity;
    };
    solid("floor", {0, 200}, {320, 10}, {70, 78, 96});
    auto door = solid("door", {60, 120}, {8, 70}, {190, 150, 110});
    door.Add<RigidBody2D>().type = BodyType2D::Kinematic;
    door.Add<Door2D>();
    auto spot = world.Create("spot");
    AddToScene(spot);
    auto& pose = spot.Add<Transform2D>();
    pose.position = {-120, -120}; pose.rotation = 0.9;
    auto& light = spot.Add<Light2D>();
    light.type = LightType2D::Spot; light.range = 420; light.coneAngle = 1.2;
}

} // namespace

void LightLabProject::Describe(ComponentTypes& types) const {
    types.Add<Door2D>("Door2D")
        .Field<&Door2D::velocity>("velocity").Tooltip("Kinematic velocity in world units/s; it reverses every period.").Scrub(1)
        .Field<&Door2D::period>("period").Tooltip("Seconds of simulated time between reversals.").Range(0.1, 60).Unit(" s");
    types.Add<LightLabScenario>("LightLabScenario").Scenario()
        .Field<&LightLabScenario::doorSpeedScale>("doorSpeedScale").Label("door speed scale").Tooltip("Factor on every door's velocity.").Range(0, 4).Scrub(0.01);
}

void LightLabProject::Build(World& world, Camera2D& /*camera*/) {
    if (world.Registry().view<LightingSettings2D>().empty()) BuildRoom(world);
    doors_.clear();
    // Before the scene physics step, so the solve that follows uses this tick's velocity.
    world.AddSystem(SystemPhase::PreUpdate, "light-lab.doors", [this](World& w, const FrameTime& time, const Input&) {
        auto* physics = ScenePhysicsWorld2D(w);
        if (!physics) return;
        double scale = 1;
        for (auto [id, scenario] : w.Registry().view<const LightLabScenario>().each()) scale = scenario.doorSpeedScale;
        for (auto [id, door] : w.Registry().view<const Door2D>().each()) {
            const auto handle = SceneBody2D(w, id);
            if (!handle.value || physics->BodyType(handle) != BodyType2D::Kinematic || !(door.period > 0)) continue;
            auto& motion = doors_[id];
            if ((motion.timer += time.delta) >= door.period) { motion.timer -= door.period; motion.sign = -motion.sign; }
            physics->SetBodyVelocity(handle, door.velocity * (motion.sign * scale), 0.0);
        }
    });
}

bool LightLabProject::SpawnAt(World& world, pxPoint at) {
    auto box = world.Create("dropped box");
    box.Add<Transform2D>().position = at;
    auto& shape = box.Add<Shape>();
    shape.kind = ShapeKind::Quad; shape.size = {12, 12}; shape.color = {150, 120, 90};
    box.Add<Collider2D>().halfSize = {12, 12};
    box.Add<RigidBody2D>();
    box.Add<Occluder2D>();
    return true;
}

namespace pixie {
std::unique_ptr<Project> CreateProject() { return std::make_unique<LightLabProject>(); }
}
