#pragma once

#include "pixie/runtime/Export.hpp"

#include <entt/entity/entity.hpp>

#include <concepts>
#include <cstdint>

namespace pixie {

class World;
class Behaviour;

/// Non-owning handle combining a World pointer with an EnTT identity. While that World lives,
/// IsAlive distinguishes a removed entity from a live one. Discard all handles when their World
/// is destroyed or replaced. Include World.hpp to use the component templates.
class Entity {
public:
    Entity() = default;
    /// Constructs an unchecked handle. The ID must come from the supplied World to use its components.
    Entity(World* world, entt::entity id) : world_(world), id_(id) {}

    /// Borrows the owning World. Requires a non-null handle and a live World.
    World& GetWorld() const { return *world_; }
    entt::entity Id() const { return id_; }
    /// Runtime number (EnTT index and version) for logs and interop. Persist SceneId for authored identity.
    std::uint32_t Number() const { return static_cast<std::uint32_t>(id_); }

    /// Checks a null or removed entity while its World, if any, is still alive.
    PIXIE_RUNTIME_API bool IsAlive() const;
    explicit operator bool() const { return IsAlive(); }

    /// Adds or replaces T on a live entity. Returns a borrowed component reference or void for an
    /// empty tag. Structural edits can invalidate borrowed storage.
    template<typename T, typename... Args>
    decltype(auto) Add(Args&&... args) const;
    /// Borrows an existing T. Requires a live entity with a stored component of that type.
    template<typename T>
    T& Get() const;
    /// Borrows a stored T from a live entity, or returns null when the component is absent.
    template<typename T>
    T* TryGet() const;
    template<typename T>
    bool Has() const;
    template<typename T>
    void Remove() const;

    /// Registers B for updates, adds or replaces the component and binds Self to this live entity.
    /// Remove a started B explicitly when its Stop hook must run before adding a new instance.
    template<std::derived_from<Behaviour> B, typename... Args>
    B& AddBehaviour(Args&&... args) const;

    /// Requests destruction from the owning World. Requires a non-null handle to a live World.
    PIXIE_RUNTIME_API void Destroy() const;

    friend bool operator==(const Entity&, const Entity&) = default;

private:
    friend class World;
    World* world_ = nullptr;
    entt::entity id_ = entt::null;
};

} // namespace pixie
