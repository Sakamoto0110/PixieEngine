#pragma once

#include "pixie/FrameTime.hpp"
#include "pixie/Input.hpp"
#include "pixie/Telemetry.hpp"
#include "pixie/runtime/Behaviour.hpp"
#include "pixie/runtime/Entity.hpp"
#include "pixie/runtime/Random.hpp"

#include <entt/entity/registry.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pixie {

// When an engine system runs within a step. Behaviours run between the two.
enum class SystemPhase {
    // Before the Behaviours: shared state they read (the previous transform, spatial queries).
    PreUpdate,
    // After the Behaviours: what applies their results (integration, bounds).
    PostUpdate,
};

// A system: a function over the whole World, run once per step in its phase.
using System = std::function<void(World& world, const FrameTime& time, const Input& input)>;

// The live simulation: entities and their components, stored contiguously per component type by
// EnTT, with Behaviours for per-entity logic and systems for the shared work. A World owns its time
// (steps and simulated seconds) and its random number generator; nothing in it is global.
//
// A step runs, in order: the PreUpdate systems, then every Behaviour (class by class), then the
// PostUpdate systems, then the destructions and deferred actions requested during the step. The
// World's own systems come first in their phase: in PreUpdate, copying each Transform2D into its
// PreviousTransform2D; in PostUpdate, integrating Velocity2D into Transform2D for entities without
// CharacterMotor2D and RigidBody2D, whose physics service owns motion instead.
class World {
public:
    explicit World(std::uint64_t seed = 0);
    ~World();
    World(const World&) = delete;
    World& operator=(const World&) = delete;

    // A new entity, with a Name if `name` is not empty.
    Entity Create(std::string_view name = {});
    // Destroys the entity; during a step, at the end of it.
    void Destroy(Entity entity);
    // Runs `action` at the end of the current step, or now when no step is running. Creating
    // entities and adding or removing components during a step goes through here.
    void Defer(std::function<void(World&)> action);

    // Adds a system to a phase, after the ones already there. `name` labels it for tools.
    void AddSystem(SystemPhase phase, std::string name, System system);

    // Advances the World by `delta` seconds.
    void Step(double delta, const Input& input);

    // Steps completed, and simulated seconds they covered.
    std::uint64_t Steps() const { return steps_; }
    double Elapsed() const { return elapsed_; }
    bool IsStepping() const { return stepping_; }

    // Per-World CPU metrics, disabled initially and never serialized. Named systems and whole-step
    // timings are inclusive; presentation does not control collection.
    Telemetry& GetTelemetry() { return telemetry_; }
    const Telemetry& GetTelemetry() const { return telemetry_; }

    Random& GetRandom() { return random_; }

    // Makes the previous transform equal the current one, so a jump is not drawn as motion.
    void ResetInterpolation(Entity entity);

    std::size_t EntityCount() const;
    entt::registry& Registry() { return registry_; }
    const entt::registry& Registry() const { return registry_; }
    Entity Wrap(entt::entity id) { return Entity(this, id); }

    // Calls fn(Entity, Components&...) for every entity that has all the components.
    template<typename... Components, typename Fn>
    void Each(Fn&& fn) {
        registry_.view<Components...>().each([this, &fn](entt::entity id, Components&... components) {
            fn(Entity(this, id), components...);
        });
    }

private:
    friend class Entity;

    struct SystemEntry {
        std::string name;
        System run;
        MetricHandle timing;
    };
    struct BehaviourClass {
        entt::id_type type;
        void (*update)(World&, const FrameTime&, const Input&);
        void (*clear)(World&);
    };

    template<typename B>
    void RegisterBehaviour();
    template<typename B>
    static void UpdateAll(World& world, const FrameTime& time, const Input& input);
    template<typename B>
    static void ClearAll(World& world);
    template<typename B>
    static void StopOne(entt::registry& registry, entt::entity id);

    void RunSystems(SystemPhase phase, const FrameTime& time, const Input& input);
    void FinishStep();

    entt::registry registry_;
    Telemetry telemetry_;
    MetricHandle stepMetric_, behaviourMetric_;
    std::vector<SystemEntry> preUpdate_;
    std::vector<SystemEntry> postUpdate_;
    std::vector<BehaviourClass> behaviours_;
    std::vector<entt::entity> pendingDestroy_;
    std::vector<std::function<void(World&)>> deferred_;
    Random random_;
    std::uint64_t steps_ = 0;
    double elapsed_ = 0.0;
    bool stepping_ = false;
};

// --- Templates of World ---------------------------------------------------------------------------

template<typename B>
void World::RegisterBehaviour() {
    const entt::id_type type = entt::type_hash<B>::value();
    for (const BehaviourClass& known : behaviours_) {
        if (known.type == type) {
            return;
        }
    }
    registry_.on_destroy<B>().template connect<&World::StopOne<B>>();
    behaviours_.push_back({type, &World::UpdateAll<B>, &World::ClearAll<B>});
}

template<typename B>
void World::UpdateAll(World& world, const FrameTime& time, const Input& input) {
    for (auto [id, behaviour] : world.registry_.storage<B>().each()) {
        if (!behaviour.started_) {
            behaviour.started_ = true;
            behaviour.Start();
        }
        behaviour.Update(time, input);
    }
}

template<typename B>
void World::ClearAll(World& world) {
    world.registry_.storage<B>().clear();
}

template<typename B>
void World::StopOne(entt::registry& registry, entt::entity id) {
    B& behaviour = registry.get<B>(id);
    if (behaviour.started_) {
        behaviour.started_ = false;
        behaviour.Stop();
    }
}

// --- Templates of Entity, which need the World ----------------------------------------------------

template<typename T, typename... Args>
decltype(auto) Entity::Add(Args&&... args) const {
    return world_->registry_.emplace_or_replace<T>(id_, std::forward<Args>(args)...);
}

template<typename T>
T& Entity::Get() const {
    return world_->registry_.get<T>(id_);
}

template<typename T>
T* Entity::TryGet() const {
    return world_->registry_.try_get<T>(id_);
}

template<typename T>
bool Entity::Has() const {
    return world_->registry_.all_of<T>(id_);
}

template<typename T>
void Entity::Remove() const {
    world_->registry_.remove<T>(id_);
}

template<std::derived_from<Behaviour> B, typename... Args>
B& Entity::AddBehaviour(Args&&... args) const {
    world_->RegisterBehaviour<B>();
    B& behaviour = world_->registry_.emplace_or_replace<B>(id_, std::forward<Args>(args)...);
    behaviour.self_ = *this;
    return behaviour;
}

} // namespace pixie
