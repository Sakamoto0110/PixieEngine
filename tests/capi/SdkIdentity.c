/* Context-free C query, typed admission and selected-module retention qualification. */
#include "pixie_capi.h"
#include "pixie_sdk_build.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

static int failures;
static int checks;
#define CHECK(condition) do { ++checks; if (!(condition)) { \
    ++failures; printf("FAIL line %d: %s\n", __LINE__, #condition); } } while (0)

#ifdef PIXIE_DEVELOPMENT
static HMODULE ModuleHandle(const char* path, int load) {
    wchar_t wide[32768];
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wide, 32768)) return NULL;
    return load ? LoadLibraryExW(wide, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS)
                : GetModuleHandleW(wide);
}
#endif

static void Queries(void) {
    const int32_t length = pixie_sdk_identity(NULL, 0);
    char* identity;
    char* guarded;
    char sentinel = 'Q';
    char error[256];
    CHECK(length > 0 && length <= PIXIE_SDK_IDENTITY_MAX_LENGTH);
    if (length <= 0 || length > PIXIE_SDK_IDENTITY_MAX_LENGTH) return;
    identity = (char*)malloc((size_t)length + 1);
    guarded = (char*)malloc((size_t)length + 3);
    CHECK(identity != NULL && guarded != NULL);
    if (!identity || !guarded) { free(identity); free(guarded); return; }
    CHECK(pixie_sdk_identity(identity, length + 1) == length);
    CHECK(identity[length] == '\0' && (int32_t)strlen(identity) == length);
    CHECK(strcmp(identity, PIXIE_SDK_RUNTIME_IDENTITY_JSON) == 0);
    CHECK(pixie_api_version() == 28);
    CHECK(pixie_capabilities() == PIXIE_SDK_EXPECTED_CAPABILITIES);
    CHECK(pixie_sdk_identity(&sentinel, 0) == length && sentinel == 'Q');
    memset(guarded, 'Q', (size_t)length + 3);
    CHECK(pixie_sdk_identity(guarded + 1, 1) == length);
    CHECK(guarded[0] == 'Q' && guarded[1] == '\0' && guarded[2] == 'Q');
    memset(guarded, 'Q', (size_t)length + 3);
    CHECK(pixie_sdk_identity(guarded + 1, length) == length);
    CHECK(guarded[0] == 'Q' && guarded[length] == '\0' && guarded[length + 1] == 'Q');
    CHECK(memcmp(guarded + 1, identity, (size_t)length - 1) == 0);
    memset(guarded, 'Q', (size_t)length + 3);
    CHECK(pixie_sdk_identity(guarded + 1, length + 1) == length);
    CHECK(guarded[0] == 'Q' && guarded[length + 1] == '\0' && guarded[length + 2] == 'Q');
    CHECK(strcmp(guarded + 1, identity) == 0);
    CHECK(pixie_sdk_identity(NULL, 1) == -1);
    CHECK(pixie_last_error(error, (int32_t)sizeof(error)) > 0);
    CHECK(pixie_sdk_identity(&sentinel, -1) == -1 && sentinel == 'Q');
    CHECK(pixie_sdk_identity(NULL, -1) == -1);
    CHECK(pixie_sdk_identity(NULL, 0) == length);
    free(identity);
    free(guarded);
}

#ifdef PIXIE_DEVELOPMENT
typedef int (*Counter)(void);
typedef void (*Override)(const char*);

static char* Replace(const char* source, const char* old_text, const char* new_text) {
    const char* found = strstr(source, old_text);
    const size_t old_size = strlen(old_text);
    const size_t new_size = strlen(new_text);
    const size_t source_size = strlen(source);
    char* result;
    if (!found) return NULL;
    result = (char*)malloc(source_size - old_size + new_size + 1);
    if (!result) return NULL;
    memcpy(result, source, (size_t)(found - source));
    memcpy(result + (found - source), new_text, new_size);
    memcpy(result + (found - source) + new_size, found + old_size, strlen(found + old_size) + 1);
    return result;
}

static void RefuseOverride(Override override_identity, const char* path, const char* identity, const char* selected) {
    override_identity(identity);
    CHECK(pixie_module_load(path) == PIXIE_ERROR_ARGUMENT);
    CHECK(ModuleHandle(selected, 0) != NULL);
}

static void Modules(char** paths) {
    int index;
    const char* good = paths[5];
    const char* base = PIXIE_SDK_NATIVE_IDENTITY_JSON;
    CHECK(pixie_module_load(good) == PIXIE_OK);
    CHECK(ModuleHandle(good, 0) != NULL);
    for (index = 0; index < 5; ++index) {
        HMODULE module = ModuleHandle(paths[index], 1);
        Counter counter;
        CHECK(module != NULL);
        if (!module) continue;
        counter = (Counter)(void*)GetProcAddress(module, "pixie_test_factory_calls");
        CHECK(counter != NULL);
        CHECK(pixie_module_load(paths[index]) == PIXIE_ERROR_ARGUMENT);
        CHECK(ModuleHandle(good, 0) != NULL);
        if (counter) CHECK(counter() == 0);
        if (index == 2) {
            Override override_identity = (Override)(void*)GetProcAddress(module, "pixie_test_identity_override");
            char* changed;
            char* padded;
            const size_t size = strlen(base);
            CHECK(override_identity != NULL);
            if (!override_identity) { FreeLibrary(module); continue; }
            RefuseOverride(override_identity, paths[index], "{}", good);
            RefuseOverride(override_identity, paths[index], "{", good);
            RefuseOverride(override_identity, paths[index], "{\"schemaVersion\":2}", good);
            changed = Replace(base, "\"cApiVersion\":28", "\"cApiVersion\":28.0");
            CHECK(changed != NULL);
            if (changed) { RefuseOverride(override_identity, paths[index], changed, good); free(changed); }
            changed = Replace(base, "\"schemaVersion\":1", "\"schemaVersion\":1,\"schemaVersion\":1");
            CHECK(changed != NULL);
            if (changed) { RefuseOverride(override_identity, paths[index], changed, good); free(changed); }
            padded = (char*)malloc(PIXIE_SDK_IDENTITY_MAX_LENGTH + 2u);
            CHECK(padded != NULL);
            if (padded) {
                memset(padded, ' ', PIXIE_SDK_IDENTITY_MAX_LENGTH + 1u);
                memcpy(padded, base, size);
                padded[PIXIE_SDK_IDENTITY_MAX_LENGTH + 1u] = '\0';
                RefuseOverride(override_identity, paths[index], padded, good);
                /* A valid JSON identity at exactly the accepted limit is admitted. */
                padded[PIXIE_SDK_IDENTITY_MAX_LENGTH] = '\0';
                override_identity(padded);
                CHECK(pixie_module_load(paths[index]) == PIXIE_OK);
                CHECK(counter() == 0);
                CHECK(pixie_module_load(good) == PIXIE_OK);
                free(padded);
            }
            /* Extra provenance and different key order cannot become compatibility. */
            changed = (char*)malloc(size + 100);
            CHECK(changed != NULL);
            if (changed) {
                const char* prefix = "{\"files\":[],\"candidateContract\":99,\"productVersion\":\"display-only\",";
                memcpy(changed, prefix, strlen(prefix));
                memcpy(changed + strlen(prefix), base + 1, size);
                override_identity(changed);
                CHECK(pixie_module_load(paths[index]) == PIXIE_OK);
                CHECK(counter() == 0);
                CHECK(pixie_module_load(good) == PIXIE_OK);
                free(changed);
            }
            override_identity(NULL);
        }
        FreeLibrary(module);
    }
    CHECK(pixie_module_load(NULL) == PIXIE_OK);
    CHECK(ModuleHandle(good, 0) == NULL);
}
#endif

int main(int argc, char** argv) {
    Queries();
#ifdef PIXIE_DEVELOPMENT
    CHECK(argc == 7);
    if (argc == 7) Modules(argv + 1);
#else
    CHECK(argc == 1);
    (void)argv;
#endif
    printf("%s: %d SDK identity/query/admission checks (%d failures)\n", failures ? "FAIL" : "PASS", checks, failures);
    return failures ? 1 : 0;
}
