#include "pixie/runtime/World.hpp"
#include "Parenting2DInternal.hpp"
#include "pixie/runtime/Lighting2D.hpp"
#include <stdexcept>

#include "pixie/runtime/Acceleration2D.hpp"
#include "pixie/runtime/Name.hpp"
#include "pixie/runtime/Parenting2D.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/Velocity2D.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/RigidBody2D.hpp"
#include <cmath>

namespace pixie {

namespace {

std::string ValidatePhysicsValues(const PhysicsSettings2D& settings) {
    if (!std::isfinite(settings.gravity.x) || !std::isfinite(settings.gravity.y) ||
        !std::isfinite(settings.unitsPerMeter) || settings.unitsPerMeter < 0.001 ||
        settings.unitsPerMeter > 1000000 ||
        std::abs(settings.gravity.x) > 1000000 * settings.unitsPerMeter ||
        std::abs(settings.gravity.y) > 1000000 * settings.unitsPerMeter ||
        settings.substeps < 1 || settings.substeps > 32) {
        return "invalid PhysicsSettings2D gravity, scale or substep count";
    }
    return {};
}

std::string ReadGravitySettings(const World& world, PhysicsSettings2D& settings) {
    std::size_t count = 0;
    for (auto [id, authored] : world.Registry().view<const PhysicsSettings2D>().each()) {
        (void)id;
        settings = authored;
        ++count;
    }
    if (count > 1) return "duplicate PhysicsSettings2D authorities";
    return ValidatePhysicsValues(settings);
}

std::string ValidateGravity(const SceneGravity2D& gravity, const PhysicsSettings2D& settings) {
    if (!std::isfinite(gravity.value.x) || !std::isfinite(gravity.value.y) ||
        !std::isfinite(gravity.multiplier) || gravity.multiplier < 0 ||
        std::abs(gravity.value.x) > 1000000 * settings.unitsPerMeter ||
        std::abs(gravity.value.y) > 1000000 * settings.unitsPerMeter) {
        return "invalid Scene gravity values";
    }
    switch (gravity.mode) {
    case SceneGravityMode2D::Inherit: break;
    case SceneGravityMode2D::Multiply: {
        const pxVec2 value = settings.gravity * gravity.multiplier;
        if (!std::isfinite(value.x) || !std::isfinite(value.y) ||
            std::abs(value.x) > 1000000 * settings.unitsPerMeter ||
            std::abs(value.y) > 1000000 * settings.unitsPerMeter) {
            return "Scene gravity multiplication exceeds the physical coordinate limit";
        }
        break;
    }
    case SceneGravityMode2D::Override: break;
    default: return "unknown Scene gravity mode";
    }
    return {};
}

void KeepPreviousOnCreate(entt::registry& registry, entt::entity id) {
    registry.emplace_or_replace<PreviousTransform2D>(id, registry.get<Transform2D>(id));
}

void CopyPreviousTransforms(World& world, const FrameTime&, const Input&) {
    for (auto [id, transform, previous] :
         world.Registry().view<const Transform2D, PreviousTransform2D>().each()) {
        previous.value = transform;
    }
}

void IntegrateVelocities(World& world, const FrameTime& time, const Input&) {
    // Semi-implicit Euler: the velocity first, then the position with the new velocity.
    for (auto [id, velocity, acceleration] :
         world.Registry().view<Velocity2D, const Acceleration2D>(entt::exclude<CharacterMotor2D, RigidBody2D>).each()) {
        velocity.linear += acceleration.linear * time.delta;
    }
    for (auto [id, transform, velocity] :
         world.Registry().view<Transform2D, const Velocity2D>(entt::exclude<CharacterMotor2D, RigidBody2D>).each()) {
        transform.position = transform.position + velocity.linear * time.delta;
        transform.rotation += velocity.angular * time.delta;
    }
}

} // namespace

bool Entity::IsAlive() const {
    return world_ != nullptr && world_->registry_.valid(id_);
}

void Entity::Destroy() const {
    world_->Destroy(*this);
}

World::World(std::uint64_t seed) : random_(seed) {
    stepMetric_ = telemetry_.RegisterTiming("world.step");
    behaviourMetric_ = telemetry_.RegisterTiming("world.behaviours");
    registry_.on_construct<Transform2D>().connect<&KeepPreviousOnCreate>();
    AddSystem(SystemPhase::PreUpdate, "transform.previous", &CopyPreviousTransforms);
    AddSystem(SystemPhase::Physics, "velocity.integrate", &IntegrateVelocities);
}

World::~World() {
    // Behaviours stop first, while the rest of their entity still exists.
    for (const BehaviourClass& behaviour : behaviours_) {
        behaviour.clear(*this);
    }
    registry_.clear();
}

Entity World::Create(std::string_view name) {
    const Entity entity(this, registry_.create());
    if (!name.empty()) {
        entity.Add<Name>(std::string(name));
    }
    return entity;
}

void World::Destroy(Entity entity) {
    if (entity.world_ != this || !registry_.valid(entity.Id())) {
        return;
    }
    if (stepping_) {
        pendingDestroy_.push_back(entity.Id());
    } else {
        detail::RemoveParenting2D(entity);
        membership_.erase(entity.Id());
        registry_.destroy(entity.Id());
    }
}

bool World::CreateScope(ScopeId id, std::string_view name, std::optional<SceneGravity2D> gravity) {
    if (stepping_ || id.value == 0 || scopes_.contains(id)) return false;
    if (gravity && !ValidateScopeGravity(*gravity).empty()) return false;
    scopes_.emplace(id, SceneScope{id, std::string(name), gravity});
    return true;
}

bool World::RenameScope(ScopeId id, std::string_view name) {
    if (stepping_) return false;
    const auto it = scopes_.find(id);
    if (it == scopes_.end()) return false;
    it->second.name = name;
    return true;
}

std::string World::ValidateScopeGravity(const SceneGravity2D& gravity) const {
    PhysicsSettings2D settings;
    if (std::string error = ReadGravitySettings(*this, settings); !error.empty()) return error;
    return ValidateGravity(gravity, settings);
}

std::string World::ValidatePhysicsSettings(const PhysicsSettings2D& settings) const {
    if (std::string error = ValidatePhysicsValues(settings); !error.empty()) return error;
    for (const auto& [id, scope] : scopes_) {
        (void)id;
        if (scope.gravity) {
            if (std::string error = ValidateGravity(*scope.gravity, settings); !error.empty()) return error;
        }
    }
    return {};
}

bool World::SetPresentation(std::optional<WorldPresentation2D> value) {
    if (stepping_ || (value && !ValidatePresentation2D(*value).empty())) return false;
    if (value && !registry_.view<const LightingSettings2D>().empty()) return false;
    presentation_ = std::move(value);
    return true;
}
bool World::SetScopeEnvironment(ScopeId id, std::optional<SceneEnvironment2D> value) {
    const auto found = scopes_.find(id);
    if (stepping_ || found == scopes_.end() || (value && !ValidateEnvironment2D(value->value).empty())) return false;
    found->second.environment = std::move(value);
    return true;
}
bool World::SetScopeCamera(ScopeId id, std::uint64_t entityId) {
    const auto found = scopes_.find(id);
    if (stepping_ || found == scopes_.end()) return false;
    found->second.camera = entityId;
    return true;
}

bool World::SetScopeGravity(ScopeId id, SceneGravity2D gravity) {
    if (stepping_) return false;
    const auto it = scopes_.find(id);
    if (it == scopes_.end()) return false;
    if (!ValidateScopeGravity(gravity).empty()) return false;
    it->second.gravity = gravity;
    return true;
}

GravityResolution2D World::ResolveGravity(ScopeId id) const {
    GravityResolution2D result;
    result.scope = id;
    PhysicsSettings2D settings;
    result.error = ReadGravitySettings(*this, settings);
    if (!result.error.empty()) return result;
    return ResolveGravityWithSettings(id, settings);
}

GravityResolution2D World::ResolveGravityWithSettings(ScopeId id, const PhysicsSettings2D& settings) const {
    GravityResolution2D result;
    result.scope = id;
    if (id.value == 0) {
        result.value = settings.gravity;
        return result;
    }
    const auto it = scopes_.find(id);
    if (it == scopes_.end()) {
        result.error = "unknown Scene scope";
        return result;
    }
    if (!it->second.gravity) {
        result.value = settings.gravity;
        return result;
    }
    const SceneGravity2D& gravity = *it->second.gravity;
    result.mode = gravity.mode;
    result.error = ValidateGravity(gravity, settings);
    if (!result.error.empty()) return result;
    switch (gravity.mode) {
    case SceneGravityMode2D::Inherit: result.value = settings.gravity; break;
    case SceneGravityMode2D::Multiply: result.value = settings.gravity * gravity.multiplier; break;
    case SceneGravityMode2D::Override: result.value = gravity.value; break;
    default: result.error = "unknown Scene gravity mode"; break;
    }
    return result;
}

GravityResolution2D World::ResolveGravity(Entity entity) const {
    if (entity.world_ != this || !registry_.valid(entity.Id())) {
        GravityResolution2D result;
        result.error = "invalid or foreign entity";
        return result;
    }
    return ResolveGravity(ScopeOf(entity).value());
}

GravityResolution2D World::ResolveGravity(Entity entity, pxVec2 effectiveWorldGravity) const {
    GravityResolution2D result;
    if (entity.world_ != this || !registry_.valid(entity.Id())) {
        result.error = "invalid or foreign entity";
        return result;
    }
    result.scope = ScopeOf(entity).value();
    PhysicsSettings2D settings;
    result.error = ReadGravitySettings(*this, settings);
    if (!result.error.empty()) return result;
    settings.gravity = effectiveWorldGravity;
    result.error = ValidatePhysicsValues(settings);
    if (!result.error.empty()) return result;
    return ResolveGravityWithSettings(result.scope, settings);
}

bool World::RemoveScope(ScopeId id) {
    if (stepping_ || !scopes_.contains(id)) return false;
    for (const auto& [entity, scope] : membership_) {
        if (scope == id && registry_.valid(entity)) return false;
    }
    scopes_.erase(id);
    return true;
}

std::vector<SceneScope> World::Scopes() const {
    std::vector<SceneScope> result;
    result.reserve(scopes_.size());
    for (const auto& [id, scope] : scopes_) result.push_back(scope);
    return result;
}

bool World::SetScope(Entity entity, ScopeId id) {
    if (entity.world_ != this || !registry_.valid(entity.Id()) ||
        (id.value != 0 && !scopes_.contains(id))) return false;
    if (ScopeOf(entity) != id && (Parent2D(entity) || !Children2D(entity).empty())) return false;
    if (stepping_) {
        const entt::entity entityId = entity.Id();
        deferred_.push_back([entityId, id](World& world) {
            world.SetScope(world.Wrap(entityId), id);
        });
    } else if (id.value == 0) {
        membership_.erase(entity.Id());
    } else {
        membership_[entity.Id()] = id;
    }
    return true;
}

std::optional<ScopeId> World::ScopeOf(Entity entity) const {
    if (entity.world_ != this || !registry_.valid(entity.Id())) return std::nullopt;
    const auto it = membership_.find(entity.Id());
    return it == membership_.end() ? ScopeId{} : it->second;
}

void World::Defer(std::function<void(World&)> action) {
    if (stepping_) {
        deferred_.push_back(std::move(action));
    } else {
        action(*this);
    }
}

void World::AddSystem(SystemPhase phase, std::string name, System system) {
    auto& systems = phase == SystemPhase::PreUpdate ? preUpdate_ : phase == SystemPhase::Physics ? physics_ : postUpdate_;
    const auto timing = telemetry_.RegisterTiming((phase == SystemPhase::PreUpdate ? "system.pre." : phase == SystemPhase::Physics ? "system.physics." : "system.post.") + name);
    systems.push_back({std::move(name), std::move(system), timing});
}

void World::RunSystems(SystemPhase phase, const FrameTime& time, const Input& input) {
    for (const SystemEntry& system : phase == SystemPhase::PreUpdate ? preUpdate_ : phase == SystemPhase::Physics ? physics_ : postUpdate_) {
        const TelemetryScope measure(telemetry_,system.timing);
        system.run(*this, time, input);
    }
}

void World::Step(double delta, const Input& input) {
    if (advancing_) throw std::logic_error("World::Step cannot be reentered, including from deferred actions.");
    advancing_ = true;
    struct RestoreAdvance { bool& flag; ~RestoreAdvance() { flag = false; } } advancing{advancing_};
    const TelemetryScope measure(telemetry_,stepMetric_);
    const FrameTime time{delta, 0.0, elapsed_, steps_};
    stepping_ = true;
    struct RestoreStepping { bool& flag; ~RestoreStepping() { flag = false; } } restore{stepping_};
    detail::RefreshParenting2D(*this);
    detail::CaptureParenting2D(*this);
    RunSystems(SystemPhase::PreUpdate, time, input);
    // A Behaviour class added during the step is updated from the next step on.
    const std::size_t classes = behaviours_.size();
    {
        const TelemetryScope behaviours(telemetry_,behaviourMetric_);
        for (std::size_t i = 0; i < classes; ++i) behaviours_[i].update(*this, time, input);
    }
    detail::RefreshParenting2D(*this);
    RunSystems(SystemPhase::Physics, time, input);
    detail::RefreshParenting2D(*this);
    RunSystems(SystemPhase::PostUpdate, time, input);
    stepping_ = false;
    FinishStep();
    detail::RefreshParenting2D(*this);
    ++steps_;
    elapsed_ += delta;
}

void World::FinishStep() {
    // Destructions first, then deferred actions; an action may itself destroy or defer, and then
    // runs immediately, since the step is over.
    for (const entt::entity id : pendingDestroy_) {
        Destroy(Wrap(id));
    }
    pendingDestroy_.clear();
    auto actions = std::move(deferred_);
    deferred_.clear();
    for (auto& action : actions) {
        action(*this);
    }
}

void World::ResetInterpolation(Entity entity) {
    if (entity.world_ != this || !entity.IsAlive()) return;
    detail::RefreshParenting2D(*this);
    if (auto* transform = registry_.try_get<Transform2D>(entity.Id())) {
        registry_.emplace_or_replace<PreviousTransform2D>(entity.Id(), *transform);
        detail::ResetBranchInterpolation2D(entity);
    }
}

std::size_t World::EntityCount() const {
    return registry_.view<entt::entity>().size();
}

} // namespace pixie
