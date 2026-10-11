#pragma once

#include "pixie/FrameTime.hpp"
#include "pixie/Window.hpp"

namespace pixie {

// What pixie::Run drives. The GL context is current during every call.
class App {
public:
    virtual ~App() = default;

    // Called once, after the context exists. Returning false ends the run with an error.
    virtual bool Start(Window&) { return true; }
    // Called at the fixed update rate; time.delta is the fixed step.
    virtual void Update(const FrameTime& time) = 0;
    // Called once per frame; time.alpha interpolates between the last two updates.
    virtual void Render(const FrameTime& time) = 0;
    // Called once, before the context is destroyed. Release GL objects here.
    virtual void Stop() {}
};

} // namespace pixie
