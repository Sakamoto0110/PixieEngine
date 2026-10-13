#pragma once

#include <entt/entity/entity.hpp>

#include <concepts>
#include <cstdint>

namespace pixie {

class World;
class Behaviour;

// A handle to one entity of a World: the EC-style way to reach its components. It is a value (a
// World pointer and an EnTT entity) and stays valid to copy after the entity is gone; IsAlive
// says whether it still names a live entity.
//
// The template members are defined in World.hpp; include that to use them.
class Entity {
public:
    Entity() = default;
    Entity(World* world, entt::entity id) : world_(world), id_(id) {}

    World& GetWorld() const { return *world_; }
    entt::entity Id() const { return id_; }
    // The entity as a 32-bit number (index and version), for logs and the C API.
    std::uint32_t Number() const { return static_cast<std::uint32_t>(id_); }

    bool IsAlive() const;
    explicit operator bool() const { return IsAlive(); }

    // Adds a component, or replaces the one of the same type, and returns it; returns nothing for
    // an empty (tag) type, which EnTT stores without instances.
    template<typename T, typename... Args>
    decltype(auto) Add(Args&&... args) const;
    template<typename T>
    T& Get() const;
    // Null when the entity has no T.
    template<typename T>
    T* TryGet() const;
    template<typename T>
    bool Has() const;
    template<typename T>
    void Remove() const;

    // Adds a Behaviour, a component whose Update runs every step; see Behaviour.hpp.
    template<std::derived_from<Behaviour> B, typename... Args>
    B& AddBehaviour(Args&&... args) const;

    // Destroys the entity, at the end of the step when called during one.
    void Destroy() const;

    friend bool operator==(const Entity&, const Entity&) = default;

private:
    World* world_ = nullptr;
    entt::entity id_ = entt::null;
};

} // namespace pixie
