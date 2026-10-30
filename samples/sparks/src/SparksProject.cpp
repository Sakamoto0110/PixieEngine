#include "SparksProject.hpp"
#include "pixie/core/Log.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/Name.hpp"
#include "pixie/runtime/ParticlePhysics2D.hpp"
#include "pixie/runtime/ParticleSystem.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/World.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <format>
#include <cstdlib>
#include <set>
#include <array>
#include "pixie/runtime/ScreenCanvas.hpp"

using namespace pixie;
namespace {
Entity Named(World& world, std::string_view name) {
    for (auto [id, existing] : world.Registry().view<const Name>().each())
        if (existing.value == name) return world.Wrap(id);
    auto entity = world.Create(name); AddToScene(entity); return entity;
}
}
void SparksProject::Describe(ComponentTypes& types) const {
    types.Add<SparksScenario>("SparksScenario").Scenario()
        .Field<&SparksScenario::burstCount>("burstCount").Label("click burst").Tooltip("Sparks emitted by each click on empty space.").Range(1, 1000);
}
void SparksProject::Build(World& world, Camera2D&) {
    initialBodies_.clear(); dragged_ = {}; editingMass_ = false; massError_.clear();
    scenario_ = Named(world, "scenario");
    if (!scenario_.Has<SparksScenario>()) scenario_.Add<SparksScenario>();
    floor_ = Named(world, "floor");
    if (!floor_.Has<Transform2D>()) {
        floor_.Add<Transform2D>().position = {0, 150};
        floor_.Add<Collider2D>().halfSize = {430, 10};
        auto& shape = floor_.Add<Shape>(); shape.size = {430, 10}; shape.color = {75, 95, 120};
    }
    box_ = Named(world, "drag box");
    if (!box_.Has<Transform2D>()) {
        box_.Add<Transform2D>().position = {-120, 122};
        box_.Add<Collider2D>().halfSize = {18, 18};
        auto& body = box_.Add<RigidBody2D>(); body.massOverride = 1; body.fixedRotation = true;
        auto& shape = box_.Add<Shape>(); shape.size = {18, 18}; shape.color = {90, 180, 245};
    }
    // Deliberately unequal extents and tilted starting poses exercise real angular dynamics.
    const pxPoint starts[]{{105, 25}, {205, -35}, {300, 40}};
    const pxVec2 sizes[]{{36, 10}, {10, 34}, {42, 9}};
    const double turns[]{0.24, -0.55, 0.65};
    for (int i = 0; i < 3; ++i) {
        auto rectangle = Named(world, std::format("rectangle {}", i+1));
        if (!rectangle.Has<Transform2D>()) {
            auto& pose = rectangle.Add<Transform2D>(); pose.position = starts[i]; pose.rotation = turns[i];
            rectangle.Add<Collider2D>().halfSize = sizes[i];
            auto& body = rectangle.Add<RigidBody2D>(); body.massOverride = i+1; body.fixedRotation = false;
            auto& shape = rectangle.Add<Shape>(); shape.kind = ShapeKind::Quad;
            shape.size = sizes[i]; shape.color = {125, 195, 155};
        }
    }
    const auto fixture = [&](std::string_view name, pxPoint at, pxVec2 half, double turn, double mass) {
        auto entity = Named(world, name);
        if (entity.Has<Transform2D>()) return;
        auto& pose = entity.Add<Transform2D>(); pose.position = at; pose.rotation = turn;
        auto& collider = entity.Add<Collider2D>(); collider.halfSize = half; collider.friction = 0.65;
        auto& shape = entity.Add<Shape>(); shape.kind = ShapeKind::Quad; shape.size = half;
        shape.color = mass > 0 ? pxColorRgba{220, 170, 100} : pxColorRgba{100, 115, 145};
        if (mass > 0) { auto& body = entity.Add<RigidBody2D>(); body.massOverride = mass; body.fixedRotation = false; }
    };
    fixture("left ramp", {-335, 108}, {65, 8}, 0.35, 0);
    fixture("right ramp", {165, 102}, {85, 8}, -0.30, 0);
    fixture("drop platform", {-310, 10}, {55, 7}, 0, 0);
    fixture("stack base", {300, 122}, {18, 18}, 0, 2);
    fixture("drop box", {288, 72}, {14, 14}, 0.08, 0.5);
    for (auto [id, body, pose] : world.Registry().view<RigidBody2D, Transform2D>().each())
        if (body.type == BodyType2D::Dynamic) initialBodies_.push_back({world.Wrap(id), pose, body});
    SelectBody(box_);
    pool_ = Named(world, "sparks");
    if (!pool_.Has<ParticleSystem>()) {
        auto& system = pool_.Add<ParticleSystem>(); system.capacity = 10000;
        system.style.shape = ParticleShape::Streak;
        system.style.additive = true;
        pool_.Add<ParticlePhysics2D>().collisionRadius = 1.5;
    }
    player_ = Named(world, "player");
    if (!player_.Has<Transform2D>()) {
        player_.Add<Transform2D>().position = {-220, 116};
        auto& motor = player_.Add<CharacterMotor2D>(); motor.acceleration = 4500; motor.maxPushForce = 2500;
        player_.Add<KeyboardControl2D>();
        auto& shape = player_.Add<Shape>(); shape.size = {12, 24}; shape.color = {245, 220, 100};
    }
    initialBox_ = box_.Get<Transform2D>().position;
    initialPlayer_ = player_.Get<Transform2D>().position;
    seed_ = world.GetRandom().Seed();
    contacts_.clear();
    InstallSceneMovement2D(world);
    auto& telemetry = world.GetTelemetry();
    contactTime_ = telemetry.RegisterTiming("sample.sparks.contacts");
    contactCount_ = telemetry.RegisterGauge("sample.sparks.contact_points", "points");
    emittedCount_ = telemetry.RegisterGauge("sample.sparks.emitted", "particles");
    world.AddSystem(SystemPhase::PostUpdate, "sparks.contacts",
        [this](World& current, const FrameTime& time, const Input&) { ContactSparks(current, time.delta); });
    LogInfo("sparks: A/D or arrows move the player; Space jumps; push the blue box for floor sparks; right click fires a burst; left drag moves bodies; R resets");
}
void SparksProject::Burst(World& world, pxPoint at, int count, double direction) {
    auto& system = pool_.Get<ParticleSystem>();
    auto& random = world.GetRandom();
    for (int i=0; i<count; ++i) {
        const pxVec2f velocity{static_cast<float>(random.Range(-200, 200)+direction),
                              static_cast<float>(random.Range(-340, -60))};
        const pxColorRgba color{255, static_cast<std::uint8_t>(random.Int(110, 220)), 35};
        if (!system.Emit({static_cast<float>(at.x), static_cast<float>(at.y)}, velocity,
                         static_cast<float>(random.Range(1.5, 3.0)), color,
                         1.5f, ParticleShape::Streak, true)) break;
    }
}
bool SparksProject::SpawnAt(World& world, pxPoint at) {
    Burst(world, at, std::clamp(scenario_.Get<SparksScenario>().burstCount, 1, 1000));
    return true;
}
void SparksProject::ResetRoom(World& world) {
    pool_.Get<ParticleSystem>().particles.clear();
    dragged_ = {}; editingMass_ = false;
    for (const auto& initial : initialBodies_) {
        initial.entity.Get<Transform2D>() = initial.transform;
        initial.entity.Get<RigidBody2D>() = initial.body;
    }
    SelectBody(box_);
    box_.Get<Transform2D>().position = initialBox_;
    player_.Get<Transform2D>().position = initialPlayer_;
    ResetSceneMovement2D(world);
    world.GetRandom().Reseed(seed_);
    contacts_.clear();
}
void SparksProject::SelectBody(Entity entity) {
    selected_ = entity; editingMass_ = false; replaceMass_ = false; massError_.clear();
    massText_ = selected_ ? std::format("{:g}", selected_.Get<RigidBody2D>().massOverride) : "";
}
void SparksProject::EditMass(const Input& input) {
    if (!editingMass_) return;
    if (input.WasPressed(Key::Escape)) { SelectBody(selected_); return; }
    if (input.WasPressed(Key::Backspace)) {
        if (replaceMass_) massText_.clear();
        else if (!massText_.empty()) massText_.pop_back();
        replaceMass_ = false;
    }
    if ((input.IsDown(Key::LeftControl) || input.IsDown(Key::RightControl)) && input.WasPressed(Key::A))
        replaceMass_ = true;
    for (int i = 0; i <= 9; ++i) if (input.WasPressed(static_cast<Key>(48+i)) || input.WasPressed(static_cast<Key>(320+i))) {
        if (replaceMass_) massText_.clear();
        if (massText_.size() < 16) massText_ += static_cast<char>('0'+i);
        replaceMass_ = false;
    }
    if (input.WasPressed(Key::Period) || input.WasPressed(Key::Comma) || input.WasPressed(static_cast<Key>(330))) {
        if (replaceMass_) massText_.clear();
        if (massText_.find('.') == std::string::npos && massText_.size() < 16) massText_ += '.';
        replaceMass_ = false;
    }
    if (input.WasPressed(Key::Enter) || input.WasPressed(Key::KeypadEnter)) {
        char* end = nullptr;
        const double mass = std::strtod(massText_.c_str(), &end);
        if (massText_.empty() || *end || !std::isfinite(mass) || mass < 0.01 || mass > 1000) {
            massError_ = "Enter a mass from 0.01 to 1000 kg";
        } else {
            auto& world = selected_.GetWorld();
            auto* physics = ScenePhysicsWorld2D(world);
            const auto handle = SceneBody2D(world, selected_.Id());
            const bool live = physics && handle.value;
            const auto motion = live ? physics->BodyMotion(handle) : BodyMotion2D{};
            selected_.Get<RigidBody2D>().massOverride = mass;
            // Reconfiguration recalculates inertia; retain motion instead of restarting a falling body.
            SyncSceneMovement2D(world);
            if (live) if (auto* updated = ScenePhysicsWorld2D(world))
                updated->SetBodyVelocity(SceneBody2D(world, selected_.Id()), motion.velocity, motion.angularVelocity);
            SelectBody(selected_);
        }
    }
}
void SparksProject::BeforeStep(World& world, Camera2D& camera, const Input& input) {
    const bool reset = !editingMass_ && input.WasPressed(Key::R);
    player_.Get<KeyboardControl2D>().enabled = !reset && !editingMass_ && dragged_ != player_;
    if (reset) { ResetRoom(world); return; }
    const auto cursor = input.Cursor();
    const bool overField = cursor.x >= 12 && cursor.x <= 282 && cursor.y >= 42 && cursor.y <= 74;
    if (input.WasPressed(MouseButton::Left)) {
        if (overField && selected_) { editingMass_ = true; replaceMass_ = true; dragged_ = {}; }
        else {
            editingMass_ = false; massError_.clear(); dragged_ = {};
            const auto at = camera.ScreenToWorld(cursor);
            // Query the native authored outline, including rotation, rather than an axis-aligned rectangle.
            auto picked = world.Wrap(PickMovementGizmo2D(world, at, 1));
            if (picked && picked.Has<RigidBody2D>() && picked.Get<RigidBody2D>().type == BodyType2D::Dynamic) {
                SelectBody(picked); dragged_ = picked;
                if (auto* physics = ScenePhysicsWorld2D(world))
                    dragLocal_ = physics->BodyLocalPoint(SceneBody2D(world, picked.Id()), at);
            } else if (picked == player_) { dragged_ = player_; }
        }
    }
    EditMass(input);
    if (!input.IsDown(MouseButton::Left)) dragged_ = {};
    if (dragged_ && !editingMass_) {
        const auto target = camera.ScreenToWorld(cursor);
        if (dragged_ == player_) {
            if (const auto* collision = SceneCollisionWorld2D(world)) {
                auto& pose = player_.Get<Transform2D>(); const auto& motor = player_.Get<CharacterMotor2D>();
                auto travel = target-pose.position; const double distance = std::hypot(travel.x, travel.y);
                if (distance > 32) travel = travel*(32/distance);
                pose.position = collision->MoveCapsule(pose.position, motor.radius, motor.halfSegment,
                                                       travel, {0, 0}, 0.70710678).position;
            }
        } else if (auto* physics = ScenePhysicsWorld2D(world)) {
            const auto handle = SceneBody2D(world, dragged_.Id());
            const auto point = physics->BodyWorldPoint(handle, dragLocal_);
            const double mass = physics->BodyMass(handle).mass;
            auto force = ((target-point)*100.0 - physics->BodyPointVelocity(handle, point)*20.0 - physics->Gravity())*mass;
            const double magnitude = std::hypot(force.x, force.y);
            if (magnitude > 200000) force = force*(200000/magnitude);
            // A bounded spring force at the grabbed point preserves native collision and angular response.
            physics->ApplyForce(handle, force, point);
        }
    }
    if (!editingMass_ && input.WasPressed(MouseButton::Right)) SpawnAt(world, camera.ScreenToWorld(cursor));
}
void SparksProject::DrawScreen(World& world, ScreenCanvas& screen) {
    const TextStyle text{"fonts/inter-regular.ttf", 15, {235, 240, 250}};
    screen.Rect({8, 8}, {360, massError_.empty() ? 102.0 : 124.0}, {20, 25, 38, 240});
    std::string label = selected_ ? selected_.Get<pixie::Name>().value : "select a dynamic box";
    screen.Text("Mass (kg) - " + label, {12, 16}, text);
    screen.Rect({12, 42}, {270, 32}, editingMass_ ? pxColorRgba{65, 90, 125} : pxColorRgba{38, 47, 66});
    if (!editingMass_ && selected_) {
        const auto* physics = SceneCollisionWorld2D(world); const auto handle = SceneBody2D(world, selected_.Id());
        if (physics && handle.value) massText_ = std::format("{:.4g}", physics->BodyMass(handle).mass);
    }
    screen.Text(massText_ + (editingMass_ ? " |" : ""), {20, 48}, text);
    screen.Text("Click field, type, Enter applies; Esc cancels", {12, 80}, TextStyle{text.font, 12, text.color});
    if (!massError_.empty()) screen.Text(massError_, {12, 102}, TextStyle{text.font, 12, {255, 160, 140}});
}
void SparksProject::ContactSparks(World& world, double delta) {
    auto& telemetry = world.GetTelemetry();
    const TelemetryScope measure(telemetry, contactTime_);
    const auto* physics = SceneCollisionWorld2D(world);
    std::map<ContactKey, ContactHistory> next;
    std::set<ContactKey> seen;
    int emitted = 0, points = 0;
    if (physics) for (const auto& initial : initialBodies_) {
        if (!initial.entity) continue;
        const auto handle = SceneBody2D(world, initial.entity.Id());
        if (!handle.value) continue;
        for (const auto& contact : physics->BodyContacts(handle)) {
            const ContactKey key{contact.a.collider.value, contact.b.collider.value, contact.pointId};
            if (!seen.insert(key).second || contact.normalImpulse <= 0) continue;
            ++points;
            const auto previous = contacts_.find(key);
            const auto old = previous == contacts_.end() ? ContactHistory{} : previous->second;
            const double a = physics->BodyMass(contact.a.body).mass, b = physics->BodyMass(contact.b.body).mass;
            const double inverseMass = (a > 0 ? 1/a : 0) + (b > 0 ? 1/b : 0);
            // Increment above last tick's impulse rejects steady support loads and warm-started stacks.
            const double impulseRise = std::max(0.0, contact.normalImpulse-old.impulse);
            const double impact = impulseRise*inverseMass;
            const auto relative = physics->BodyPointVelocity(contact.a.body, contact.point)
                                - physics->BodyPointVelocity(contact.b.body, contact.point);
            const pxVec2 tangent{-contact.normal.y, contact.normal.x};
            const double slide = std::abs(relative.x*tangent.x + relative.y*tangent.y);
            const auto materialA = physics->Material(contact.a.collider), materialB = physics->Material(contact.b.collider);
            const double friction = std::sqrt(materialA.friction*materialB.friction);
            const double pending = old.pending + (slide > 15 ? slide*delta*friction*0.6 : 0);
            const int impactCount = impact > 60 ? std::clamp(static_cast<int>(std::sqrt(impulseRise*impact)*0.035), 1, 24) : 0;
            const int slidingCount = std::min(32, static_cast<int>(pending));
            next[key] = {contact.normalImpulse, pending-slidingCount};
            const int count = std::min({32, impactCount+slidingCount, 256-emitted});
            if (count <= 0) continue;
            const double radius = pool_.Get<ParticlePhysics2D>().collisionRadius;
            const double offset = radius+physics->CapsuleSlop()*2+0.5;
            // Contact points lie between solids. Probe a bounded set beside the contact edge,
            // so births do not start buried in either box or in a ramp.
            bool free = false; pxPoint birth;
            for (double span : {1.0, 2.0, 4.0}) {
                for (double side : {-1.0, 1.0}) for (double face : {-1.0, 1.0}) {
                    const auto candidate = contact.point + (tangent*(side*span)+contact.normal*face)*offset;
                    if (!free && !physics->OverlapsCircle(candidate, radius)) { birth = candidate; free = true; }
                }
                if (free) break;
            }
            if (!free) continue;
            const auto before = pool_.Get<ParticleSystem>().particles.size();
            Burst(world, birth, count, relative.x*0.25);
            emitted += static_cast<int>(pool_.Get<ParticleSystem>().particles.size()-before);
        }
    }
    contacts_.swap(next);
    telemetry.SetGauge(contactCount_, points); telemetry.SetGauge(emittedCount_, emitted);
}

namespace pixie {
std::unique_ptr<Project> CreateProject() { return std::make_unique<SparksProject>(); }
}
