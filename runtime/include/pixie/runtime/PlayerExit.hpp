#pragma once
#include "pixie/runtime/Export.hpp"

namespace pixie {
class World;
// A per-World request, consumed by the native player after an update. Embedded hosts own their lifetime.
PIXIE_RUNTIME_API void RequestPlayerExit(World& world);
PIXIE_RUNTIME_API bool PlayerExitRequested(World& world);
}
