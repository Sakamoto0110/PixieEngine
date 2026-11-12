#pragma once

#include "pixie/app/Export.hpp"

#include "pixie/app/App.hpp"
#include "pixie/app/RunConfig.hpp"

namespace pixie {

/// Borrows app, opens the player window and drives callbacks through Stop.
/// Returns 0 on normal exit or 1 on window, loader, Start or overlay startup failure.
/// F3 toggles read-only FPS/UPS. F3+C toggles the modal command bar (docs/player-debug.md).
PIXIE_APP_API int Run(App& app, const RunConfig& config);

} // namespace pixie
