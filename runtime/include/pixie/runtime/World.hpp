#pragma once

#include "pixie/runtime/Export.hpp"

#include "pixie/core/FrameTime.hpp"
#include "pixie/platform/Input.hpp"
#include "pixie/core/Telemetry.hpp"
#include "pixie/runtime/Behaviour.hpp"
#include "pixie/runtime/InputControl.hpp"
#include "pixie/runtime/Entity.hpp"
#include "pixie/runtime/Random.hpp"
#include "pixie/runtime/Presentation2D.hpp"
#include <pixie/math/pxVec.hpp>

#include <entt/entity/registry.hpp>

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <type_traits>
#include <vector>

namespace pixie {
struct PhysicsSettings2D;

/// Selects a system's phase. Execution is PreUpdate, Behaviours, Physics, then PostUpdate.
enum class SystemPhase {
    // Before the Behaviours: shared state they read (the previous transform, spatial queries).
    PreUpdate,
    // After Physics: effects, bounds and other consumers of completed movement.
    PostUpdate,
    Physics, // After Behaviours, before every PostUpdate system.
};

/// Runs shared work over a World once per completed pass through its registered phase.
using System = std::function<void(World& world, const FrameTime& time, const Input& input)>;

// A logical Scene scope identity, separate from an entity's persisted SceneId.
// Zero denotes direct World membership and cannot identify a Scene scope.
struct ScopeId {
    std::uint64_t value = 0;
    friend bool operator==(ScopeId, ScopeId) = default;
    friend bool operator<(ScopeId a, ScopeId b) { return a.value < b.value; }
};

enum class SceneGravityMode2D { Inherit, Multiply, Override };

// Both inactive values remain authored when the mode changes.
struct SceneGravity2D {
    SceneGravityMode2D mode = SceneGravityMode2D::Inherit;
    pxVec2 value{0, 0};
    double multiplier = 1;
    bool operator==(const SceneGravity2D&) const = default;
};

struct SceneScope {
    ScopeId id;
    std::string name;
    std::optional<SceneGravity2D> gravity;
    std::optional<SceneEnvironment2D> environment;
    std::uint64_t camera = 0;
};

struct GravityResolution2D {
    pxVec2 value{0, 0};
    ScopeId scope;
    SceneGravityMode2D mode = SceneGravityMode2D::Inherit;
    std::string error;
    explicit operator bool() const { return error.empty(); }
};

/// Owns one live simulation, including ECS storage, step clock, RNG and logical Scene scopes.
/// Step runs PreUpdate, Behaviours, Physics and PostUpdate, then queued destruction and deferred
/// actions. Systems run in registration order within each phase. The built-in PreUpdate system
/// records previous transforms. The built-in Physics integrator applies acceleration and velocity
/// to eligible entities. An installed scene movement service owns body and motor motion.
class World {
public:
    PIXIE_RUNTIME_API explicit World(std::uint64_t seed = 0);
    PIXIE_RUNTIME_API ~World();
    World(const World&) = delete;
    World& operator=(const World&) = delete;

    // A new entity, with a Name if `name` is not empty.
    PIXIE_RUNTIME_API Entity Create(std::string_view name = {});
    /// Queues destruction during a step; otherwise destroys immediately. Ignores foreign or dead handles.
    PIXIE_RUNTIME_API void Destroy(Entity entity);
    /// Runs action immediately outside the simulation phases. During a phase it queues the action
    /// after pending destruction. Use this for structural edits that could invalidate active views.
    PIXIE_RUNTIME_API void Defer(std::function<void(World&)> action);

    // In-memory logical scopes. Catalog edits are accepted only between steps. IDs are supplied
    // by the caller and never consume the simulation RNG. Names may change without changing IDs.
    PIXIE_RUNTIME_API bool CreateScope(ScopeId id, std::string_view name = {},
                                       std::optional<SceneGravity2D> gravity = std::nullopt);
    PIXIE_RUNTIME_API bool RenameScope(ScopeId id, std::string_view name);
    PIXIE_RUNTIME_API std::string ValidateScopeGravity(const SceneGravity2D& gravity) const;
    // Pure prospective check for World physics values and every retained scope override.
    PIXIE_RUNTIME_API std::string ValidatePhysicsSettings(const PhysicsSettings2D& settings) const;
    PIXIE_RUNTIME_API bool SetScopeGravity(ScopeId id, SceneGravity2D gravity);
    PIXIE_RUNTIME_API bool SetScopeEnvironment(ScopeId id, std::optional<SceneEnvironment2D> value);
    PIXIE_RUNTIME_API bool SetScopeCamera(ScopeId id, std::uint64_t entityId);
    const std::optional<WorldPresentation2D>& Presentation() const { return presentation_; }
    PIXIE_RUNTIME_API bool SetPresentation(std::optional<WorldPresentation2D> value);
    PIXIE_RUNTIME_API GravityResolution2D ResolveGravity(ScopeId id) const;
    PIXIE_RUNTIME_API GravityResolution2D ResolveGravity(Entity entity) const;
    // Resolve a consumer's effective runtime base through the same Scene policy.
    // For example, particles inherit the already-selected live backend gravity.
    // The one-argument overloads continue to report authored World defaults.
    PIXIE_RUNTIME_API GravityResolution2D ResolveGravity(Entity entity, pxVec2 effectiveWorldGravity) const;
    // Refuses an occupied scope. Reassign its entities to another scope or World first.
    PIXIE_RUNTIME_API bool RemoveScope(ScopeId id);
    PIXIE_RUNTIME_API std::vector<SceneScope> Scopes() const;
    /// Zero denotes direct World membership. Rejects invalid entities, unknown scopes and transfers
    /// of linked entities. A request during a phase queues a transfer for revalidation at finish.
    PIXIE_RUNTIME_API bool SetScope(Entity entity, ScopeId id);
    /// Returns no value for dead or foreign entities and ScopeId{} for direct World membership.
    PIXIE_RUNTIME_API std::optional<ScopeId> ScopeOf(Entity entity) const;

    // Adds a system to a phase, after the ones already there. `name` labels it for tools.
    PIXIE_RUNTIME_API void AddSystem(SystemPhase phase, std::string name, System system);

    /// Advances by delta seconds and borrows input for the call. Recursive Step throws, including
    /// from deferred actions. On failure earlier writes may survive and the clock does not advance.
    PIXIE_RUNTIME_API void Step(double delta, const Input& input);

    // Steps completed, and simulated seconds they covered.
    std::uint64_t Steps() const { return steps_; }
    double Elapsed() const { return elapsed_; }
    /// True in simulation phases. Deferred actions run with this flag cleared.
    bool IsStepping() const { return stepping_; }

    // Per-World CPU metrics, disabled initially and never serialized. Named systems and whole-step
    // timings are inclusive; presentation does not control collection.
    Telemetry& GetTelemetry() { return telemetry_; }
    const Telemetry& GetTelemetry() const { return telemetry_; }

    Random& GetRandom() { return random_; }

    // Makes the previous transform equal the current one, so a jump is not drawn as motion.
    PIXIE_RUNTIME_API void ResetInterpolation(Entity entity);

    PIXIE_RUNTIME_API std::size_t EntityCount() const;
    /// Borrows the underlying EnTT registry. The caller owns the lifetime of any views it creates
    /// and must respect World invariants when mutating storage directly.
    entt::registry& Registry() { return registry_; }
    const entt::registry& Registry() const { return registry_; }
    /// Wraps an ID from this World without a liveness check.
    Entity Wrap(entt::entity id) { return Entity(this, id); }

    /// Calls fn(Entity, Components&...) for each match. Keep the iterated storage stable during fn.
    template<typename... Components, typename Fn>
    void Each(Fn&& fn) {
        registry_.view<Components...>().each([this, &fn](entt::entity id, Components&... components) {
            fn(Entity(this, id), components...);
        });
    }

private:
    friend class Entity;

    GravityResolution2D ResolveGravityWithSettings(ScopeId id, const PhysicsSettings2D& settings) const;

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

    PIXIE_RUNTIME_API void RunSystems(SystemPhase phase, const FrameTime& time, const Input& input);
    PIXIE_RUNTIME_API void FinishStep();

    entt::registry registry_;
    Telemetry telemetry_;
    MetricHandle stepMetric_, behaviourMetric_;
    std::vector<SystemEntry> preUpdate_;
    std::vector<SystemEntry> postUpdate_;
    std::vector<SystemEntry> physics_;
    std::vector<BehaviourClass> behaviours_;
    std::vector<entt::entity> pendingDestroy_;
    std::vector<std::function<void(World&)>> deferred_;
    std::map<ScopeId, SceneScope> scopes_;
    std::map<entt::entity, ScopeId> membership_;
    Random random_;
    std::optional<WorldPresentation2D> presentation_;
    std::uint64_t steps_ = 0;
    double elapsed_ = 0.0;
    bool stepping_ = false;
    bool advancing_ = false;
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
        // Start may initialize input state. Recheck the matching control per event so a callback
        // can disable further delivery within the same tick.
        const auto* keys = world.registry_.try_get<KeyboardControl>(id);
        const auto* mouse = world.registry_.try_get<MouseControl>(id);
        if ((keys && keys->enabled) || (mouse && mouse->enabled)) {
            for (const auto& event : input.Events()) {
                std::visit([&](const auto& args) {
                    using Event = std::decay_t<decltype(args)>;
                    if constexpr (std::is_same_v<Event, KeyEvent>) {
                        const auto* control = world.registry_.try_get<KeyboardControl>(id);
                        if (control && control->enabled) behaviour.OnKey(args, input);
                    } else {
                        const auto* control = world.registry_.try_get<MouseControl>(id);
                        if (!control || !control->enabled) return;
                        if constexpr (std::is_same_v<Event, MouseButtonEvent>) behaviour.OnMouseButton(args, input);
                        else if constexpr (std::is_same_v<Event, MouseMoveEvent>) behaviour.OnMouseMove(args, input);
                        else if constexpr (std::is_same_v<Event, MouseScrollEvent>) behaviour.OnMouseScroll(args, input);
                    }
                }, event);
            }
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
