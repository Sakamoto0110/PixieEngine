// The development module exports only this versioned factory and its game's code.
#include "pixie/runtime/Project.hpp"
#include "pixie_capi.h"

#ifdef _WIN32
#define PIXIE_MODULE_EXPORT __declspec(dllexport)
#else
#define PIXIE_MODULE_EXPORT __attribute__((visibility("default")))
#endif
extern "C" PIXIE_MODULE_EXPORT int pixie_game_api_version() { return PIXIE_CAPI_VERSION; }
extern "C" PIXIE_MODULE_EXPORT pixie::Project* pixie_game_create() {
    return pixie::CreateProject().release();
}
