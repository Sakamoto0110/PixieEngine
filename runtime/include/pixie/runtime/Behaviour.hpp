#pragma once

#include "pixie/core/FrameTime.hpp"
#include "pixie/platform/Input.hpp"
#include "pixie/runtime/Entity.hpp"

namespace pixie {

/// Per-entity logic attached with Entity::AddBehaviour<T>(). World visits classes in registration
/// order and instances in storage order, after PreUpdate and before Physics. Keep Entity handles
/// while their World lives because storage can move Behaviour instances. Use World::Defer for
/// structural changes during a step.
class Behaviour {
public:
    virtual ~Behaviour() = default;

    /// Runs once on first visit, before device callbacks and Update.
    virtual void Start() {}
    /// Key, button, move and scroll callbacks follow host event order for this instance. Delivery
    /// requires the matching enabled KeyboardControl or MouseControl on Self. Input polling reads
    /// the final tick state. The event and input are borrowed for the call.
    virtual void OnKey(const KeyEvent& /*event*/, const Input& /*input*/) {}
    /// See OnKey for device callback ordering and lifetime.
    virtual void OnMouseButton(const MouseButtonEvent& /*event*/, const Input& /*input*/) {}
    /// See OnKey for device callback ordering and lifetime.
    virtual void OnMouseMove(const MouseMoveEvent& /*event*/, const Input& /*input*/) {}
    /// See OnKey for device callback ordering and lifetime.
    virtual void OnMouseScroll(const MouseScrollEvent& /*event*/, const Input& /*input*/) {}
    /// Runs after device callbacks each tick. For order-independent logic, read shared snapshots
    /// and write only Self's state. Put shared work in systems.
    virtual void Update(const FrameTime& /*time*/, const Input& /*input*/) {}
    /// Runs on removal, entity destruction or World teardown for an instance that has started.
    virtual void Stop() {}

    /// Returns the owning entity by value. Its handle requires the owning World to remain alive.
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
