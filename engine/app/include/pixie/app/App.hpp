#pragma once

#include "pixie/core/FrameTime.hpp"
#include "pixie/platform/Input.hpp"
#include "pixie/platform/Window.hpp"

namespace pixie {

class Telemetry;

/// Native player callbacks driven by Run on the thread with the current GL context.
/// Time and input are borrowed for each callback.
class App {
public:
    virtual ~App() = default;

    /// Initializes after GL loading. The Window is borrowed through Stop.
    /// Returning false leads to Stop for partial cleanup and an error result from Run.
    virtual bool Start(Window&) { return true; }
    /// Advances one fixed step. The first update to consume pending input sees its transients.
    virtual void Update(const FrameTime& time, const Input& input) = 0;
    /// Draws a frame with a positive framebuffer size. time.alpha interpolates previous/current state.
    /// Input reflects any fixed updates already run for this frame.
    virtual void Render(const FrameTime& time, const Input& input) = 0;
    /// Releases GL objects while the context is current, including after a failed Start.
    virtual void Stop() {}
    // Optional non-owning view for native debug display/control; valid until the next App callback.
    // The collector stays owned by the app/session. Existing apps need not provide one.
    virtual Telemetry* GetTelemetry() { return nullptr; }
};

} // namespace pixie
