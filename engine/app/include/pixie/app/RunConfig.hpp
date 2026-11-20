#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#ifdef PIXIE_DEVELOPMENT
#include "pixie/core/FramebufferSize.hpp"
#include <functional>
#endif

namespace pixie {

struct RunConfig {
    std::string title = "PixieEngine";
    int width = 1280;
    int height = 720;
    /// Fixed updates per second. Supply a finite positive rate.
    double updateRate = 60.0;
    // Maximum frames per second; 0 means no cap. Applies together with vsync.
    double frameRateCap = 0.0;
    bool vsync = true;
    // No override by default: host collection is off and the app chooses its own setting.
    // on/off overrides both host and the collector exposed by App::GetTelemetry.
    std::optional<bool> telemetry;
    // Closes the window after this many seconds; 0 runs until the window is closed.
    double quitAfterSeconds = 0.0;
#ifdef PIXIE_DEVELOPMENT
    /// The built-in update driver quits after this many steps. Zero draws the initial state.
    std::optional<std::uint64_t> quitAfterSteps;
    /// Initial seed for a caller's World setup. An authored scene seed takes precedence.
    std::uint64_t seed = 1;
    bool exactFramebufferSize = false;
    bool hidden = false;
    /// Replaces the built-in update branch. Receives wall seconds. The host owns injected input.
    std::function<void(double)> advanceFrame;
    /// Runs after advancement, including on paused or minimized frames.
    std::function<void()> beforeRender;
    /// Overrides the interpolation value passed to App::Render.
    std::function<double()> renderAlpha;
    /// Runs after game drawing and before the overlay. Also receives empty sizes when minimized.
    std::function<void(FramebufferSize)> afterRender;
    /// Runs before App::Stop while the context is current.
    std::function<void()> beforeStop;
#endif
    // With a timed or step-limited exit, the final frame is saved here as a PNG.
    std::filesystem::path screenshotPath;
};

} // namespace pixie
