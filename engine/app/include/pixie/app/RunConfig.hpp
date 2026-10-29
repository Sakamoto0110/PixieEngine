#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace pixie {

struct RunConfig {
    std::string title = "PixieEngine";
    int width = 1280;
    int height = 720;
    // Update calls per second; fixed regardless of the frame rate.
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
    // Zero captures the initial state. An authored scene seed takes precedence.
    std::optional<std::uint64_t> quitAfterSteps;
    std::uint64_t seed = 1;
    bool exactFramebufferSize = false;
#endif
    // With a timed or step-limited exit, the final frame is saved here as a PNG.
    std::filesystem::path screenshotPath;
};

} // namespace pixie
