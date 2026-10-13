#pragma once

#include "pixie/FrameTime.hpp"
#include "pixie/Input.hpp"
#include "pixie/Window.hpp"

namespace pixie {

// What pixie::Run drives. The GL context is current during every call. Time and input are passed
// to each call; there is no global clock or input state.
class App {
public:
    virtual ~App() = default;

    // Called once, after the context exists. Returning false ends the run with an error.
    virtual bool Start(Window&) { return true; }
    // Called at the fixed update rate; time.delta is the fixed step. Presses and releases since the
    // previous update are visible here once.
    virtual void Update(const FrameTime& time, const Input& input) = 0;
    // Called once per frame; time.alpha interpolates between the last two updates.
    virtual void Render(const FrameTime& time, const Input& input) = 0;
    // Called once, before the context is destroyed. Release GL objects here.
    virtual void Stop() {}
};

} // namespace pixie
