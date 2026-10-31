#include "CouplingLabProject.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/Name.hpp"
#include "pixie/runtime/RigidBody2D.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/ScreenCanvas.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"
#include <cmath>
#include <format>
#include <map>
#include <memory>
#include <string>

using namespace pixie;
namespace {
const char* Blocked(CharacterBlocked2D blocked) {
    switch (blocked) {
    case CharacterBlocked2D::SupportStepLimit: return "support step limit";
    case CharacterBlocked2D::CarryObstructed: return "carry obstructed";
    case CharacterBlocked2D::NoValidPlacement: return "no valid placement";
    case CharacterBlocked2D::WorldChanged: return "world changed";
    default: return "free";
    }
}
const char* Support(const CollisionWorld2D* physics, BodyHandle2D support) {
    if (!physics || !support.value) return "air";
    switch (physics->BodyType(support)) {
    case BodyType2D::Static: return "static";
    case BodyType2D::Kinematic: return "kinematic";
    default: return "dynamic";
    }
}

// The room scenes/main.scene holds, made in code for a World built without the scene (as the C API
// test builds it). Y points down; the floor's top is at y = 190.
Entity Visible(World& world, std::string_view name, pxPoint at, pxVec2 half, pxColorRgba color) {
    auto entity = world.Create(name);
    AddToScene(entity);
    entity.Add<Transform2D>().position = at;
    auto& shape = entity.Add<Shape>();
    shape.kind = ShapeKind::Quad; shape.size = half; shape.color = color;
    return entity;
}
void Static(World& world, std::string_view name, pxPoint at, pxVec2 half, pxColorRgba color = {75, 95, 120}) {
    auto& collider = Visible(world, name, at, half, color).Add<Collider2D>();
    collider.halfSize = half; collider.friction = 0.3;
}
void Platform(World& world, std::string_view name, pxPoint at, pxVec2 half, Shuttle shuttle) {
    auto platform = Visible(world, name, at, half, {170, 100, 235});
    auto& collider = platform.Add<Collider2D>();
    collider.halfSize = half; collider.friction = 0.3;
    auto& body = platform.Add<RigidBody2D>();
    body.type = BodyType2D::Kinematic;
    body.initialVelocity = shuttle.velocity; body.initialAngularVelocity = shuttle.angularVelocity;
    platform.Add<Shuttle>() = shuttle;
}
void Character(World& world, std::string_view name, double x, double feet, pxColorRgba color, bool keyboard = false) {
    // The default capsule is 48 units tall; its center sits 24.5 above the surface it stands on.
    auto character = Visible(world, name, {x, feet - 24.5}, {12, 24}, color);
    auto& motor = character.Add<CharacterMotor2D>();
    if (keyboard) { motor.acceleration = 4500; character.Add<KeyboardControl2D>(); }
}
void Box(World& world, std::string_view name, double x, pxVec2 half, double mass, pxColorRgba color, bool compound = false) {
    auto box = Visible(world, name, {x, 190 - half.y}, half, color);
    if (compound) {
        Collider2D left, right;
        left.halfSize = right.halfSize = {half.x / 2, half.y};
        left.offset = {-half.x / 2, 0}; right.offset = {half.x / 2, 0};
        box.Add<CompoundCollider2D>().shapes = {{left, {}}, {right, {}}};
    } else {
        auto& collider = box.Add<Collider2D>();
        collider.halfSize = half; collider.friction = 0.3;
    }
    box.Add<RigidBody2D>().massOverride = mass;
}
void BuildRoom(World& world) {
    auto scenario = world.Create("scenario");
    AddToScene(scenario);
    scenario.Add<CouplingScenario>();
    auto settings = world.Create("world physics");
    AddToScene(settings);
    settings.Add<PhysicsSettings2D>();
    Static(world, "floor", {0, 200}, {660, 10});
    Static(world, "left wall", {-670, 60}, {10, 150});
    Static(world, "right wall", {670, 60}, {10, 150});
    // A: translation. The post stops the rider, not the platform: carry obstructed, then the rider drops.
    Platform(world, "translating platform", {-560, 120}, {50, 6}, {{70, 0}, 0, 2.4});
    Character(world, "A translation", -560, 114, {100, 240, 190});
    Static(world, "carry post", {-400, 60}, {6, 22}, {130, 130, 145});
    // B: rotation, with the rider off center so the support point has tangential velocity.
    Platform(world, "rotating platform", {-300, 110}, {55, 6}, {{0, 0}, 0.2, 2.0});
    Character(world, "B rotation", -275, 104, {100, 240, 190});
    // C: an elevator with free space above it.
    Platform(world, "elevator", {-140, 170}, {40, 6}, {{0, -50}, 0, 2.0});
    Character(world, "C elevator", -140, 164, {100, 240, 190});
    // D: an elevator under a ceiling crushes its rider (unsafe placement); its lowest pose clears the floor.
    Platform(world, "crusher", {-10, 120}, {30, 6}, {{0, -50}, 0, 1.0});
    Character(world, "D crush", -10, 114, {240, 100, 150});
    Static(world, "crush ceiling", {-10, 32}, {34, 8}, {130, 130, 145});
    // E: the keyboard player with light, heavy and compound boxes, and a ram that shoves a box back.
    Character(world, "E player", 120, 190, {245, 220, 100}, true);
    Box(world, "light box 1 kg", 200, {16, 16}, 1, {90, 180, 245});
    Box(world, "heavy box 10 kg", 270, {16, 16}, 10, {220, 120, 75});
    Box(world, "compound box", 350, {16, 16}, 0, {125, 195, 155}, true);
    Box(world, "incoming box 2 kg", 520, {16, 16}, 2, {235, 200, 120});
    Platform(world, "ram", {600, 170}, {10, 20}, {{-160, 0}, 0, 1.0});
}
}

void CouplingLabProject::Describe(ComponentTypes& types) const {
    types.Add<Shuttle>("Shuttle")
        .Field<&Shuttle::velocity>("velocity").Label("velocity").Tooltip("Kinematic velocity in world units/s; it reverses every period.").Scrub(1)
        .Field<&Shuttle::angularVelocity>("angularVelocity").Label("angular velocity").Tooltip("Kinematic rotation speed, displayed in degrees/s; it reverses every period.").Degrees("°/s").Scrub(1)
        .Field<&Shuttle::period>("period").Label("period").Tooltip("Seconds of simulated time between reversals.").Range(0.1, 60).Unit(" s");
    types.Add<CouplingScenario>("CouplingScenario").Scenario()
        .Field<&CouplingScenario::speedScale>("speedScale").Label("speed scale").Tooltip("Factor on every shuttle's velocity and angular velocity.").Range(0, 4).Scrub(0.01);
}

void CouplingLabProject::Build(World& world, Camera2D&) {
    if (world.Registry().view<PhysicsSettings2D>().empty()) BuildRoom(world);
    // Before the scene physics step, so the solve that follows uses this tick's velocity.
    world.AddSystem(SystemPhase::PreUpdate, "coupling-lab.shuttles", [](World& w, const FrameTime& time, const Input&) {
        auto* physics = ScenePhysicsWorld2D(w);
        if (!physics) return;
        double scale = 1;
        for (auto [id, scenario] : w.Registry().view<const CouplingScenario>().each()) scale = scenario.speedScale;
        for (auto [id, shuttle] : w.Registry().view<const Shuttle>().each()) {
            const auto handle = SceneBody2D(w, id);
            if (!handle.value || physics->BodyType(handle) != BodyType2D::Kinematic || !(shuttle.period > 0)) continue;
            const double sign = static_cast<long long>(std::floor(time.elapsed / shuttle.period)) % 2 == 0 ? scale : -scale;
            physics->SetBodyVelocity(handle, shuttle.velocity * sign, shuttle.angularVelocity * sign);
        }
    });
}

bool CouplingLabProject::SpawnAt(World& world, pxPoint at) {
    // A 1 kg box dropped where asked; runtime state, not part of the authored room.
    auto box = world.Create("dropped box");
    box.Add<Transform2D>().position = at;
    auto& shape = box.Add<Shape>();
    shape.kind = ShapeKind::Quad; shape.size = {12, 12}; shape.color = {150, 210, 255};
    box.Add<Collider2D>().halfSize = {12, 12};
    box.Add<RigidBody2D>().massOverride = 1;
    return true;
}

void CouplingLabProject::DrawScreen(World& world, ScreenCanvas& screen) {
    const TextStyle title{"fonts/inter-regular.ttf", 15, {235, 240, 250}};
    const TextStyle line{title.font, 12, {205, 215, 230}};
    const TextStyle alert{title.font, 12, {255, 150, 140}};
    // Name order, so a row keeps its place while the room runs.
    std::map<std::string, entt::entity> characters;
    for (auto [id, name, motor] : world.Registry().view<const pixie::Name, const CharacterMotor2D>().each()) characters[name.value] = id;
    const auto* physics = SceneCollisionWorld2D(world);
    screen.Rect({8, 8}, {620, 30.0 + 18.0 * static_cast<double>(characters.size())}, {20, 25, 38, 235});
    screen.Text("character  support  world velocity  carried  push impulse  support load  status", {14, 14}, title);
    double y = 36;
    for (const auto& [name, id] : characters) {
        const auto* coupling = CharacterCoupling2D(world, id);
        if (!coupling) continue;
        const bool unsafe = coupling->unsafePlacement;
        screen.Text(std::format("{:<10}  {:<9}  ({:6.1f}, {:6.1f})  ({:5.1f}, {:5.1f})  {:7.2f}  {:7.2f}  {}", name,
                                Support(physics, coupling->support), coupling->worldVelocity.x, coupling->worldVelocity.y,
                                coupling->carried.x, coupling->carried.y, coupling->pushedImpulse, coupling->supportLoad,
                                unsafe ? "UNSAFE placement" : Blocked(coupling->blocked)),
                    {14, y}, unsafe ? alert : line);
        y += 18;
    }
}

namespace pixie {
std::unique_ptr<Project> CreateProject() { return std::make_unique<CouplingLabProject>(); }
}
