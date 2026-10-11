#pragma once

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
    // Closes the window after this many seconds; 0 runs until the window is closed.
    double quitAfterSeconds = 0.0;
};

} // namespace pixie
