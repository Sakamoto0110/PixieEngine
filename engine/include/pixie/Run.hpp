#pragma once

#include "pixie/App.hpp"
#include "pixie/RunConfig.hpp"

namespace pixie {

// Opens the window, runs the main loop until the window closes, and returns a process exit code.
int Run(App& app, const RunConfig& config);

} // namespace pixie
