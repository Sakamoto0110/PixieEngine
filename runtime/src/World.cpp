#include "pixie/runtime/World.hpp"

#include "pixie/runtime/Acceleration2D.hpp"
#include "pixie/runtime/Name.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/Velocity2D.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"

namespace pixie {

namespace {

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
    AddSystem(SystemPhase::PostUpdate, "velocity.integrate", &IntegrateVelocities);
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
    if (!registry_.valid(entity.Id())) {
        return;
    }
    if (stepping_) {
        pendingDestroy_.push_back(entity.Id());
    } else {
        registry_.destroy(entity.Id());
    }
}

void World::Defer(std::function<void(World&)> action) {
    if (stepping_) {
        deferred_.push_back(std::move(action));
    } else {
        action(*this);
    }
}

void World::AddSystem(SystemPhase phase, std::string name, System system) {
    auto& systems = phase == SystemPhase::PreUpdate ? preUpdate_ : postUpdate_;
    const auto timing = telemetry_.RegisterTiming((phase == SystemPhase::PreUpdate ? "system.pre." : "system.post.") + name);
    systems.push_back({std::move(name), std::move(system), timing});
}

void World::RunSystems(SystemPhase phase, const FrameTime& time, const Input& input) {
    for (const SystemEntry& system : phase == SystemPhase::PreUpdate ? preUpdate_ : postUpdate_) {
        const TelemetryScope measure(telemetry_,system.timing);
        system.run(*this, time, input);
    }
}

void World::Step(double delta, const Input& input) {
    const TelemetryScope measure(telemetry_,stepMetric_);
    const FrameTime time{delta, 0.0, elapsed_, steps_};
    stepping_ = true;
    RunSystems(SystemPhase::PreUpdate, time, input);
    // A Behaviour class added during the step is updated from the next step on.
    const std::size_t classes = behaviours_.size();
    {
        const TelemetryScope behaviours(telemetry_,behaviourMetric_);
        for (std::size_t i = 0; i < classes; ++i) behaviours_[i].update(*this, time, input);
    }
    RunSystems(SystemPhase::PostUpdate, time, input);
    stepping_ = false;
    FinishStep();
    ++steps_;
    elapsed_ += delta;
}

void World::FinishStep() {
    // Destructions first, then deferred actions; an action may itself destroy or defer, and then
    // runs immediately, since the step is over.
    for (const entt::entity id : pendingDestroy_) {
        if (registry_.valid(id)) {
            registry_.destroy(id);
        }
    }
    pendingDestroy_.clear();
    auto actions = std::move(deferred_);
    deferred_.clear();
    for (auto& action : actions) {
        action(*this);
    }
}

void World::ResetInterpolation(Entity entity) {
    if (auto* transform = registry_.try_get<Transform2D>(entity.Id())) {
        registry_.emplace_or_replace<PreviousTransform2D>(entity.Id(), *transform);
    }
}

std::size_t World::EntityCount() const {
    return registry_.view<entt::entity>().size();
}

} // namespace pixie
