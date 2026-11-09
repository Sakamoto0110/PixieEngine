/* Controlled identity negatives. No C++ Project or GL context is required. */
#include "pixie_capi.h"
#include "pixie_sdk_build.h"
#include <stddef.h>

#define MODULE_API __declspec(dllexport)
static int factory_calls;
#if PIXIE_IDENTITY_CASE != 0
static const char* identity_override;
MODULE_API void pixie_test_identity_override(const char* identity) { identity_override = identity; }
MODULE_API const char* pixie_game_sdk_identity(void) {
    if (identity_override) return identity_override;
#if PIXIE_IDENTITY_CASE == 2
    return PIXIE_SDK_IDENTITY_FOR("0000000000000000000000000000000000000000000000000000000000000000",
        PIXIE_SDK_CONFIGURATION, PIXIE_SDK_PROFILE_NAME);
#elif PIXIE_IDENTITY_CASE == 3
    return PIXIE_SDK_IDENTITY_FOR(PIXIE_SDK_ID, "incompatible-configuration", PIXIE_SDK_PROFILE_NAME);
#elif PIXIE_IDENTITY_CASE == 4
    return PIXIE_SDK_IDENTITY_FOR(PIXIE_SDK_ID, PIXIE_SDK_CONFIGURATION, "incompatible-profile");
#else
    return PIXIE_SDK_NATIVE_IDENTITY_JSON;
#endif
}
#endif
MODULE_API int pixie_game_api_version(void) {
#if PIXIE_IDENTITY_CASE == 1
    return PIXIE_CAPI_VERSION + 1;
#else
    return PIXIE_CAPI_VERSION;
#endif
}
MODULE_API int pixie_test_factory_calls(void) { return factory_calls; }
MODULE_API void* pixie_game_create(void) { ++factory_calls; return NULL; }
