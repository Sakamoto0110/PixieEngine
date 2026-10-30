/* A valid module handshake with a broken factory, for rejection qualification. */
#include "pixie_capi.h"
#include <stddef.h>
#ifdef _WIN32
#define MODULE_API __declspec(dllexport)
#else
#define MODULE_API __attribute__((visibility("default")))
#endif
MODULE_API int pixie_game_api_version(void) { return PIXIE_CAPI_VERSION; }
MODULE_API void* pixie_game_create(void) { return NULL; }
