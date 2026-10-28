#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/World.hpp"
#include "pixie/core/Log.hpp"
#include <algorithm>
#include <cmath>
#include <format>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>

namespace pixie {
namespace {
// The controller settings of a motor under the World's (downward or zero) gravity.
constexpr CharacterSettings2D MotorSettings(const CharacterMotor2D& motor, double worldGravity) {
    CharacterSettings2D settings;
    settings.radius = motor.radius;
    settings.halfSegment = motor.halfSegment;
    settings.maxSpeed = motor.maxSpeed;
    settings.acceleration = motor.acceleration;
    settings.braking = motor.braking;
    settings.gravity = worldGravity * motor.gravityScale;
    settings.terminalSpeed = motor.terminalSpeed;
    settings.jumpSpeed = motor.jumpSpeed;
    settings.maxSlopeDegrees = motor.maxSlopeDegrees;
    settings.coyoteSeconds = motor.coyoteSeconds;
    settings.jumpBufferSeconds = motor.jumpBufferSeconds;
    settings.effectiveMass = motor.effectiveMass;
    settings.maxPushForce = motor.maxPushForce;
    settings.maxPushSpeed = motor.maxPushSpeed;
    settings.maxSupportForce = motor.maxSupportForce;
    settings.maxInheritedSpeed = motor.maxInheritedSpeed;
    settings.maxCarryStep = motor.maxCarryStep;
    settings.maxRecoveryDistance = motor.maxRecoveryDistance;
    return settings;
}
// A default motor under the default world gravity, PhysicsSettings2D's (0, 1100), is the default
// standalone controller.
static_assert(MotorSettings(CharacterMotor2D{}, 1100) == CharacterSettings2D{});
struct StaticRecord {
    Collider2D authored;
    Transform2D transform;
    ColliderHandle2D handle;
};
struct RigidRecord {
    Collider2D collider;
    std::optional<CompoundCollider2D> compound;
    RigidBody2D authored;
    Transform2D transform;
    BodyHandle2D handle;
};
struct MovementMetrics {
    Telemetry& telemetry;
    MetricHandle sync, rebuild, publish, motors, prepare, events, rebuilds, syncCalls, published, motorCount, pendingCommands;
    std::uint64_t rebuilt = 0, synchronized = 0;
    explicit MovementMetrics(Telemetry& t) : telemetry(t),
        sync(t.RegisterTiming("physics.scene.sync")), rebuild(t.RegisterTiming("physics.scene.rebuild")),
        publish(t.RegisterTiming("physics.scene.publish")), motors(t.RegisterTiming("physics.scene.motors")),
        prepare(t.RegisterTiming("physics.scene.coupling.prepare")),
        events(t.RegisterTiming("physics.scene.events.publish")), rebuilds(t.RegisterGauge("physics.scene.rebuilds", "objects/step")),
        syncCalls(t.RegisterGauge("physics.scene.sync_calls", "calls/step")),
        published(t.RegisterGauge("physics.scene.bodies_published", "bodies/step")),
        motorCount(t.RegisterGauge("physics.scene.motor_count", "motors")),
        pendingCommands(t.RegisterGauge("physics.scene.pending_commands", "commands")) {}
    void BeginStep() {
        rebuilt = 0; synchronized = 0;
        telemetry.SetGauge(rebuilds, 0); telemetry.SetGauge(syncCalls, 0); telemetry.SetGauge(published, 0);
        // Body command APIs apply immediately; the engine has no deferred command queue.
        telemetry.SetGauge(pendingCommands, 0);
    }
    void Rebuilt() { ++rebuilt; telemetry.SetGauge(rebuilds, static_cast<double>(rebuilt)); }
};
struct MovementState {
    explicit MovementState(Telemetry& telemetry) : metrics(telemetry) {}
    MovementMetrics metrics;
    bool installed = false;
    std::unique_ptr<CollisionWorld2D> collision;
    std::map<entt::entity, StaticRecord> colliders;
    std::map<entt::entity, RigidRecord> bodies;
    std::map<std::uint64_t, entt::entity> owners;
    std::vector<std::uint64_t> retired;
    std::vector<ScenePhysicsEvent2D> events, pendingResetEvents;
    PhysicsSettings2D settings;
    bool validSettings = true;
    bool hasStepped = false;
    std::map<entt::entity, CharacterController2D> characters;
    std::vector<entt::entity> preparedCharacters;
    std::vector<entt::entity> activeColliders, activeCharacters, activeBodies;
    std::vector<std::string> errors, lastLogged;
};
MovementState& State(World& world) {
    auto& context = world.Registry().ctx();
    if (!context.contains<MovementState>()) context.emplace<MovementState>(world.GetTelemetry());
    return context.get<MovementState>();
}
bool SameTransform(const Transform2D& a, const Transform2D& b) {
    return a.position == b.position && a.rotation == b.rotation && a.scale == b.scale;
}
void ValidateTransform(const Transform2D& t) {
    if (!std::isfinite(t.position.x) || !std::isfinite(t.position.y) || !std::isfinite(t.rotation)
        || !std::isfinite(t.scale.x) || !std::isfinite(t.scale.y) || t.scale.x == 0 || t.scale.y == 0)
        throw std::invalid_argument("Transform2D requires finite placement and nonzero scale");
}
void Error(MovementState& state, entt::entity id, const std::string& message) {
    state.errors.push_back(std::format("entity {}: {}", entt::to_integral(id), message));
}
void CopyEvents(MovementState& state, std::span<const PhysicsEvent2D> source,
                std::vector<ScenePhysicsEvent2D>& destination) {
    for (const auto& event : source) {
        const auto a = state.owners.find(event.a.collider.value), b = state.owners.find(event.b.collider.value);
        destination.push_back({event, a == state.owners.end() ? entt::null : a->second,
                                     b == state.owners.end() ? entt::null : b->second});
    }
}
void ResetCollision(MovementState& state) {
    const TelemetryScope measure(state.metrics.telemetry, state.metrics.rebuild);
    if (state.collision) {
        state.collision->Reset();
        CopyEvents(state, state.collision->Events(), state.pendingResetEvents);
    }
    state.bodies.clear(); state.colliders.clear(); state.characters.clear();
    state.owners.clear(); state.retired.clear(); state.collision.reset();
    state.metrics.telemetry.SetGauge(state.metrics.motorCount, 0);
}
PhysicsEndReason2D RemovalReason(World& world, entt::entity entity) {
    if (!world.Registry().valid(entity)) return PhysicsEndReason2D::Removed;
    const auto* collider = world.Registry().try_get<Collider2D>(entity);
    if (collider && !collider->enabled) return PhysicsEndReason2D::Disabled;
    if (const auto* compound = world.Registry().try_get<CompoundCollider2D>(entity)) {
        if (!compound->enabled || std::none_of(compound->shapes.begin(), compound->shapes.end(), [](const auto& s) { return s.value.enabled; }))
            return PhysicsEndReason2D::Disabled;
        return PhysicsEndReason2D::Reconfigured;
    }
    return collider ? PhysicsEndReason2D::Reconfigured : PhysicsEndReason2D::Removed;
}
PhysicsEndReason2D ReplacementReason(const Collider2D& old, const Collider2D& next) {
    return old.category != next.category || old.mask != next.mask
        ? PhysicsEndReason2D::FilterChanged : PhysicsEndReason2D::Reconfigured;
}
void PublishErrors(MovementState& state) {
    if (state.errors != state.lastLogged) {
        for (const auto& error : state.errors) LogWarning("Scene movement: " + error);
        state.lastLogged = state.errors;
    }
}
BodyShape2D RigidShape(const Collider2D& collider, const Transform2D& t) {
    ValidateTransform(t);
    if (collider.category <= 0 || collider.mask < 0) throw std::invalid_argument("invalid collision category/mask");
    BodyShape2D shape;
    shape.offset = {collider.offset.x * t.scale.x, collider.offset.y * t.scale.y};
    shape.category = static_cast<std::uint64_t>(collider.category);
    shape.mask = static_cast<std::uint64_t>(collider.mask);
    if (!std::isfinite(collider.rotation)) throw std::invalid_argument("collider local rotation must be finite");
    const double c = std::cos(collider.rotation), s = std::sin(collider.rotation);
    const auto local = [&](pxPoint p) { return pxPoint{(c * p.x - s * p.y) * t.scale.x, (s * p.x + c * p.y) * t.scale.y}; };
    switch (collider.kind) {
    case ColliderKind2D::Box:
        shape.halfSize = {collider.halfSize.x * std::abs(t.scale.x), collider.halfSize.y * std::abs(t.scale.y)};
        if (collider.rotation != 0) {
            if (collider.halfSize.x <= 0 || collider.halfSize.y <= 0) throw std::invalid_argument("box half size must be positive");
            shape.kind = BodyShapeKind2D::Polygon;
            for (auto p : {pxPoint{-collider.halfSize.x, -collider.halfSize.y}, pxPoint{collider.halfSize.x, -collider.halfSize.y},
                           pxPoint{collider.halfSize.x, collider.halfSize.y}, pxPoint{-collider.halfSize.x, collider.halfSize.y}})
                shape.points.push_back(local(p));
        }
        break;
    case ColliderKind2D::Circle:
        if (std::abs(std::abs(t.scale.x) - std::abs(t.scale.y)) > 1e-9)
            throw std::invalid_argument("circle colliders require uniform absolute scale");
        shape.kind = BodyShapeKind2D::Circle;
        shape.radius = collider.radius * std::abs(t.scale.x);
        break;
    case ColliderKind2D::Polygon:
        shape.kind = BodyShapeKind2D::Polygon;
        for (auto point : collider.points) shape.points.push_back(local(point));
        break;
    default: throw std::invalid_argument("unknown Collider2D kind");
    }
    shape.sensor = collider.sensor;
    shape.density = collider.density;
    shape.friction = collider.friction;
    shape.restitution = collider.restitution;
    return shape;
}
ColliderHandle2D CreateCollider(CollisionWorld2D& collision, const Collider2D& data, const Transform2D& transform) {
    const auto shape = RigidShape(data, transform);
    RigidBody2D definition;
    definition.type = BodyType2D::Static;
    return collision.BodyColliders(collision.AddBody(transform.position, transform.rotation, definition, {&shape, 1})).front();
}
void MoveCharacters(World& world, const FrameTime& time, const Input& input) {
    auto& state = State(world);
    state.events.clear();
    SyncSceneMovement2D(world);
    state.events.swap(state.pendingResetEvents);
    state.pendingResetEvents.clear();
    if (!state.collision || !state.validSettings) { PublishErrors(state); return; }
    state.hasStepped = true;
    state.preparedCharacters.clear();
    if (!state.characters.empty()) {
        const TelemetryScope measure(state.metrics.telemetry, state.metrics.prepare);
        for (auto& [id, controller] : state.characters) {
            auto* command = world.Registry().try_get<CharacterCommand2D>(id);
            double horizontal = command ? command->horizontal : 0;
            bool jump = command && command->jumpPressed;
            if (const auto* keyboard = world.Registry().try_get<KeyboardControl2D>(id); keyboard && keyboard->enabled) {
                horizontal += (input.IsDown(Key::D) || input.IsDown(Key::Right) ? 1.0 : 0.0)
                            - (input.IsDown(Key::A) || input.IsDown(Key::Left) ? 1.0 : 0.0);
                jump = jump || input.WasPressed(Key::Space);
            }
            try {
                controller.PrepareStep(*state.collision, horizontal, jump, time.delta);
                state.preparedCharacters.push_back(id);
            } catch (const std::exception& error) { Error(state, id, error.what()); }
            if (command) command->jumpPressed = false;
        }
    }
    try {
        if (state.collision->NeedsSimulation()) {
            state.collision->StepBodies(time.delta, state.settings.substeps);
            const TelemetryScope measure(state.metrics.telemetry, state.metrics.events);
            CopyEvents(state, state.collision->Events(), state.events);
        }
    } catch (const std::exception& error) {
        for (auto id : state.preparedCharacters) {
            auto& controller = state.characters.at(id);
            controller.Teleport(controller.State().position); // Cancel the pending phase after a failed world step.
        }
        state.errors.push_back(error.what()); PublishErrors(state); return;
    }
    {
        const TelemetryScope measure(state.metrics.telemetry, state.metrics.motors);
        for (auto id : state.preparedCharacters) {
            auto& controller = state.characters.at(id);
            try {
                controller.FinishStep(*state.collision);
                world.Registry().get<Transform2D>(id).position = controller.State().position;
                if (controller.Coupling().blocked == CharacterBlocked2D::SupportStepLimit)
                    Error(state, id, "character support displacement exceeds maxCarryStep; last completed pose retained");
                else if (controller.Coupling().unsafePlacement)
                    Error(state, id, "character coupling could not establish a safe placement; last completed pose retained");
                else if (controller.Coupling().blocked != CharacterBlocked2D::None)
                    Error(state, id, "character support movement was blocked");
            } catch (const std::exception& error) { Error(state, id, error.what()); }
        }
    }
    {
        const TelemetryScope measure(state.metrics.telemetry, state.metrics.publish);
        for (auto retired : state.retired) state.owners.erase(retired);
        state.retired.clear();
        for (auto& [id, record] : state.bodies) {
            const auto motion = state.collision->BodyMotion(record.handle);
            auto& transform = world.Registry().get<Transform2D>(id);
            transform.position = motion.position;
            transform.rotation = motion.rotation;
            record.transform = transform; // The solved pose is published once; the next sync must not teleport it back.
        }
        state.metrics.telemetry.SetGauge(state.metrics.published, static_cast<double>(state.bodies.size()));
    }
    PublishErrors(state);
}

}

void InstallSceneMovement2D(World& world) {
    auto& state = State(world);
    if (state.installed) return;
    state.installed = true;
    world.AddSystem(SystemPhase::PreUpdate, "scene.collision.sync",
        [](World& w, const FrameTime&, const Input&) { State(w).metrics.BeginStep(); SyncSceneMovement2D(w); });
    world.AddSystem(SystemPhase::Physics, "scene.character.move", &MoveCharacters);
}

void SyncSceneMovement2D(World& world) {
    auto& state = State(world);
    if (!world.IsStepping()) state.metrics.BeginStep();
    const TelemetryScope measure(state.metrics.telemetry, state.metrics.sync);
    ++state.metrics.synchronized;
    state.metrics.telemetry.SetGauge(state.metrics.syncCalls, static_cast<double>(state.metrics.synchronized));
    auto& registry = world.Registry();
    state.errors.clear();
    auto colliders = registry.view<const Collider2D>();
    auto characters = registry.view<const CharacterMotor2D>();
    auto rigidBodies = registry.view<const RigidBody2D>();
    auto compounds = registry.view<const CompoundCollider2D>();
    PhysicsSettings2D settings;
    std::size_t settingsCount = 0;
    for (auto [id, authored] : registry.view<const PhysicsSettings2D>().each()) {
        (void)id;
        settings = authored;
        ++settingsCount;
    }
    state.validSettings = true;
    try {
        if (settingsCount > 1) throw std::invalid_argument("only one PhysicsSettings2D entity is allowed per World");
        if (!std::isfinite(settings.gravity.x) || !std::isfinite(settings.gravity.y)
            || std::abs(settings.gravity.x / settings.unitsPerMeter) > 1000000
            || std::abs(settings.gravity.y / settings.unitsPerMeter) > 1000000
            || !std::isfinite(settings.unitsPerMeter) || settings.unitsPerMeter < 0.001
            || settings.unitsPerMeter > 1000000 || settings.substeps < 1 || settings.substeps > 32)
            throw std::invalid_argument("invalid PhysicsSettings2D gravity, scale or substep count");
        if (settings.unitsPerMeter != state.settings.unitsPerMeter) {
            if (state.hasStepped) throw std::invalid_argument("changing unitsPerMeter requires stopping Play and resetting");
            ResetCollision(state);
        }
        state.settings = settings;
    } catch (const std::exception& error) {
        state.validSettings = false;
        ResetCollision(state);
        state.errors.push_back(error.what());
        if (!world.IsStepping()) PublishErrors(state);
        return;
    }
    if (!state.collision && (colliders.begin() != colliders.end() || characters.begin() != characters.end() || rigidBodies.begin() != rigidBodies.end() || compounds.begin() != compounds.end())) {
        const TelemetryScope rebuild(state.metrics.telemetry, state.metrics.rebuild);
        state.collision = std::make_unique<CollisionWorld2D>(settings.unitsPerMeter, &world.GetTelemetry());
    }
    if (!state.collision) return;
    state.collision->SetGravity(settings.gravity);
    auto& activeColliders = state.activeColliders;
    auto& activeCharacters = state.activeCharacters;
    auto& activeBodies = state.activeBodies;
    activeColliders.clear(); activeCharacters.clear(); activeBodies.clear();
    for (auto [id, authored] : colliders.each()) {
        if (!authored.enabled || registry.all_of<RigidBody2D>(id)) continue;
        try {
            if (registry.all_of<CharacterMotor2D>(id))
                throw std::invalid_argument("CharacterMotor2D owns its capsule; remove the static Collider2D");
            if (registry.all_of<CompoundCollider2D>(id)) throw std::invalid_argument("Collider2D and CompoundCollider2D cannot own the same geometry");
            const auto* transform = registry.try_get<Transform2D>(id);
            if (!transform) throw std::invalid_argument("Collider2D requires Transform2D");
            auto old = state.colliders.find(id);
            if (old == state.colliders.end() || old->second.authored != authored
                || !SameTransform(old->second.transform, *transform)) {
                // Copy authored data before allocating a backend body. Existing body remains valid until replacement.
                const TelemetryScope rebuild(state.metrics.telemetry, state.metrics.rebuild);
                StaticRecord next{authored, *transform, {}};
                auto [entry, inserted] = state.colliders.try_emplace(id);
                (void)inserted;
                next.handle = CreateCollider(*state.collision, authored, *transform);
                state.owners[next.handle.value] = id;
                if (entry->second.handle.value) {
                    state.retired.push_back(entry->second.handle.value);
                    state.collision->Remove(entry->second.handle, ReplacementReason(entry->second.authored, authored));
                }
                entry->second = std::move(next);
                state.metrics.Rebuilt();
            }
            activeColliders.push_back(id);
        } catch (const std::exception& error) { Error(state, id, error.what()); }
    }
    std::sort(activeColliders.begin(), activeColliders.end());
    for (auto it = state.colliders.begin(); it != state.colliders.end();) {
        if (!std::binary_search(activeColliders.begin(), activeColliders.end(), it->first)) {
            state.retired.push_back(it->second.handle.value);
            state.collision->Remove(it->second.handle, RemovalReason(world, it->first));
            it = state.colliders.erase(it);
        } else ++it;
    }
    auto syncBody = [&](entt::entity id, const RigidBody2D* rigid) {
        try {
            if (registry.all_of<CharacterMotor2D>(id))
                throw std::invalid_argument("RigidBody2D/CompoundCollider2D and CharacterMotor2D cannot own the same entity");
            const auto* transform = registry.try_get<Transform2D>(id);
            const auto* collider = registry.try_get<Collider2D>(id);
            const auto* compound = registry.try_get<CompoundCollider2D>(id);
            if (!transform || (!collider && !compound)) throw std::invalid_argument("RigidBody2D requires Transform2D and Collider2D or CompoundCollider2D");
            if (collider && compound) throw std::invalid_argument("Collider2D and CompoundCollider2D cannot own the same geometry");
            ValidateTransform(*transform);
            if ((collider && !collider->enabled) || (compound && !compound->enabled)) return;
            if (compound && compound->shapes.size() > 256) throw std::invalid_argument("compound exceeds 256 shapes");
            if (compound && std::none_of(compound->shapes.begin(), compound->shapes.end(), [](const auto& s) { return s.value.enabled; })) return;
            RigidBody2D authored;
            authored.type = BodyType2D::Static;
            if (rigid) authored = *rigid;
            auto old = state.bodies.find(id);
            const bool sameGeometry = old != state.bodies.end() && (compound
                ? old->second.compound && *old->second.compound == *compound
                : !old->second.compound && old->second.collider == *collider);
            if (!sameGeometry || old->second.authored != authored || old->second.transform.scale != transform->scale) {
                const TelemetryScope rebuild(state.metrics.telemetry, state.metrics.rebuild);
                std::vector<BodyShape2D> shapes;
                if (compound) {
                    for (std::size_t i = 0; i < compound->shapes.size(); ++i) {
                        if (!compound->shapes[i].value.enabled) continue;
                        try { shapes.push_back(RigidShape(compound->shapes[i].value, *transform)); }
                        catch (const std::exception& error) { throw std::invalid_argument(std::format("shape {}: {}", i + 1, error.what())); }
                    }
                } else shapes.push_back(RigidShape(*collider, *transform));
                auto definition = authored;
                if (old != state.bodies.end()) {
                    const auto motion = state.collision->BodyMotion(old->second.handle);
                    definition.initialVelocity = motion.velocity;
                    definition.initialAngularVelocity = motion.angularVelocity;
                }
                RigidRecord next;
                if (collider) next.collider = *collider;
                if (compound) next.compound = *compound;
                next.authored = authored;
                next.transform = *transform;
                next.handle = state.collision->AddBody(transform->position, transform->rotation, definition, shapes);
                for (auto handle : state.collision->BodyColliders(next.handle)) state.owners[handle.value] = id;
                auto [entry, inserted] = state.bodies.try_emplace(id);
                (void)inserted;
                if (entry->second.handle.value) {
                    for (auto handle : state.collision->BodyColliders(entry->second.handle)) state.retired.push_back(handle.value);
                    auto reason = collider && !entry->second.compound
                        ? ReplacementReason(entry->second.collider, *collider) : PhysicsEndReason2D::Reconfigured;
                    if (compound && entry->second.compound && compound->shapes.size() == entry->second.compound->shapes.size())
                        for (std::size_t i = 0; i < compound->shapes.size(); ++i)
                            if (ReplacementReason(entry->second.compound->shapes[i].value, compound->shapes[i].value) == PhysicsEndReason2D::FilterChanged)
                                reason = PhysicsEndReason2D::FilterChanged;
                    state.collision->RemoveBody(entry->second.handle, reason);
                }
                entry->second = std::move(next);
                state.metrics.Rebuilt();
            } else if (!SameTransform(old->second.transform, *transform)) {
                state.collision->TeleportBody(old->second.handle, transform->position, transform->rotation);
                old->second.transform = *transform;
                world.ResetInterpolation(world.Wrap(id));
            }
            activeBodies.push_back(id);
        } catch (const std::exception& error) { Error(state, id, error.what()); }
    };
    for (auto [id, authored] : rigidBodies.each()) syncBody(id, &authored);
    for (auto [id, compound] : compounds.each()) {
        (void)compound;
        if (!registry.all_of<RigidBody2D>(id)) syncBody(id, nullptr);
    }
    std::sort(activeBodies.begin(), activeBodies.end());
    for (auto it = state.bodies.begin(); it != state.bodies.end();) {
        if (!std::binary_search(activeBodies.begin(), activeBodies.end(), it->first)) {
            if (state.collision->ContainsBody(it->second.handle)) {
                for (auto colliderHandle : state.collision->BodyColliders(it->second.handle)) state.retired.push_back(colliderHandle.value);
                state.collision->RemoveBody(it->second.handle, RemovalReason(world, it->first));
            }
            it = state.bodies.erase(it);
        } else ++it;
    }
    for (auto [id, motor] : characters.each()) {
        try {
            if (registry.all_of<RigidBody2D>(id))
                throw std::invalid_argument("RigidBody2D and CharacterMotor2D cannot own the same entity");
            if (registry.all_of<CompoundCollider2D>(id)) throw std::invalid_argument("CharacterMotor2D owns its capsule; remove CompoundCollider2D");
            state.collision->ValidateMoverCapsule(motor.radius, motor.halfSegment);
            if (state.settings.gravity.x != 0 || state.settings.gravity.y < 0)
                throw std::invalid_argument("upright CharacterMotor2D requires downward or zero world gravity");
            if (!std::isfinite(motor.gravityScale) || motor.gravityScale < 0)
                throw std::invalid_argument("CharacterMotor2D gravity scale must be finite and zero or more");
            const auto motorSettings = MotorSettings(motor, state.settings.gravity.y);
            const auto* transform = registry.try_get<Transform2D>(id);
            if (!transform) throw std::invalid_argument("CharacterMotor2D requires Transform2D");
            ValidateTransform(*transform);
            if (transform->rotation != 0 || transform->scale != pxVec2(1, 1))
                throw std::invalid_argument("CharacterMotor2D requires upright, unit-scale Transform2D; edit radius/halfSegment instead");
            auto it = state.characters.find(id);
            if (it == state.characters.end()) {
                const TelemetryScope rebuild(state.metrics.telemetry, state.metrics.rebuild);
                it = state.characters.try_emplace(id, transform->position, motorSettings).first;
                state.metrics.Rebuilt();
            } else {
                if (it->second.Settings() != motorSettings) it->second.Configure(motorSettings);
                if (it->second.State().position != transform->position) {
                    it->second.Teleport(transform->position);
                    world.ResetInterpolation(world.Wrap(id));
                }
            }
            activeCharacters.push_back(id);
        } catch (const std::exception& error) { Error(state, id, error.what()); }
    }
    std::sort(activeCharacters.begin(), activeCharacters.end());
    std::erase_if(state.characters, [&](const auto& entry) { return !std::binary_search(activeCharacters.begin(), activeCharacters.end(), entry.first); });
    state.metrics.telemetry.SetGauge(state.metrics.motorCount, static_cast<double>(state.characters.size()));
    // During a step, publish once after movement has added any execution errors.
    // Clearing the sync report must not make an unchanged runtime error log every frame.
    if (!world.IsStepping()) PublishErrors(state);
}
CollisionLayerNames2D DescribeCollisionLayers2D(const World& world) {
    CollisionLayerNames2D result;
    for (int i = 0; i < 31; ++i) result.names[i] = "Unnamed layer " + std::to_string(i + 1);
    const auto view = world.Registry().view<const CollisionLayers2D>();
    if (view.begin() == view.end()) { result.names[0] = "Default"; return result; }
    if (std::next(view.begin()) != view.end()) {
        result.errors.push_back("Only one CollisionLayers2D is allowed. Remove the duplicate configuration; filters are unchanged.");
        return result;
    }
    const auto& layers = view.get<const CollisionLayers2D>(*view.begin()).layers;
    if (layers.size() > FieldDescriptor::maxRecords)
        result.errors.push_back("CollisionLayers2D exceeds the 256-record scene limit.");
    std::array<int, 31> counts{};
    for (const auto& record : layers) {
        const auto& layer = record.value;
        if (layer.slot < 1 || layer.slot > 31) {
            result.errors.push_back("CollisionLayers2D slot must be between 1 and 31; filters are unchanged.");
            continue;
        }
        const int index = layer.slot - 1;
        if (++counts[index] > 1) {
            result.names[index] = "Unnamed layer " + std::to_string(layer.slot);
            if (counts[index] == 2) result.errors.push_back("Duplicate collision layer slot " + std::to_string(layer.slot) + "; remove the duplicate name record.");
        } else if (!layer.name.empty()) result.names[index] = layer.name;
    }
    return result;
}
std::vector<std::string> SceneMovementErrors2D(World& world) {
    auto errors = State(world).errors;
    const auto layers = DescribeCollisionLayers2D(world);
    errors.insert(errors.end(), layers.errors.begin(), layers.errors.end());
    return errors;
}
const CollisionWorld2D* SceneCollisionWorld2D(World& world) { return State(world).validSettings ? State(world).collision.get() : nullptr; }
BodyHandle2D SceneBody2D(World& world, entt::entity entity) {
    const auto& bodies = State(world).bodies;
    const auto it = bodies.find(entity);
    return it == bodies.end() ? BodyHandle2D{} : it->second.handle;
}
CollisionWorld2D* ScenePhysicsWorld2D(World& world) { return State(world).validSettings ? State(world).collision.get() : nullptr; }
entt::entity SceneColliderEntity2D(World& world, ColliderHandle2D collider) {
    auto& state = State(world);
    if (!state.collision || !state.collision->Contains(collider)) return entt::null;
    const auto owner = state.owners.find(collider.value);
    return owner == state.owners.end() ? entt::null : owner->second;
}
std::span<const ScenePhysicsEvent2D> ScenePhysicsEvents2D(World& world) { return State(world).events; }
const CapsuleMotion2D* CharacterMotion2D(World& world, entt::entity entity) {
    const auto& characters = State(world).characters;
    const auto found = characters.find(entity);
    return found == characters.end() ? nullptr : &found->second.State();
}
void ResetSceneMovement2D(World& world) {
    if (world.IsStepping()) throw std::logic_error("reset scene movement outside a World step");
    auto& state = State(world);
    ResetCollision(state);
    state.hasStepped = false;
    state.events.clear();
    SyncSceneMovement2D(world);
    for (auto [id, transform] : world.Registry().view<const Transform2D>().each()) {
        (void)transform;
        if (world.Registry().any_of<RigidBody2D, CharacterMotor2D>(id)) world.ResetInterpolation(world.Wrap(id));
    }
}
const CharacterSettings2D* CharacterTuning2D(World& world, entt::entity entity) {
    const auto& characters = State(world).characters;
    const auto found = characters.find(entity);
    return found == characters.end() ? nullptr : &found->second.Settings();
}
const CharacterCouplingState2D* CharacterCoupling2D(World& world, entt::entity entity) {
    const auto& characters = State(world).characters;
    const auto found = characters.find(entity);
    return found == characters.end() ? nullptr : &found->second.Coupling();
}

}
