#pragma once

#include "pixie/App.hpp"
#include "pixie/RunConfig.hpp"

namespace pixie {

// Opens the window, runs the main loop until the window closes, and returns a process exit code.
// F3 toggles read-only FPS/UPS; F3+C opens the top command bar (see docs/player-debug.md).
// The bar captures game input while open; display commands do not change the fixed update rate.
int Run(App& app, const RunConfig& config);

} // namespace pixie
