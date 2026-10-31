#include "CouplingLabProject.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/Name.hpp"
#include "pixie/runtime/ParticlePhysics2D.hpp"
#include "pixie/runtime/ParticleSpawner.hpp"
#include "pixie/runtime/ParticleSystem.hpp"
#include "pixie/runtime/RigidBody2D.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/ScreenCanvas.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/World.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <memory>
#include <numbers>
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
constexpr std::array<const char*, 4> kDropNames{"1 kg box", "10 kg box", "compound box", "1 kg ball"};

// The room scenes/main.scene holds, made in code for a World built without the scene (as the C API
// test builds it). Y points down; the floor's top is at y = 190. Entities outside the scene (dropped
// with the mouse) are runtime state and are never saved.
Entity Visible(World& world, std::string_view name, pxPoint at, pxVec2 half, pxColorRgba color, double rotation = 0,
               ShapeKind kind = ShapeKind::Quad, bool scene = true) {
    auto entity = world.Create(name);
    if (scene) AddToScene(entity);
    auto& pose = entity.Add<Transform2D>();
    pose.position = at; pose.rotation = rotation;
    auto& shape = entity.Add<Shape>();
    shape.kind = kind; shape.size = half; shape.color = color;
    return entity;
}
void Static(World& world, std::string_view name, pxPoint at, pxVec2 half, pxColorRgba color = {75, 95, 120}, double rotation = 0) {
    auto& collider = Visible(world, name, at, half, color, rotation).Add<Collider2D>();
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
    motor.category = 2; // The "characters" layer.
    if (keyboard) { motor.acceleration = 4500; character.Add<KeyboardControl2D>(); }
}
// kind: 0 box, 1 compound box (two halves), 2 ball. Zero mass keeps the density-derived mass.
Entity Body(World& world, std::string_view name, pxPoint at, pxVec2 half, double mass, pxColorRgba color, int kind = 0,
            bool scene = true) {
    auto body = Visible(world, name, at, half, color, 0, kind == 2 ? ShapeKind::Circle : ShapeKind::Quad, scene);
    if (kind == 1) {
        Collider2D left, right;
        left.halfSize = right.halfSize = {half.x / 2, half.y};
        left.offset = {-half.x / 2, 0}; right.offset = {half.x / 2, 0};
        body.Add<CompoundCollider2D>().shapes = {{left, {}}, {right, {}}};
    } else {
        auto& collider = body.Add<Collider2D>();
        if (kind == 2) { collider.kind = ColliderKind2D::Circle; collider.radius = half.x; }
        else collider.halfSize = half;
        collider.friction = 0.3;
    }
    body.Add<RigidBody2D>().massOverride = mass;
    return body;
}
// Dynamic particles raining from a segment `width` wide: they bounce off the bodies and platforms,
// not off the query-only characters.
void Rain(World& world, std::string_view name, pxPoint at, double width, Entity pool, pxColorRgba color) {
    auto rain = world.Create(name);
    AddToScene(rain);
    rain.Add<Transform2D>().position = at;
    auto& spawner = rain.Add<ParticleSpawner>();
    spawner.system = pool.Id();
    spawner.rate = 60; spawner.lifetime = 5; spawner.speedMin = 20; spawner.speedMax = 60;
    spawner.shape = EmissionShape::Segment; spawner.segmentEnd = {width, 0};
    spawner.direction = std::numbers::pi / 2; spawner.spread = 0.3;
    spawner.dynamic = true; spawner.ownLook = true; spawner.particleColor = color; spawner.particleSize = 2.5;
}
Entity Pool(World& world, std::string_view name, int mask) {
    auto pool = world.Create(name);
    AddToScene(pool);
    auto& particles = pool.Add<ParticleSystem>();
    particles.capacity = 4000; particles.style.fade = true;
    auto& profile = pool.Add<ParticlePhysics2D>();
    profile.collisionRadius = 2; profile.restitution = 0.5; profile.mask = mask;
    return pool;
}
void BuildRoom(World& world) {
    auto scenario = world.Create("scenario");
    AddToScene(scenario);
    scenario.Add<CouplingScenario>();
    auto settings = world.Create("world physics");
    AddToScene(settings);
    settings.Add<PhysicsSettings2D>();
    auto layers = world.Create("collision layers");
    AddToScene(layers);
    layers.Add<CollisionLayers2D>().layers = {{{1, "world"}, {}}, {{2, "characters"}, {}}};
    Static(world, "floor", {0, 200}, {880, 10});
    Static(world, "left wall", {-890, 0}, {10, 210});
    Static(world, "right wall", {890, 0}, {10, 210});
    // Ramps: 25 degrees is walkable, 55 degrees is steeper than the motors' 45-degree limit.
    Static(world, "ramp 25 degrees", {-760, 150}, {110, 8}, {95, 120, 150}, 0.4363);
    Static(world, "ramp 55 degrees", {770, 140}, {80, 8}, {150, 100, 100}, -0.9599);
    // A: translation. The post stops the rider, not the platform: carry obstructed, then the rider drops.
    Platform(world, "translating platform", {-560, 120}, {50, 6}, {{70, 0}, 0, 2.4, false});
    Character(world, "A translation", -560, 114, {100, 240, 190});
    Static(world, "carry post", {-400, 60}, {6, 22}, {130, 130, 145});
    // B: rotation within the walkable limit, with the rider off center so its support point moves sideways.
    Platform(world, "rotating platform", {-300, 110}, {55, 6}, {{0, 0}, 0.2, 2.0, false});
    Character(world, "B rotation", -275, 104, {100, 240, 190});
    // F: rotation past the walkable limit, so the rider slides off.
    Platform(world, "steep rotating platform", {-740, -40}, {55, 6}, {{0, 0}, 0.48, 2.0, false});
    Character(world, "F steep", -715, -46, {100, 240, 190});
    // C: an elevator with free space above it.
    Platform(world, "elevator", {-140, 170}, {40, 6}, {{0, -50}, 0, 2.0, false});
    Character(world, "C elevator", -140, 164, {100, 240, 190});
    // D: an elevator under a ceiling. It turns back when it crushes its rider (game policy).
    Platform(world, "crusher", {-10, 120}, {30, 6}, {{0, -50}, 0, 1.0, true});
    Character(world, "D crush", -10, 114, {240, 100, 150});
    Static(world, "crush ceiling", {-10, 32}, {34, 8}, {130, 130, 145});
    // E: the keyboard player with a ball, light, heavy and compound boxes, a stack, and a ram that
    // shoves a box into the stack.
    Character(world, "E player", 120, 190, {245, 220, 100}, true);
    Body(world, "ball 1 kg", {60, 176}, {14, 14}, 1, {150, 210, 255}, 2);
    Body(world, "light box 1 kg", {200, 174}, {16, 16}, 1, {90, 180, 245});
    Body(world, "heavy box 10 kg", {270, 174}, {16, 16}, 10, {220, 120, 75});
    Body(world, "compound box", {350, 174}, {16, 16}, 0, {125, 195, 155}, 1);
    for (int i = 0; i < 4; ++i)
        Body(world, std::format("stacked box {}", i + 1), {440, 174.0 - 32 * i}, {16, 16}, 1, {200, 200, 120});
    Body(world, "incoming box 2 kg", {520, 174}, {16, 16}, 2, {235, 200, 120});
    Platform(world, "ram", {600, 170}, {10, 20}, {{-160, 0}, 0, 1.0, false});
    // Dynamic particles over the rotating platform and the elevator, which also bounce off the
    // characters, and over the stack and the ram, whose pool leaves out the "characters" layer.
    Rain(world, "rain over B and C", {-360, -260}, 260, Pool(world, "particles hitting characters", 0x7fffffff), {255, 200, 120});
    Rain(world, "rain over the stack", {380, -260}, 240, Pool(world, "particles passing characters", 0x7fffffff & ~2), {140, 220, 255});
}
}

void CouplingLabProject::Describe(ComponentTypes& types) const {
    types.Add<Shuttle>("Shuttle")
        .Field<&Shuttle::velocity>("velocity").Label("velocity").Tooltip("Kinematic velocity in world units/s; it reverses every period.").Scrub(1)
        .Field<&Shuttle::angularVelocity>("angularVelocity").Label("angular velocity").Tooltip("Kinematic rotation speed, displayed in degrees/s; it reverses every period.").Degrees("°/s").Scrub(1)
        .Field<&Shuttle::period>("period").Label("period").Tooltip("Seconds of simulated time between reversals.").Range(0.1, 60).Unit(" s")
        .Field<&Shuttle::reverseOnCrush>("reverseOnCrush").Label("reverse on crush").Tooltip("Turn back as soon as a character it was carrying reports unsafe placement: a game's safety policy, not engine behavior.");
    types.Add<CouplingScenario>("CouplingScenario").Scenario()
        .Field<&CouplingScenario::speedScale>("speedScale").Label("speed scale").Tooltip("Factor on every shuttle's velocity and angular velocity.").Range(0, 4).Scrub(0.01);
}

void CouplingLabProject::Build(World& world, Camera2D& camera) {
    if (world.Registry().view<PhysicsSettings2D>().empty()) BuildRoom(world);
    camera.SetZoom(0.72); // The whole room, 1780 units wide, in a 1280-pixel window.
    shuttles_.clear(); lastSupport_.clear(); dropped_.clear(); dragged_ = {};
    initial_.clear();
    for (auto [id, pose] : world.Registry().view<const Transform2D>().each()) initial_.emplace_back(world.Wrap(id), pose);
    // Before the scene physics step, so the solve that follows uses this tick's velocity.
    world.AddSystem(SystemPhase::PreUpdate, "coupling-lab.shuttles",
        [this](World& w, const FrameTime& time, const Input&) { Drive(w, time.delta); });
}

void CouplingLabProject::Drive(World& world, double delta) {
    auto* physics = ScenePhysicsWorld2D(world);
    if (!physics) return;
    double scale = 1;
    for (auto [id, scenario] : world.Registry().view<const CouplingScenario>().each()) scale = scenario.speedScale;
    // A character crushed off a reverseOnCrush shuttle sends that shuttle back along its path.
    for (auto [id, motor] : world.Registry().view<const CharacterMotor2D>().each()) {
        const auto* coupling = CharacterCoupling2D(world, id);
        if (!coupling) continue;
        if (!coupling->unsafePlacement) {
            lastSupport_[id] = coupling->support.value;
            continue;
        }
        for (auto [shuttleId, shuttle] : world.Registry().view<const Shuttle>().each()) {
            auto& motion = shuttles_[shuttleId];
            if (shuttle.reverseOnCrush && lastSupport_[id] != 0 && SceneBody2D(world, shuttleId).value == lastSupport_[id]) {
                motion.sign = -motion.sign;
                motion.timer = std::max(0.0, shuttle.period - motion.timer);
            }
        }
        lastSupport_[id] = 0;
    }
    for (auto [id, shuttle] : world.Registry().view<const Shuttle>().each()) {
        const auto handle = SceneBody2D(world, id);
        if (!handle.value || physics->BodyType(handle) != BodyType2D::Kinematic || !(shuttle.period > 0)) continue;
        auto& motion = shuttles_[id];
        if ((motion.timer += delta) >= shuttle.period) { motion.timer -= shuttle.period; motion.sign = -motion.sign; }
        physics->SetBodyVelocity(handle, shuttle.velocity * (motion.sign * scale), shuttle.angularVelocity * motion.sign * scale);
    }
}

void CouplingLabProject::ResetRoom(World& world) {
    for (auto dropped : dropped_) if (dropped) world.Destroy(dropped);
    for (auto [id, particles] : world.Registry().view<ParticleSystem>().each()) particles.particles.clear();
    dropped_.clear(); dragged_ = {}; shuttles_.clear(); lastSupport_.clear();
    for (auto& [entity, pose] : initial_) if (entity) entity.Get<Transform2D>() = pose;
    ResetSceneMovement2D(world); // Rebuilds bodies at their authored poses and initial velocities.
}

void CouplingLabProject::BeforeStep(World& world, Camera2D& camera, const Input& input) {
    if (input.WasPressed(Key::R)) { ResetRoom(world); return; }
    for (int i = 0; i < 4; ++i)
        if (input.WasPressed(static_cast<Key>(49 + i)) || input.WasPressed(static_cast<Key>(321 + i))) dropKind_ = i;
    const auto at = camera.ScreenToWorld(input.Cursor());
    if (input.WasPressed(MouseButton::Left)) {
        dragged_ = {};
        // The native authored outline, including rotation, decides what is under the pointer.
        auto picked = world.Wrap(PickMovementGizmo2D(world, at, 1));
        if (picked && picked.Has<RigidBody2D>() && picked.Get<RigidBody2D>().type == BodyType2D::Dynamic)
            if (auto* physics = ScenePhysicsWorld2D(world)) {
                dragged_ = picked;
                dragLocal_ = physics->BodyLocalPoint(SceneBody2D(world, picked.Id()), at);
            }
    }
    if (!input.IsDown(MouseButton::Left)) dragged_ = {};
    if (dragged_) if (auto* physics = ScenePhysicsWorld2D(world)) {
        const auto handle = SceneBody2D(world, dragged_.Id());
        if (handle.value) {
            const auto point = physics->BodyWorldPoint(handle, dragLocal_);
            const double mass = physics->BodyMass(handle).mass;
            // A bounded spring at the grabbed point keeps native collision and rotation.
            auto force = ((at - point) * 100.0 - physics->BodyPointVelocity(handle, point) * 20.0 - physics->Gravity()) * mass;
            const double magnitude = std::hypot(force.x, force.y);
            if (magnitude > 200000) force = force * (200000 / magnitude);
            physics->ApplyForce(handle, force, point);
        }
    }
    if (input.WasPressed(MouseButton::Right)) SpawnAt(world, at);
}

bool CouplingLabProject::SpawnAt(World& world, pxPoint at) {
    static constexpr std::array<pxColorRgba, 4> colors{{{90, 180, 245}, {220, 120, 75}, {125, 195, 155}, {150, 210, 255}}};
    const int kind = dropKind_ == 2 ? 1 : dropKind_ == 3 ? 2 : 0;
    dropped_.push_back(Body(world, std::format("dropped {}", kDropNames[dropKind_]), at, kind == 2 ? pxVec2{14, 14} : pxVec2{16, 16},
                            dropKind_ == 1 ? 10 : dropKind_ == 2 ? 0 : 1, colors[dropKind_], kind, false));
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
    screen.Rect({8, 8}, {640, 48.0 + 18.0 * static_cast<double>(characters.size())}, {20, 25, 38, 235});
    screen.Text(std::format("A/D move  Space jump  R reset  LMB drag  RMB drop {} (1-4 choose)", kDropNames[dropKind_]), {14, 12}, line);
    screen.Text("character  support  world velocity  carried  push impulse  support load  status", {14, 30}, title);
    double y = 52;
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
