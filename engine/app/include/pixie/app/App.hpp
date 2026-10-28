#pragma once

#include "pixie/core/FrameTime.hpp"
#include "pixie/platform/Input.hpp"
#include "pixie/platform/Window.hpp"

namespace pixie {

class Telemetry;

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
    // Optional non-owning view for native debug display/control; valid until the next App callback.
    // The collector stays owned by the app/session. Existing apps need not provide one.
    virtual Telemetry* GetTelemetry() { return nullptr; }
};

} // namespace pixie
