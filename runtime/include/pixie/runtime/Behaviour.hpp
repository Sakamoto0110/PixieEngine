#pragma once

#include "pixie/FrameTime.hpp"
#include "pixie/Input.hpp"
#include "pixie/runtime/Entity.hpp"

namespace pixie {

// Game logic written as a class, attached to an entity with Entity::AddBehaviour<T>(). A Behaviour
// is a component: each Behaviour class has its own contiguous storage, and the World calls Update
// on every instance of a class, class by class in the order the classes were first added, during
// the Update phase of each step.
//
// - Start runs once, right before the first Update of the instance.
// - Update runs every step. It should change only its own entity's state (its components), and
//   read the rest of the World: the engine's PreUpdate and PostUpdate systems do the shared work
//   (integration, spatial queries), so the result does not depend on update order.
// - Stop runs when the entity or the Behaviour is removed, or the World ends, if Start ran.
//
// Behaviours are moved when their storage grows, so keep no pointers to them; keep Entity handles.
// Creating entities or adding components during a step goes through World::Defer.
class Behaviour {
public:
    virtual ~Behaviour() = default;

    virtual void Start() {}
    virtual void Update(const FrameTime& /*time*/, const Input& /*input*/) {}
    virtual void Stop() {}

    Entity Self() const { return self_; }
    World& GetWorld() const { return self_.GetWorld(); }

protected:
    Behaviour() = default;
    Behaviour(const Behaviour&) = default;
    Behaviour(Behaviour&&) = default;
    Behaviour& operator=(const Behaviour&) = default;
    Behaviour& operator=(Behaviour&&) = default;

private:
    friend class World;
    friend class Entity;

    Entity self_;
    bool started_ = false;
};

} // namespace pixie
