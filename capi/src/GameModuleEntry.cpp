// Installed game-module entry support. The loader queries the C ABI and compiled SDK tuple
// before calling the factory. The host owns the returned Project and keeps the module leased.
#include "pixie/runtime/Project.hpp"
#include "pixie_capi.h"
#include "pixie_sdk_build.h"

#ifdef _WIN32
#define PIXIE_MODULE_EXPORT __declspec(dllexport)
#else
#define PIXIE_MODULE_EXPORT __attribute__((visibility("default")))
#endif
extern "C" PIXIE_MODULE_EXPORT int pixie_game_api_version() { return PIXIE_CAPI_VERSION; }
// C-only tuple admission follows OS image loading and precedes every Project callback.
extern "C" PIXIE_MODULE_EXPORT const char* pixie_game_sdk_identity() { return PIXIE_SDK_NATIVE_IDENTITY_JSON; }
// Contract1 requires fresh candidate-owned construction, stable descriptors and
// nonthrowing teardown. No factory/Describe/Build callback may mutate published
// or external state. Cleanup hooks/deleters likewise remain local to retired or
// candidate state and must not reach published state. This cooperative opt-in
// does not by itself admit custom scene types.
extern "C" PIXIE_MODULE_EXPORT int pixie_game_candidate_contract() {
#ifdef PIXIE_CANDIDATE_AUTHORING
    return 1;
#else
    return 0;
#endif
}
extern "C" PIXIE_MODULE_EXPORT pixie::Project* pixie_game_create() noexcept {
    // No C++ exception crosses the module's C factory boundary (/EHc consumers).
    try { return pixie::CreateProject().release(); }
    catch (...) { return nullptr; }
}
