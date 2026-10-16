/*
 * Drives one game's C API library the way an editor host does: loads it at run time, creates a
 * hidden window with an OpenGL 4.5 core context, and runs a session through every part of the API.
 * Written in C and including only pixie_capi.h from the engine, so it proves the API is C.
 *
 * usage: capi-test <path to a game's capi library>
 *        capi-test <path to a capi library> --project <folder> [--expect-type <name>]
 *        capi-test <path to a game's capi library> --open <folder>
 * The second form creates a project in <folder> (removed before and after) and drives it through
 * editing, saving, playing, and stopping; with --expect-type, it also checks the library describes
 * that component type, such as a game's own. The third form opens an existing project with the
 * game's library, plays it, and stops, changing nothing in the folder. Exits 0 when every check
 * passes, 1 otherwise.
 */
#include "pixie_capi.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <direct.h>
#include <windows.h>
#define RemoveFolder _rmdir
typedef HMODULE Library;
static Library OpenLibrary(const char* path) { return LoadLibraryA(path); }
static void* Symbol(Library library, const char* name) { return (void*)GetProcAddress(library, name); }
static void CloseLibrary(Library library) { FreeLibrary(library); }
#else
#include <dlfcn.h>
#include <unistd.h>
#define RemoveFolder rmdir
typedef void* Library;
static Library OpenLibrary(const char* path) { return dlopen(path, RTLD_NOW); }
static void* Symbol(Library library, const char* name) { return dlsym(library, name); }
static void CloseLibrary(Library library) { dlclose(library); }
#endif

/* After windows.h, which defines APIENTRY itself. */
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

/* The API, looked up by name, as P/Invoke does. */
static struct {
    int32_t (*api_version)(void);
    int32_t (*last_error)(char*, int32_t);
    PixieResult (*session_create)(int32_t, int32_t, uint64_t, PixieSession**);
    void (*session_destroy)(PixieSession*);
    PixieResult (*project_name)(PixieSession*, char*, int32_t);
    PixieResult (*session_resize)(PixieSession*, int32_t, int32_t);
    PixieResult (*session_advance)(PixieSession*, double);
    PixieResult (*session_step)(PixieSession*);
    PixieResult (*session_set_paused)(PixieSession*, int32_t);
    PixieResult (*session_reset)(PixieSession*);
    PixieResult (*session_set_time_scale)(PixieSession*, double);
    uint32_t (*type_flags)(PixieSession*, int32_t);
    PixieResult (*session_render)(PixieSession*, uint32_t*);
    PixieResult (*session_stats)(PixieSession*, PixieStats*);
    PixieResult (*input_cursor)(PixieSession*, double, double);
    PixieResult (*input_key)(PixieSession*, int32_t, int32_t);
    PixieResult (*entities)(PixieSession*, uint32_t*, int32_t, int32_t*);
    PixieResult (*entity_name)(PixieSession*, uint32_t, char*, int32_t);
    int32_t (*entity_alive)(PixieSession*, uint32_t);
    int32_t (*type_count)(PixieSession*);
    PixieResult (*type_name)(PixieSession*, int32_t, char*, int32_t);
    int32_t (*entity_has)(PixieSession*, uint32_t, int32_t);
    int32_t (*field_count)(PixieSession*, int32_t);
    PixieResult (*field_name)(PixieSession*, int32_t, int32_t, char*, int32_t);
    PixieResult (*field_kind)(PixieSession*, int32_t, int32_t, int32_t*);
    PixieResult (*field_info)(PixieSession*, int32_t, int32_t, PixieFieldInfo*);
    PixieResult (*field_text)(PixieSession*, int32_t, int32_t, int32_t, char*, int32_t);
    PixieResult (*field_get)(PixieSession*, uint32_t, int32_t, int32_t, PixieValue*);
    PixieResult (*field_set)(PixieSession*, uint32_t, int32_t, int32_t, const PixieValue*);
    PixieResult (*entity_destroy)(PixieSession*, uint32_t);
    PixieResult (*pick)(PixieSession*, double, double, double, uint32_t*);
    PixieResult (*pick_box)(PixieSession*, double, double, double, double, uint32_t*, int32_t, int32_t*);
    PixieResult (*entity_view_bounds)(PixieSession*, uint32_t, double*, double*, double*);
    PixieResult (*particle_remove_at)(PixieSession*, double, double, double);
    PixieResult (*spawn_at)(PixieSession*, double, double, int32_t*);
    PixieResult (*project_create)(const char*);
    PixieResult (*session_open_project)(int32_t, int32_t, const char*, PixieSession**);
    PixieResult (*project_save)(PixieSession*);
    int32_t (*project_unsaved)(PixieSession*);
    int32_t (*session_mode)(PixieSession*);
    PixieResult (*session_play)(PixieSession*);
    PixieResult (*session_stop)(PixieSession*);
    PixieResult (*entity_create)(PixieSession*, const char*, uint32_t*);
    PixieResult (*type_find)(PixieSession*, const char*, int32_t*);
    PixieResult (*component_add)(PixieSession*, uint32_t, int32_t);
    PixieResult (*component_remove)(PixieSession*, uint32_t, int32_t);
    PixieResult (*view_to_world)(PixieSession*, double, double, double*, double*);
    PixieResult (*world_to_view)(PixieSession*, double, double, double*, double*);
    PixieResult (*entity_set_name)(PixieSession*, uint32_t, const char*);
    PixieResult (*entity_scene_id)(PixieSession*, uint32_t, uint64_t*);
    PixieResult (*entity_find)(PixieSession*, uint64_t, uint32_t*);
    PixieResult (*entities_snapshot)(PixieSession*, const uint32_t*, int32_t, char*, int32_t, int32_t*);
    PixieResult (*entities_restore)(PixieSession*, const char*, int32_t, uint32_t*, int32_t, int32_t*);
    PixieResult (*entity_view_quad)(PixieSession*, uint32_t, double*);
    PixieResult (*field_get_text)(PixieSession*, uint32_t, int32_t, int32_t, char*, int32_t, int32_t*);
    PixieResult (*field_set_text)(PixieSession*, uint32_t, int32_t, int32_t, const char*);
    PixieResult (*image)(PixieSession*, const char*, uint32_t*, int32_t*, int32_t*);
    PixieResult (*entity_view_path)(PixieSession*, uint32_t, double*, int32_t, int32_t*);
    PixieResult (*field_get_points)(PixieSession*, uint32_t, int32_t, int32_t, double*, int32_t, int32_t*);
    PixieResult (*field_set_points)(PixieSession*, uint32_t, int32_t, int32_t, const double*, int32_t);
} api;

static int failures = 0;

#define CHECK(expression)                                                                          \
    do {                                                                                           \
        if (!(expression)) {                                                                       \
            char error[256];                                                                       \
            error[0] = '\0';                                                                       \
            if (api.last_error != NULL) {                                                          \
                api.last_error(error, (int32_t)sizeof error);                                      \
            }                                                                                      \
            fprintf(stderr, "%s(%d): check failed: %s [last error: %s]\n", __FILE__, __LINE__,     \
                    #expression, error);                                                           \
            ++failures;                                                                            \
        }                                                                                          \
    } while (0)

static int Load(Library library) {
    int missing = 0;
#define LOAD(field, name)                                                                          \
    *(void**)&api.field = Symbol(library, name);                                                   \
    if (api.field == NULL) {                                                                       \
        fprintf(stderr, "missing export: %s\n", name);                                             \
        missing = 1;                                                                               \
    }
    LOAD(api_version, "pixie_api_version")
    LOAD(last_error, "pixie_last_error")
    LOAD(session_create, "pixie_session_create")
    LOAD(session_destroy, "pixie_session_destroy")
    LOAD(project_name, "pixie_project_name")
    LOAD(session_resize, "pixie_session_resize")
    LOAD(session_advance, "pixie_session_advance")
    LOAD(session_step, "pixie_session_step")
    LOAD(session_set_paused, "pixie_session_set_paused")
    LOAD(session_reset, "pixie_session_reset")
    LOAD(session_set_time_scale, "pixie_session_set_time_scale")
    LOAD(type_flags, "pixie_type_flags")
    LOAD(session_render, "pixie_session_render")
    LOAD(session_stats, "pixie_session_stats")
    LOAD(input_cursor, "pixie_input_cursor")
    LOAD(input_key, "pixie_input_key")
    LOAD(entities, "pixie_entities")
    LOAD(entity_name, "pixie_entity_name")
    LOAD(entity_alive, "pixie_entity_alive")
    LOAD(type_count, "pixie_type_count")
    LOAD(type_name, "pixie_type_name")
    LOAD(entity_has, "pixie_entity_has")
    LOAD(field_count, "pixie_field_count")
    LOAD(field_name, "pixie_field_name")
    LOAD(field_kind, "pixie_field_kind")
    LOAD(field_info, "pixie_field_info")
    LOAD(field_text, "pixie_field_text")
    LOAD(field_get, "pixie_field_get")
    LOAD(field_set, "pixie_field_set")
    LOAD(entity_destroy, "pixie_entity_destroy")
    LOAD(pick, "pixie_pick")
    LOAD(pick_box, "pixie_pick_box")
    LOAD(entity_view_bounds, "pixie_entity_view_bounds")
    LOAD(particle_remove_at, "pixie_particle_remove_at")
    LOAD(spawn_at, "pixie_spawn_at")
    LOAD(project_create, "pixie_project_create")
    LOAD(session_open_project, "pixie_session_open_project")
    LOAD(project_save, "pixie_project_save")
    LOAD(project_unsaved, "pixie_project_unsaved")
    LOAD(session_mode, "pixie_session_mode")
    LOAD(session_play, "pixie_session_play")
    LOAD(session_stop, "pixie_session_stop")
    LOAD(entity_create, "pixie_entity_create")
    LOAD(type_find, "pixie_type_find")
    LOAD(component_add, "pixie_component_add")
    LOAD(component_remove, "pixie_component_remove")
    LOAD(view_to_world, "pixie_view_to_world")
    LOAD(world_to_view, "pixie_world_to_view")
    LOAD(entity_set_name, "pixie_entity_set_name")
    LOAD(entity_scene_id, "pixie_entity_scene_id")
    LOAD(entity_find, "pixie_entity_find")
    LOAD(entities_snapshot, "pixie_entities_snapshot")
    LOAD(entities_restore, "pixie_entities_restore")
    LOAD(entity_view_quad, "pixie_entity_view_quad")
    LOAD(field_get_text, "pixie_field_get_text")
    LOAD(field_set_text, "pixie_field_set_text")
    LOAD(image, "pixie_image")
    LOAD(entity_view_path, "pixie_entity_view_path")
    LOAD(field_get_points, "pixie_field_get_points")
    LOAD(field_set_points, "pixie_field_set_points")
#undef LOAD
    return !missing;
}

static int32_t FindType(PixieSession* s, const char* name) {
    char buffer[128];
    for (int32_t t = 0; t < api.type_count(s); ++t) {
        if (api.type_name(s, t, buffer, (int32_t)sizeof buffer) == PIXIE_OK && strcmp(buffer, name) == 0) {
            return t;
        }
    }
    return -1;
}

static int32_t FindField(PixieSession* s, int32_t type, const char* name) {
    char buffer[128];
    for (int32_t f = 0; f < api.field_count(s, type); ++f) {
        if (api.field_name(s, type, f, buffer, (int32_t)sizeof buffer) == PIXIE_OK && strcmp(buffer, name) == 0) {
            return f;
        }
    }
    return -1;
}

/* The entity named `name`; 0 after a failed check. */
static uint32_t FindEntity(PixieSession* s, const char* name) {
    int32_t count = 0;
    CHECK(api.entities(s, NULL, 0, &count) == PIXIE_OK);
    uint32_t* ids = (uint32_t*)malloc(sizeof(uint32_t) * (size_t)(count > 0 ? count : 1));
    /* Handle 0 is an entity too, so whether one was found is kept apart. */
    uint32_t entity = 0;
    int found = 0;
    char buffer[128];
    if (ids != NULL && api.entities(s, ids, count, &count) == PIXIE_OK) {
        for (int32_t i = 0; i < count && !found; ++i) {
            if (api.entity_name(s, ids[i], buffer, (int32_t)sizeof buffer) == PIXIE_OK && strcmp(buffer, name) == 0) {
                entity = ids[i];
                found = 1;
            }
        }
    }
    free(ids);
    CHECK(found);
    return entity;
}

/* A snapshot of `count` entities as text, allocated; the caller frees it. */
static char* Snapshot(PixieSession* s, const uint32_t* entities, int32_t count) {
    int32_t length = 0;
    CHECK(api.entities_snapshot(s, entities, count, NULL, 0, &length) == PIXIE_OK && length > 0);
    char* text = (char*)malloc((size_t)length + 1);
    int32_t again = 0;
    CHECK(text != NULL && api.entities_snapshot(s, entities, count, text, length + 1, &again) == PIXIE_OK &&
          again == length);
    return text;
}

/* The one GL function the test calls itself, loaded through GLFW, with the constants it needs. */
#if defined(_WIN32)
#define TEST_GLAPI __stdcall
#else
#define TEST_GLAPI
#endif
#define TEST_GL_RGBA 0x1908
#define TEST_GL_UNSIGNED_BYTE 0x1401
typedef void(TEST_GLAPI* GetTextureImage)(unsigned int, int, unsigned int, unsigned int, int, void*);

/* True when the texture's pixels are not all the same: the World was drawn into it. */
static int TextureHasContent(uint32_t texture, int width, int height) {
    GetTextureImage getTextureImage = (GetTextureImage)glfwGetProcAddress("glGetTextureImage");
    if (getTextureImage == NULL) {
        return 0;
    }
    const size_t size = (size_t)width * (size_t)height * 4;
    unsigned char* pixels = (unsigned char*)malloc(size);
    if (pixels == NULL) {
        return 0;
    }
    getTextureImage(texture, 0, TEST_GL_RGBA, TEST_GL_UNSIGNED_BYTE, (int)size, pixels);
    int differs = 0;
    for (size_t i = 4; i < size && !differs; i += 4) {
        differs = memcmp(pixels, pixels + i, 3) != 0;
    }
    free(pixels);
    return differs;
}

/* The pixel at (x, y) of the texture, in view pixels from the top-left; the texture's first row is
 * the bottom. */
static void TexturePixel(uint32_t texture, int width, int height, int x, int y, unsigned char rgba[4]) {
    memset(rgba, 0, 4);
    GetTextureImage getTextureImage = (GetTextureImage)glfwGetProcAddress("glGetTextureImage");
    const size_t size = (size_t)width * (size_t)height * 4;
    unsigned char* pixels = (unsigned char*)malloc(size);
    if (getTextureImage == NULL || pixels == NULL) {
        free(pixels);
        return;
    }
    getTextureImage(texture, 0, TEST_GL_RGBA, TEST_GL_UNSIGNED_BYTE, (int)size, pixels);
    memcpy(rgba, pixels + ((size_t)(height - 1 - y) * (size_t)width + (size_t)x) * 4, 4);
    free(pixels);
}

/* A 4 x 4 PNG, all of one opaque color: red 40, green 200, blue 120. */
static const unsigned char kGreenPng[] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x04, 0x08, 0x06, 0x00, 0x00, 0x00, 0xa9, 0xf1, 0x9e, 0x7e, 0x00, 0x00, 0x00, 0x12, 0x49,
    0x44, 0x41, 0x54, 0x78, 0xda, 0x63, 0xd0, 0x38, 0x51, 0xf1, 0x1f, 0x19, 0x33, 0x90, 0x2e, 0x00, 0x00, 0x09, 0xc7,
    0x26, 0x71, 0xde, 0x34, 0xac, 0x79, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82};

static void Exercise(PixieSession* s) {
    char name[128];
    CHECK(api.project_name(s, name, (int32_t)sizeof name) == PIXIE_OK && name[0] != '\0');
    printf("project: %s\n", name);

    /* Types and fields. */
    const int32_t transform = FindType(s, "Transform2D");
    CHECK(transform >= 0);
    const int32_t position = FindField(s, transform, "position");
    CHECK(position >= 0);
    int32_t kind = -1;
    CHECK(api.field_kind(s, transform, position, &kind) == PIXIE_OK && kind == PIXIE_FIELD_POINT);
    printf("component types: %d\n", api.type_count(s));
    int32_t foundType = -1;
    CHECK(api.type_find(s, "Transform2D", &foundType) == PIXIE_OK && foundType == transform);
    CHECK(api.type_find(s, "NoSuchType", &foundType) == PIXIE_ERROR_NOT_FOUND);

    /* Without a project the session plays, and there is no scene to play, stop, or save. */
    CHECK(api.session_mode(s) == PIXIE_MODE_PLAY);
    CHECK(api.session_play(s) == PIXIE_ERROR_ARGUMENT && api.session_stop(s) == PIXIE_ERROR_ARGUMENT);
    CHECK(api.project_save(s) == PIXIE_ERROR_ARGUMENT && api.project_unsaved(s) == 0);

    /* Metadata: Transform2D.rotation is shown in degrees, with a range and scrubbing. */
    const int32_t rotation = FindField(s, transform, "rotation");
    PixieFieldInfo info = {0};
    char text[256];
    CHECK(sizeof(PixieFieldInfo) == 72);
    CHECK(rotation >= 0 && api.field_info(s, transform, rotation, &info) == PIXIE_OK);
    CHECK(info.kind == PIXIE_FIELD_DOUBLE && (info.flags & PIXIE_FIELD_HAS_UNIT) != 0);
    CHECK(fabs(info.unitFactor - 57.29577951308232) < 1e-9 && info.scrub > 0.0);
    CHECK((info.flags & PIXIE_FIELD_HAS_RANGE) != 0 && info.rangeMin < info.rangeMax);
    CHECK(api.field_text(s, transform, rotation, PIXIE_TEXT_UNIT_SUFFIX, text, (int32_t)sizeof text) == PIXIE_OK &&
          strcmp(text, "\xC2\xB0") == 0);
    CHECK(api.field_text(s, transform, rotation, PIXIE_TEXT_LABEL, text, (int32_t)sizeof text) == PIXIE_OK &&
          strcmp(text, "rotation") == 0);
    CHECK(api.field_text(s, transform, rotation, 99, text, (int32_t)sizeof text) == PIXIE_ERROR_ARGUMENT);
    /* An entity reference: ParticleSpawner.system names the type its choices have. */
    const int32_t spawner = FindType(s, "ParticleSpawner");
    const int32_t system = spawner >= 0 ? FindField(s, spawner, "system") : -1;
    CHECK(system >= 0 && api.field_info(s, spawner, system, &info) == PIXIE_OK && info.kind == PIXIE_FIELD_ENTITY);
    CHECK(api.field_text(s, spawner, system, PIXIE_TEXT_REFERENCES, text, (int32_t)sizeof text) == PIXIE_OK &&
          strcmp(text, "ParticleSystem") == 0);
    /* Every field of every type carries a tooltip. */
    for (int32_t t = 0; t < api.type_count(s); ++t) {
        for (int32_t f = 0; f < api.field_count(s, t); ++f) {
            const int ok = api.field_text(s, t, f, PIXIE_TEXT_TOOLTIP, text, (int32_t)sizeof text) == PIXIE_OK;
            CHECK(ok && text[0] != '\0');
        }
    }

    /* Entities: find one with a Transform2D and a name or not. */
    int32_t count = 0;
    CHECK(api.entities(s, NULL, 0, &count) == PIXIE_OK && count > 0);
    uint32_t* ids = (uint32_t*)malloc(sizeof(uint32_t) * (size_t)count);
    int32_t filled = 0;
    CHECK(ids != NULL && api.entities(s, ids, count, &filled) == PIXIE_OK && filled == count);
    uint32_t moving = 0;
    int found = 0;
    for (int32_t i = 0; i < count && !found; ++i) {
        if (api.entity_has(s, ids[i], transform)) {
            moving = ids[i];
            found = 1;
        }
    }
    CHECK(found);
    CHECK(api.entity_name(s, ids[0], name, (int32_t)sizeof name) == PIXIE_OK);
    printf("entities: %d, first named \"%s\"\n", count, name);

    /* Create an entity, add and remove a component, and destroy it. */
    int32_t shapeType = -1;
    CHECK(api.type_find(s, "Shape", &shapeType) == PIXIE_OK);
    uint32_t made = 0;
    CHECK(api.entity_create(s, "made", &made) == PIXIE_OK && api.entity_alive(s, made) == 1);
    CHECK(api.component_add(s, made, shapeType) == PIXIE_OK && api.entity_has(s, made, shapeType) == 1);
    CHECK(api.component_remove(s, made, shapeType) == PIXIE_OK && api.entity_has(s, made, shapeType) == 0);
    CHECK(api.component_remove(s, made, shapeType) == PIXIE_ERROR_NOT_FOUND);
    CHECK(api.component_add(s, made, 9999) == PIXIE_ERROR_ARGUMENT);

    /* Its scene ID finds it; a rename shows; a snapshot brings it back, under the same ID, after it is
     * destroyed. */
    uint64_t sceneId = 0;
    uint32_t back = 0;
    CHECK(api.entity_scene_id(s, made, &sceneId) == PIXIE_OK && sceneId != 0);
    CHECK(api.entity_find(s, sceneId, &back) == PIXIE_OK && back == made);
    CHECK(api.entity_set_name(s, made, "renamed") == PIXIE_OK);
    CHECK(api.entity_name(s, made, name, (int32_t)sizeof name) == PIXIE_OK && strcmp(name, "renamed") == 0);
    CHECK(api.component_add(s, made, shapeType) == PIXIE_OK);
    char* snapshot = Snapshot(s, &made, 1);
    char small[8];
    int32_t length = 0;
    CHECK(api.entities_snapshot(s, &made, 1, small, (int32_t)sizeof small, &length) == PIXIE_ERROR_BUFFER);
    CHECK(api.entities_restore(s, snapshot, PIXIE_RESTORE_SAME_IDS, &back, 1, &count) == PIXIE_ERROR_ARGUMENT);
    CHECK(api.entity_destroy(s, made) == PIXIE_OK);
    CHECK(api.entity_find(s, sceneId, &back) == PIXIE_ERROR_NOT_FOUND);
    CHECK(api.entities_restore(s, snapshot, PIXIE_RESTORE_SAME_IDS, &back, 1, &count) == PIXIE_OK && count == 1);
    uint64_t restoredId = 0;
    CHECK(api.entity_scene_id(s, back, &restoredId) == PIXIE_OK && restoredId == sceneId);
    CHECK(api.entity_name(s, back, name, (int32_t)sizeof name) == PIXIE_OK && strcmp(name, "renamed") == 0);
    CHECK(api.entity_has(s, back, shapeType) == 1);
    CHECK(api.entity_set_name(s, back, NULL) == PIXIE_OK);
    CHECK(api.entity_name(s, back, name, (int32_t)sizeof name) == PIXIE_OK && name[0] == '\0');
    free(snapshot);
    made = back;
    CHECK(api.entity_destroy(s, made) == PIXIE_OK);
    CHECK(api.component_add(s, made, shapeType) == PIXIE_ERROR_NOT_FOUND);

    /* Write a field and read it back. */
    const PixieValue target = {12.5, -3.25, 0.0, 0.0};
    PixieValue read;
    CHECK(api.field_set(s, moving, transform, position, &target) == PIXIE_OK);
    CHECK(api.field_get(s, moving, transform, position, &read) == PIXIE_OK);
    CHECK(read.x == 12.5 && read.y == -3.25);

    /* A field of the game's own: every project describes at least one type after the runtime's. */
    const int32_t own = api.type_count(s) - 1;
    CHECK(api.type_name(s, own, name, (int32_t)sizeof name) == PIXIE_OK);
    printf("last type: %s\n", name);

    /* Time: 0.2 s of wall time is 12 steps at 60 per second; a frame longer than 0.25 s counts as
     * 0.25 s, so 1 s more adds 15. */
    PixieStats stats;
    CHECK(api.input_cursor(s, 320.0, 180.0) == PIXIE_OK);
    CHECK(api.session_advance(s, 0.2) == PIXIE_OK);
    CHECK(api.session_stats(s, &stats) == PIXIE_OK && stats.steps == 12);
    CHECK(api.session_advance(s, 1.0) == PIXIE_OK);
    CHECK(api.session_stats(s, &stats) == PIXIE_OK && stats.steps == 27);
    printf("after 1.2 s: %llu steps, %.3f ms last step\n", (unsigned long long)stats.steps,
           stats.lastStepMilliseconds);

    /* Pause: time does not advance, a single step does. */
    CHECK(api.session_set_paused(s, 1) == PIXIE_OK);
    CHECK(api.session_advance(s, 0.2) == PIXIE_OK);
    CHECK(api.session_stats(s, &stats) == PIXIE_OK && stats.steps == 27 && stats.paused == 1);
    CHECK(api.session_step(s) == PIXIE_OK);
    CHECK(api.session_stats(s, &stats) == PIXIE_OK && stats.steps == 28);
    CHECK(api.session_set_paused(s, 0) == PIXIE_OK);

    /* Time scale: at 0.25, 0.2 s of wall time is 3 steps; at 2, 0.1 s is 12. */
    CHECK(api.session_set_time_scale(s, 0.0) == PIXIE_ERROR_ARGUMENT);
    CHECK(api.session_set_time_scale(s, 0.25) == PIXIE_OK);
    CHECK(api.session_advance(s, 0.2) == PIXIE_OK);
    CHECK(api.session_stats(s, &stats) == PIXIE_OK && stats.steps == 31 && stats.timeScale == 0.25);
    CHECK(api.session_set_time_scale(s, 2.0) == PIXIE_OK);
    CHECK(api.session_advance(s, 0.1) == PIXIE_OK);
    CHECK(api.session_stats(s, &stats) == PIXIE_OK && stats.steps == 43);
    CHECK(api.session_set_time_scale(s, 1.0) == PIXIE_OK);

    /* Every game marks one scenario type. */
    int scenarios = 0;
    for (int32_t t = 0; t < api.type_count(s); ++t) {
        scenarios += (api.type_flags(s, t) & PIXIE_TYPE_SCENARIO) != 0;
    }
    CHECK(scenarios == 1);

    /* A key press goes through without error. */
    CHECK(api.input_key(s, 32, 1) == PIXIE_OK && api.input_key(s, 32, 0) == PIXIE_OK);
    CHECK(api.session_advance(s, 1.0 / 60.0) == PIXIE_OK);

    /* Render into the session's texture, at a new size. */
    uint32_t texture = 0;
    CHECK(api.session_resize(s, 320, 200) == PIXIE_OK);
    CHECK(api.session_render(s, &texture) == PIXIE_OK && texture != 0);
    CHECK(TextureHasContent(texture, 320, 200));

    /* Errors: a bad type, a stale entity, a short buffer. */
    CHECK(api.field_get(s, moving, 9999, 0, &read) == PIXIE_ERROR_ARGUMENT);
    char tiny[3];
    CHECK(api.project_name(s, tiny, (int32_t)sizeof tiny) == PIXIE_ERROR_BUFFER && strlen(tiny) == 2);

    /* Picking, paused so the view shows the current positions: the first entity that can be picked,
     * found again at its center and in a box around it, then destroyed; a spawn at the same place;
     * and, in the particles game, the particle spawned there removed. */
    CHECK(api.session_set_paused(s, 1) == PIXIE_OK);
    int32_t total = 0;
    CHECK(api.entities(s, NULL, 0, &total) == PIXIE_OK);
    uint32_t* all = (uint32_t*)malloc(sizeof(uint32_t) * (size_t)total);
    CHECK(api.entities(s, all, total, &total) == PIXIE_OK);
    uint32_t pickable = 0;
    double px = 0.0, py = 0.0, pr = 0.0;
    int pickableFound = 0;
    for (int32_t i = 0; i < total && !pickableFound; ++i) {
        if (api.entity_view_bounds(s, all[i], &px, &py, &pr) == PIXIE_OK) {
            pickable = all[i];
            pickableFound = 1;
        }
    }
    free(all);
    CHECK(pickableFound && pr > 0.0);
    uint32_t picked = 0;
    CHECK(api.pick(s, px, py, 0.0, &picked) == PIXIE_OK && picked == pickable);
    /* The view's point turned into world units lands on the entity's transform, and back. */
    double wx = 0.0, wy = 0.0, vx = 0.0, vy = 0.0;
    PixieValue at;
    CHECK(api.view_to_world(s, px, py, &wx, &wy) == PIXIE_OK);
    CHECK(api.field_get(s, pickable, transform, position, &at) == PIXIE_OK);
    CHECK(fabs(wx - at.x) < 1e-6 && fabs(wy - at.y) < 1e-6);
    CHECK(api.world_to_view(s, wx, wy, &vx, &vy) == PIXIE_OK && fabs(vx - px) < 1e-6 && fabs(vy - py) < 1e-6);
    CHECK(api.view_to_world(s, px, py, NULL, &wy) == PIXIE_ERROR_ARGUMENT);
    CHECK(api.pick(s, -100000.0, -100000.0, 0.0, &picked) == PIXIE_ERROR_NOT_FOUND);
    CHECK(api.pick(s, px, py, 0.0, NULL) == PIXIE_ERROR_ARGUMENT);
    uint32_t boxed[64];
    int32_t inBox = 0;
    CHECK(api.pick_box(s, px + 0.5, py + 0.5, px - 0.5, py - 0.5, boxed, 64, &inBox) == PIXIE_OK && inBox >= 1);
    int boxedIt = 0;
    for (int32_t i = 0; i < inBox && i < 64; ++i) {
        boxedIt |= boxed[i] == pickable;
    }
    CHECK(boxedIt);
    CHECK(api.entity_destroy(s, pickable) == PIXIE_OK && api.entity_alive(s, pickable) == 0);
    CHECK(api.entity_destroy(s, pickable) == PIXIE_ERROR_NOT_FOUND);
    CHECK(api.entity_view_bounds(s, pickable, &px, &py, &pr) == PIXIE_ERROR_NOT_FOUND);
    int32_t created = 0;
    CHECK(api.spawn_at(s, px, py, &created) == PIXIE_OK && created == 1);
    char game[64];
    CHECK(api.project_name(s, game, (int32_t)sizeof game) == PIXIE_OK);
    const int particles = strcmp(game, "particles") == 0;
    CHECK(api.particle_remove_at(s, px, py, 1.0) == (particles ? PIXIE_OK : PIXIE_ERROR_NOT_FOUND));
    CHECK(api.particle_remove_at(s, px, py, -1.0) == PIXIE_ERROR_ARGUMENT);
    CHECK(api.session_set_paused(s, 0) == PIXIE_OK);
    printf("picked %u at (%.1f, %.1f), radius %.1f px\n", pickable, px, py, pr);

    /* Reset: a new World, with its own time, and the old handles stale or new. */
    CHECK(api.session_reset(s) == PIXIE_OK);
    CHECK(api.session_stats(s, &stats) == PIXIE_OK && stats.steps == 0 && stats.entities > 0);
    printf("after reset: %d entities\n", stats.entities);

    free(ids);
}

/* The last part of a folder's path: a new project's name. */
static const char* BaseName(const char* folder) {
    const char* name = folder;
    for (const char* c = folder; *c != '\0'; ++c) {
        if (*c == '/' || *c == '\\') {
            name = c + 1;
        }
    }
    return name;
}

static void Cleanup(const char* folder) {
    char path[1024];
    const char* name = BaseName(folder);
    snprintf(path, sizeof path, "%s/scenes/main.scene", folder);
    remove(path);
    snprintf(path, sizeof path, "%s/scenes/main.scene.saving", folder);
    remove(path);
    snprintf(path, sizeof path, "%s/scenes", folder);
    RemoveFolder(path);
    snprintf(path, sizeof path, "%s/%s.pixie", folder, name);
    remove(path);
    snprintf(path, sizeof path, "%s/green.png", folder);
    remove(path);
    snprintf(path, sizeof path, "%s/inter.ttf", folder);
    remove(path);
    RemoveFolder(folder);
}

static int SameColor(const unsigned char* rgba, int r, int g, int b) {
    return abs(rgba[0] - r) <= 2 && abs(rgba[1] - g) <= 2 && abs(rgba[2] - b) <= 2;
}

/* A path in the project, after ExerciseSprite wrote green.png: its points are a list field; it is
 * drawn in its fill color up to the fill and in its color after it, over a sprite of a negative layer
 * and under one of layer 0; it is picked along its line, outlined, saved, and opened again. */
static void ExercisePath(const char* folder) {
    PixieSession* s = NULL;
    CHECK(api.session_open_project(640, 360, folder, &s) == PIXIE_OK && s != NULL);
    if (s == NULL) {
        return;
    }
    int32_t transform = -1, pathType = -1, spriteType = -1;
    CHECK(api.type_find(s, "Transform2D", &transform) == PIXIE_OK && api.type_find(s, "Path", &pathType) == PIXIE_OK &&
          api.type_find(s, "Sprite", &spriteType) == PIXIE_OK);
    const int32_t points = FindField(s, pathType, "points");
    int32_t kind = -1;
    CHECK(api.field_kind(s, pathType, points, &kind) == PIXIE_OK && kind == PIXIE_FIELD_POINT_LIST);

    uint32_t wire = 0;
    CHECK(api.entity_create(s, "wire", &wire) == PIXIE_OK);
    CHECK(api.component_add(s, wire, transform) == PIXIE_OK && api.component_add(s, wire, pathType) == PIXIE_OK);
    PixieValue value;
    CHECK(api.field_get(s, wire, pathType, points, &value) == PIXIE_ERROR_ARGUMENT);
    CHECK(api.field_get_points(s, wire, transform, FindField(s, transform, "position"), NULL, 0, &kind) ==
          PIXIE_ERROR_ARGUMENT);

    /* 200 units long, at y = 120, filled up to its middle (world x = -200). The view shows world (x, y)
     * at (x + 320, y + 180). */
    const double line[] = {-300.0, 120.0, -100.0, 120.0};
    CHECK(api.field_set_points(s, wire, pathType, points, line, 2) == PIXIE_OK);
    int32_t count = 0;
    double read[4] = {0.0, 0.0, 0.0, 0.0};
    CHECK(api.field_get_points(s, wire, pathType, points, NULL, 0, &count) == PIXIE_OK && count == 2);
    CHECK(api.field_get_points(s, wire, pathType, points, read, 1, &count) == PIXIE_OK && count == 2 &&
          read[0] == -300.0 && read[1] == 120.0 && read[2] == 0.0);
    const PixieValue width = {10.0, 0.0, 0.0, 0.0};
    const PixieValue blue = {0.0, 0.0, 255.0, 255.0};
    const PixieValue red = {255.0, 0.0, 0.0, 255.0};
    const PixieValue half = {0.5, 0.0, 0.0, 0.0};
    CHECK(api.field_set(s, wire, pathType, FindField(s, pathType, "width"), &width) == PIXIE_OK);
    CHECK(api.field_set(s, wire, pathType, FindField(s, pathType, "color"), &blue) == PIXIE_OK);
    CHECK(api.field_set(s, wire, pathType, FindField(s, pathType, "fillColor"), &red) == PIXIE_OK);
    CHECK(api.field_set(s, wire, pathType, FindField(s, pathType, "fill"), &half) == PIXIE_OK);

    uint32_t texture = 0;
    unsigned char rgba[4];
    CHECK(api.session_render(s, &texture) == PIXIE_OK);
    TexturePixel(texture, 640, 360, 70, 300, rgba);
    CHECK(SameColor(rgba, 255, 0, 0));
    TexturePixel(texture, 640, 360, 170, 300, rgba);
    CHECK(SameColor(rgba, 0, 0, 255));
    TexturePixel(texture, 640, 360, 170, 310, rgba);
    CHECK(!SameColor(rgba, 0, 0, 255));

    uint32_t picked = 0;
    CHECK(api.pick(s, 120.0, 303.0, 0.0, &picked) == PIXIE_OK && picked == wire);
    CHECK(api.pick(s, 120.0, 310.0, 0.0, &picked) == PIXIE_ERROR_NOT_FOUND);
    double outline[8];
    CHECK(api.entity_view_path(s, wire, outline, 4, &count) == PIXIE_OK && count == 2 && fabs(outline[0] - 20.0) < 1e-6 &&
          fabs(outline[1] - 300.0) < 1e-6 && fabs(outline[2] - 220.0) < 1e-6);
    double x = 0.0, y = 0.0, radius = 0.0;
    CHECK(api.entity_view_bounds(s, wire, &x, &y, &radius) == PIXIE_OK && fabs(x - 120.0) < 1e-6 &&
          fabs(radius - 100.0) < 1e-6);

    /* A sprite on a negative layer is drawn under the path; on layer 0, over it. */
    uint32_t sprite = 0;
    CHECK(api.entity_create(s, "under", &sprite) == PIXIE_OK && api.component_add(s, sprite, transform) == PIXIE_OK &&
          api.component_add(s, sprite, spriteType) == PIXIE_OK);
    CHECK(api.field_set_text(s, sprite, spriteType, FindField(s, spriteType, "image"), "green.png") == PIXIE_OK);
    const PixieValue at = {-150.0, 120.0, 0.0, 0.0};
    const PixieValue scale = {5.0, 5.0, 0.0, 0.0};
    const PixieValue below = {-1.0, 0.0, 0.0, 0.0};
    const PixieValue above = {0.0, 0.0, 0.0, 0.0};
    CHECK(api.field_set(s, sprite, transform, FindField(s, transform, "position"), &at) == PIXIE_OK);
    CHECK(api.field_set(s, sprite, transform, FindField(s, transform, "scale"), &scale) == PIXIE_OK);
    CHECK(api.field_set(s, sprite, spriteType, FindField(s, spriteType, "layer"), &below) == PIXIE_OK);
    CHECK(api.session_render(s, &texture) == PIXIE_OK);
    TexturePixel(texture, 640, 360, 170, 300, rgba);
    CHECK(SameColor(rgba, 0, 0, 255));
    TexturePixel(texture, 640, 360, 170, 308, rgba);
    CHECK(SameColor(rgba, 40, 200, 120));
    CHECK(api.field_set(s, sprite, spriteType, FindField(s, spriteType, "layer"), &above) == PIXIE_OK);
    CHECK(api.session_render(s, &texture) == PIXIE_OK);
    TexturePixel(texture, 640, 360, 170, 300, rgba);
    CHECK(SameColor(rgba, 40, 200, 120));
    printf("path pixels: fill, color, and sprite over it as expected\n");

    CHECK(api.project_save(s) == PIXIE_OK);
    api.session_destroy(s);
    s = NULL;
    CHECK(api.session_open_project(640, 360, folder, &s) == PIXIE_OK && s != NULL);
    if (s != NULL) {
        wire = FindEntity(s, "wire");
        CHECK(api.field_get_points(s, wire, pathType, points, read, 2, &count) == PIXIE_OK && count == 2 &&
              read[2] == -100.0 && read[3] == 120.0);
        api.session_destroy(s);
    }
}

/* Copies the file at `from` to `to`; 0 when it cannot. */
static int CopyBytes(const char* from, const char* to) {
    FILE* in = fopen(from, "rb");
    FILE* out = in != NULL ? fopen(to, "wb") : NULL;
    int ok = in != NULL && out != NULL;
    char buffer[4096];
    size_t n = 0;
    while (ok && (n = fread(buffer, 1, sizeof buffer, in)) > 0) {
        ok = fwrite(buffer, 1, n, out) == n;
    }
    if (in != NULL) {
        fclose(in);
    }
    if (out != NULL) {
        fclose(out);
    }
    return ok;
}

/* A text in the project, with the test's copy of Inter as inter.ttf: drawn in its color inside its
 * outline and nowhere else; over a sprite of a lower layer and under one of a higher; picked; and saved. */
static void ExerciseText(const char* folder) {
    char path[1024];
    snprintf(path, sizeof path, "%s/inter.ttf", folder);
    CHECK(CopyBytes(PIXIE_TEST_FONT, path));
    PixieSession* s = NULL;
    CHECK(api.session_open_project(640, 360, folder, &s) == PIXIE_OK && s != NULL);
    if (s == NULL) {
        return;
    }
    int32_t transform = -1, textType = -1, spriteType = -1;
    CHECK(api.type_find(s, "Transform2D", &transform) == PIXIE_OK && api.type_find(s, "Text", &textType) == PIXIE_OK &&
          api.type_find(s, "Sprite", &spriteType) == PIXIE_OK);
    char asset[16];
    CHECK(api.field_text(s, textType, FindField(s, textType, "font"), PIXIE_TEXT_ASSET, asset, (int32_t)sizeof asset) ==
              PIXIE_OK &&
          strcmp(asset, "ttf") == 0);

    /* "HELLO", 48 units, magenta, centered at world (0, -120): the view's (320, 60). */
    uint32_t sign = 0;
    CHECK(api.entity_create(s, "sign", &sign) == PIXIE_OK && api.component_add(s, sign, transform) == PIXIE_OK &&
          api.component_add(s, sign, textType) == PIXIE_OK);
    const PixieValue at = {0.0, -120.0, 0.0, 0.0};
    const PixieValue size = {48.0, 0.0, 0.0, 0.0};
    const PixieValue magenta = {255.0, 0.0, 255.0, 255.0};
    CHECK(api.field_set(s, sign, transform, FindField(s, transform, "position"), &at) == PIXIE_OK);
    CHECK(api.field_set_text(s, sign, textType, FindField(s, textType, "text"), "HELLO") == PIXIE_OK);
    CHECK(api.field_set_text(s, sign, textType, FindField(s, textType, "font"), "inter.ttf") == PIXIE_OK);
    CHECK(api.field_set(s, sign, textType, FindField(s, textType, "size"), &size) == PIXIE_OK);
    CHECK(api.field_set(s, sign, textType, FindField(s, textType, "color"), &magenta) == PIXIE_OK);

    uint32_t texture = 0;
    CHECK(api.session_render(s, &texture) == PIXIE_OK);
    double corners[8];
    CHECK(api.entity_view_quad(s, sign, corners) == PIXIE_OK);
    const double left = corners[0], top = corners[1], right = corners[4], bottom = corners[5];
    CHECK(fabs((left + right) / 2.0 - 320.0) < 1e-6 && fabs((top + bottom) / 2.0 - 60.0) < 1e-6 && right - left > 48.0);

    /* Magenta inside the outline, none outside it; the lit pixel nearest the center is remembered. */
    GetTextureImage getTextureImage = (GetTextureImage)glfwGetProcAddress("glGetTextureImage");
    unsigned char* pixels = (unsigned char*)malloc(640 * 360 * 4);
    int inside = 0, outside = 0, px = -1, py = -1;
    double nearest = 1e9;
    if (getTextureImage != NULL && pixels != NULL) {
        getTextureImage(texture, 0, TEST_GL_RGBA, TEST_GL_UNSIGNED_BYTE, 640 * 360 * 4, pixels);
        for (int y = 0; y < 360; ++y) {
            for (int x = 0; x < 640; ++x) {
                const unsigned char* p = pixels + ((size_t)(359 - y) * 640 + (size_t)x) * 4;
                if (!(p[0] > 200 && p[1] < 60 && p[2] > 200)) {
                    continue;
                }
                const int in = x >= left - 1 && x <= right + 1 && y >= top - 1 && y <= bottom + 1;
                inside += in;
                outside += !in;
                const double d = fabs(x - 320.0) + fabs(y - 60.0);
                if (in && d < nearest) {
                    nearest = d;
                    px = x;
                    py = y;
                }
            }
        }
    }
    free(pixels);
    CHECK(inside > 100 && outside == 0 && px >= 0);
    printf("text pixels: %d inside its outline, %d outside\n", inside, outside);

    uint32_t picked = 0;
    CHECK(api.pick(s, px, py, 0.0, &picked) == PIXIE_OK && picked == sign);

    /* A sprite at the same place: on layer -1 the text lies over it, on layer 1 under it. */
    uint32_t sprite = 0;
    CHECK(api.entity_create(s, "plate", &sprite) == PIXIE_OK && api.component_add(s, sprite, transform) == PIXIE_OK &&
          api.component_add(s, sprite, spriteType) == PIXIE_OK);
    CHECK(api.field_set_text(s, sprite, spriteType, FindField(s, spriteType, "image"), "green.png") == PIXIE_OK);
    const PixieValue plateAt = {px - 320.0, py - 180.0, 0.0, 0.0};
    const PixieValue plateScale = {4.0, 4.0, 0.0, 0.0};
    const PixieValue below = {-1.0, 0.0, 0.0, 0.0};
    const PixieValue above = {1.0, 0.0, 0.0, 0.0};
    CHECK(api.field_set(s, sprite, transform, FindField(s, transform, "position"), &plateAt) == PIXIE_OK);
    CHECK(api.field_set(s, sprite, transform, FindField(s, transform, "scale"), &plateScale) == PIXIE_OK);
    CHECK(api.field_set(s, sprite, spriteType, FindField(s, spriteType, "layer"), &below) == PIXIE_OK);
    unsigned char rgba[4];
    CHECK(api.session_render(s, &texture) == PIXIE_OK);
    TexturePixel(texture, 640, 360, px, py, rgba);
    CHECK(rgba[0] > 200 && rgba[1] < 60 && rgba[2] > 200);
    CHECK(api.pick(s, px, py, 0.0, &picked) == PIXIE_OK && picked == sign);
    CHECK(api.field_set(s, sprite, spriteType, FindField(s, spriteType, "layer"), &above) == PIXIE_OK);
    CHECK(api.session_render(s, &texture) == PIXIE_OK);
    TexturePixel(texture, 640, 360, px, py, rgba);
    CHECK(SameColor(rgba, 40, 200, 120));
    CHECK(api.pick(s, px, py, 0.0, &picked) == PIXIE_OK && picked == sprite);

    CHECK(api.project_save(s) == PIXIE_OK);
    api.session_destroy(s);
    s = NULL;
    CHECK(api.session_open_project(640, 360, folder, &s) == PIXIE_OK && s != NULL);
    if (s != NULL) {
        char text[64];
        int32_t length = 0;
        sign = FindEntity(s, "sign");
        CHECK(api.field_get_text(s, sign, textType, FindField(s, textType, "text"), text, (int32_t)sizeof text, &length) ==
                  PIXIE_OK &&
              strcmp(text, "HELLO") == 0);
        api.session_destroy(s);
    }
}

/* A sprite in the project: its image is a text field naming a PNG in the project folder; it is
 * drawn in the image's color, picked by its rectangle, saved, and opened again. */
static void ExerciseSprite(const char* folder) {
    char path[1024];
    snprintf(path, sizeof path, "%s/green.png", folder);
    FILE* file = fopen(path, "wb");
    CHECK(file != NULL);
    if (file == NULL) {
        return;
    }
    fwrite(kGreenPng, 1, sizeof kGreenPng, file);
    fclose(file);

    PixieSession* s = NULL;
    CHECK(api.session_open_project(640, 360, folder, &s) == PIXIE_OK && s != NULL);
    if (s == NULL) {
        return;
    }
    int32_t transform = -1, spriteType = -1;
    CHECK(api.type_find(s, "Transform2D", &transform) == PIXIE_OK && api.type_find(s, "Sprite", &spriteType) == PIXIE_OK);
    const int32_t image = FindField(s, spriteType, "image");
    int32_t kind = -1;
    CHECK(api.field_kind(s, spriteType, image, &kind) == PIXIE_OK && kind == PIXIE_FIELD_TEXT);
    char text[256];
    CHECK(api.field_text(s, spriteType, image, PIXIE_TEXT_ASSET, text, (int32_t)sizeof text) == PIXIE_OK &&
          strcmp(text, "png") == 0);

    uint32_t sprite = 0;
    CHECK(api.entity_create(s, "sprite", &sprite) == PIXIE_OK);
    CHECK(api.component_add(s, sprite, transform) == PIXIE_OK && api.component_add(s, sprite, spriteType) == PIXIE_OK);
    /* Text goes only through the text functions, and the others refuse it. */
    PixieValue value;
    CHECK(api.field_get(s, sprite, spriteType, image, &value) == PIXIE_ERROR_ARGUMENT);
    CHECK(api.field_set_text(s, sprite, transform, FindField(s, transform, "position"), "x") == PIXIE_ERROR_ARGUMENT);
    CHECK(api.field_set_text(s, sprite, spriteType, image, "green.png") == PIXIE_OK && api.project_unsaved(s) == 1);
    int32_t length = 0;
    CHECK(api.field_get_text(s, sprite, spriteType, image, NULL, 0, &length) == PIXIE_OK && length == 9);
    CHECK(api.field_get_text(s, sprite, spriteType, image, text, 4, &length) == PIXIE_ERROR_BUFFER);
    CHECK(api.field_get_text(s, sprite, spriteType, image, text, (int32_t)sizeof text, &length) == PIXIE_OK &&
          strcmp(text, "green.png") == 0);

    /* 4 x 4 pixels scaled 25: 100 world units wide, centered at (200, 0), which the view shows at
     * (520, 180). */
    const PixieValue at = {200.0, 0.0, 0.0, 0.0};
    const PixieValue scale = {25.0, 25.0, 0.0, 0.0};
    CHECK(api.field_set(s, sprite, transform, FindField(s, transform, "position"), &at) == PIXIE_OK);
    CHECK(api.field_set(s, sprite, transform, FindField(s, transform, "scale"), &scale) == PIXIE_OK);
    uint32_t texture = 0;
    unsigned char rgba[4];
    CHECK(api.session_render(s, &texture) == PIXIE_OK);
    TexturePixel(texture, 640, 360, 520, 180, rgba);
    CHECK(rgba[0] == 40 && rgba[1] == 200 && rgba[2] == 120);
    printf("sprite pixel: %d %d %d %d\n", rgba[0], rgba[1], rgba[2], rgba[3]);

    uint32_t picked = 0;
    CHECK(api.pick(s, 560.0, 220.0, 0.0, &picked) == PIXIE_OK && picked == sprite);
    CHECK(api.pick(s, 580.0, 180.0, 0.0, &picked) == PIXIE_ERROR_NOT_FOUND);
    double corners[8];
    CHECK(api.entity_view_quad(s, sprite, corners) == PIXIE_OK);
    CHECK(fabs(corners[0] - 470.0) < 1e-6 && fabs(corners[1] - 130.0) < 1e-6);
    CHECK(fabs(corners[4] - 570.0) < 1e-6 && fabs(corners[5] - 230.0) < 1e-6);
    double x = 0.0, y = 0.0, radius = 0.0;
    CHECK(api.entity_view_bounds(s, sprite, &x, &y, &radius) == PIXIE_OK && fabs(x - 520.0) < 1e-6 &&
          fabs(radius - 50.0 * sqrt(2.0)) < 1e-6);

    /* The session's image, for a preview, and one that cannot be read. */
    int32_t width = 0, height = 0;
    CHECK(api.image(s, "green.png", &texture, &width, &height) == PIXIE_OK && texture != 0 && width == 4 && height == 4);
    CHECK(api.image(s, "missing.png", &texture, &width, &height) == PIXIE_ERROR_IO);
    CHECK(api.project_save(s) == PIXIE_OK);
    api.session_destroy(s);

    s = NULL;
    CHECK(api.session_open_project(640, 360, folder, &s) == PIXIE_OK && s != NULL);
    if (s != NULL) {
        sprite = FindEntity(s, "sprite");
        CHECK(api.field_get_text(s, sprite, spriteType, image, text, (int32_t)sizeof text, &length) == PIXIE_OK &&
              strcmp(text, "green.png") == 0);
        api.session_destroy(s);
    }
}

/* A whole project through the generic library: created, opened in edit mode, authored, saved,
 * played, stopped, and opened again. */
static void ExerciseProject(const char* folder, const char* expectedType) {
    Cleanup(folder);
    CHECK(api.project_create(folder) == PIXIE_OK);
    CHECK(api.project_create(folder) == PIXIE_ERROR_IO);
    CHECK(api.project_create(NULL) == PIXIE_ERROR_ARGUMENT);

    PixieSession* s = NULL;
    CHECK(api.session_open_project(640, 360, folder, &s) == PIXIE_OK && s != NULL);
    if (s == NULL) {
        return;
    }
    char name[128];
    CHECK(api.project_name(s, name, (int32_t)sizeof name) == PIXIE_OK && strcmp(name, BaseName(folder)) == 0);
    CHECK(api.session_mode(s) == PIXIE_MODE_EDIT && api.project_unsaved(s) == 0);
    if (expectedType != NULL) {
        int32_t expected = -1;
        CHECK(api.type_find(s, expectedType, &expected) == PIXIE_OK);
        printf("type %s: %d\n", expectedType, expected);
    }
    int32_t count = 0;
    CHECK(api.entities(s, NULL, 0, &count) == PIXIE_OK && count == 1);
    FindEntity(s, "shape");

    /* Edit mode runs no step, and the new project's shape is drawn. */
    PixieStats stats;
    CHECK(api.session_advance(s, 0.2) == PIXIE_OK && api.session_step(s) == PIXIE_OK);
    CHECK(api.session_stats(s, &stats) == PIXIE_OK && stats.steps == 0);
    uint32_t texture = 0;
    CHECK(api.session_render(s, &texture) == PIXIE_OK && TextureHasContent(texture, 640, 360));

    /* An entity that moves, made by hand. */
    int32_t transform = -1, velocity = -1, shape = -1;
    CHECK(api.type_find(s, "Transform2D", &transform) == PIXIE_OK);
    CHECK(api.type_find(s, "Velocity2D", &velocity) == PIXIE_OK);
    CHECK(api.type_find(s, "Shape", &shape) == PIXIE_OK);
    const int32_t position = FindField(s, transform, "position");
    const int32_t linear = FindField(s, velocity, "linear");
    uint32_t mover = 0;
    CHECK(api.entity_create(s, "mover", &mover) == PIXIE_OK && api.project_unsaved(s) == 1);
    CHECK(api.component_add(s, mover, transform) == PIXIE_OK && api.component_add(s, mover, velocity) == PIXIE_OK);
    const PixieValue speed = {100.0, 0.0, 0.0, 0.0};
    CHECK(api.field_set(s, mover, velocity, linear, &speed) == PIXIE_OK);
    CHECK(api.component_add(s, mover, velocity) == PIXIE_OK);
    PixieValue read;
    CHECK(api.field_get(s, mover, velocity, linear, &read) == PIXIE_OK && read.x == 100.0);
    CHECK(api.component_add(s, mover, shape) == PIXIE_OK && api.component_remove(s, mover, shape) == PIXIE_OK);

    /* A copy of a spawner and the system it feeds: the copied spawner feeds the copied system. A copy
     * of the spawner alone feeds the system it fed. The copies are removed again. */
    int32_t systemType = -1, spawnerType = -1;
    CHECK(api.type_find(s, "ParticleSystem", &systemType) == PIXIE_OK);
    CHECK(api.type_find(s, "ParticleSpawner", &spawnerType) == PIXIE_OK);
    const int32_t feeds = FindField(s, spawnerType, "system");
    uint32_t pair[2];
    CHECK(api.entity_create(s, "system", &pair[0]) == PIXIE_OK && api.component_add(s, pair[0], systemType) == PIXIE_OK);
    CHECK(api.entity_create(s, "spawner", &pair[1]) == PIXIE_OK && api.component_add(s, pair[1], spawnerType) == PIXIE_OK);
    const PixieValue target = {(double)pair[0], 0.0, 0.0, 0.0};
    CHECK(api.field_set(s, pair[1], spawnerType, feeds, &target) == PIXIE_OK);
    char* both = Snapshot(s, pair, 2);
    uint32_t copies[2];
    int32_t copied = 0;
    CHECK(api.entities_restore(s, both, PIXIE_RESTORE_NEW_IDS, copies, 2, &copied) == PIXIE_OK && copied == 2);
    CHECK(api.field_get(s, copies[1], spawnerType, feeds, &read) == PIXIE_OK && (uint32_t)read.x == copies[0]);
    uint64_t original = 0, copy = 0;
    CHECK(api.entity_scene_id(s, pair[0], &original) == PIXIE_OK && api.entity_scene_id(s, copies[0], &copy) == PIXIE_OK &&
          original != copy);
    char* alone = Snapshot(s, &pair[1], 1);
    uint32_t single = 0;
    CHECK(api.entities_restore(s, alone, PIXIE_RESTORE_NEW_IDS, &single, 1, &copied) == PIXIE_OK && copied == 1);
    CHECK(api.field_get(s, single, spawnerType, feeds, &read) == PIXIE_OK && (uint32_t)read.x == pair[0]);
    free(both);
    free(alone);
    CHECK(api.entity_destroy(s, single) == PIXIE_OK);
    for (int i = 0; i < 2; ++i) {
        CHECK(api.entity_destroy(s, copies[i]) == PIXIE_OK && api.entity_destroy(s, pair[i]) == PIXIE_OK);
    }
    CHECK(api.project_save(s) == PIXIE_OK && api.project_unsaved(s) == 0);

    /* Play: built again from the scene, and stepping. Edits in play mark nothing. */
    CHECK(api.session_play(s) == PIXIE_OK && api.session_mode(s) == PIXIE_MODE_PLAY);
    CHECK(api.session_advance(s, 0.2) == PIXIE_OK && api.session_advance(s, 0.2) == PIXIE_OK);
    CHECK(api.session_stats(s, &stats) == PIXIE_OK && stats.steps == 24);
    mover = FindEntity(s, "mover");
    CHECK(api.field_get(s, mover, transform, position, &read) == PIXIE_OK && fabs(read.x - 40.0) < 1e-6);
    const PixieValue away = {999.0, 0.0, 0.0, 0.0};
    CHECK(api.field_set(s, mover, transform, position, &away) == PIXIE_OK && api.project_unsaved(s) == 0);

    /* Stop: back to the scene as it was at play. */
    CHECK(api.session_stop(s) == PIXIE_OK && api.session_mode(s) == PIXIE_MODE_EDIT);
    CHECK(api.session_stats(s, &stats) == PIXIE_OK && stats.steps == 0 && stats.entities == 2);
    mover = FindEntity(s, "mover");
    CHECK(api.field_get(s, mover, transform, position, &read) == PIXIE_OK && read.x == 0.0);
    api.session_destroy(s);

    /* Opened again, the scene holds what was saved. */
    s = NULL;
    CHECK(api.session_open_project(640, 360, folder, &s) == PIXIE_OK && s != NULL);
    if (s != NULL) {
        CHECK(api.entities(s, NULL, 0, &count) == PIXIE_OK && count == 2);
        mover = FindEntity(s, "mover");
        CHECK(api.field_get(s, mover, velocity, linear, &read) == PIXIE_OK && read.x == 100.0);
        api.session_destroy(s);
    }

    ExerciseSprite(folder);
    ExercisePath(folder);
    ExerciseText(folder);

    /* The project file names its scene relative to the folder, and holds no absolute path. */
    char path[1024];
    snprintf(path, sizeof path, "%s/%s.pixie", folder, BaseName(folder));
    FILE* file = fopen(path, "rb");
    CHECK(file != NULL);
    if (file != NULL) {
        char text[2048];
        const size_t length = fread(text, 1, sizeof text - 1, file);
        text[length] = '\0';
        fclose(file);
        CHECK(strstr(text, "\"startScene\": \"scenes/main.scene\"") != NULL);
        CHECK(strstr(text, ":/") == NULL && strstr(text, ":\\\\") == NULL);
        printf("%s", text);
    }

    /* A folder that is not a project opens nothing. */
    snprintf(path, sizeof path, "%s/scenes", folder);
    s = NULL;
    CHECK(api.session_open_project(640, 360, path, &s) == PIXIE_ERROR_IO && s == NULL);
    Cleanup(folder);
}

/* An existing project, such as a sample's, opened with its game's library: it opens in edit mode
 * with a scenario, plays, and stops back to the scene as it was; nothing is saved. A project without
 * code (`sceneOnly`, opened with the generic library) has no scenario; instead, every image its
 * sprites name must be readable. */
static void ExerciseOpen(const char* folder, int sceneOnly) {
    PixieSession* s = NULL;
    CHECK(api.session_open_project(640, 360, folder, &s) == PIXIE_OK && s != NULL);
    if (s == NULL) {
        return;
    }
    char name[128];
    CHECK(api.project_name(s, name, (int32_t)sizeof name) == PIXIE_OK);
    CHECK(api.session_mode(s) == PIXIE_MODE_EDIT && api.project_unsaved(s) == 0);
    PixieStats before;
    CHECK(api.session_stats(s, &before) == PIXIE_OK && before.entities > 0 && before.steps == 0);
    int scenarios = 0;
    for (int32_t t = 0; t < api.type_count(s); ++t) {
        scenarios += (api.type_flags(s, t) & PIXIE_TYPE_SCENARIO) != 0;
    }
    if (!sceneOnly) {
        FindEntity(s, "scenario");
        CHECK(scenarios == 1);
    } else {
        CHECK(scenarios == 0);
        int32_t spriteType = -1;
        CHECK(api.type_find(s, "Sprite", &spriteType) == PIXIE_OK);
        const int32_t image = FindField(s, spriteType, "image");
        uint32_t* ids = (uint32_t*)malloc(sizeof(uint32_t) * (size_t)before.entities);
        int32_t filled = 0;
        int sprites = 0;
        CHECK(ids != NULL && api.entities(s, ids, before.entities, &filled) == PIXIE_OK);
        for (int32_t i = 0; ids != NULL && i < filled; ++i) {
            if (!api.entity_has(s, ids[i], spriteType)) {
                continue;
            }
            char path[512];
            int32_t length = 0;
            uint32_t texture = 0;
            int32_t width = 0, height = 0;
            CHECK(api.field_get_text(s, ids[i], spriteType, image, path, (int32_t)sizeof path, &length) == PIXIE_OK);
            CHECK(api.image(s, path, &texture, &width, &height) == PIXIE_OK && width > 0 && height > 0);
            ++sprites;
        }
        free(ids);
        CHECK(sprites > 0);
        printf("%d sprites, every image readable\n", sprites);
    }

    PixieStats stats;
    CHECK(api.session_play(s) == PIXIE_OK && api.session_mode(s) == PIXIE_MODE_PLAY);
    CHECK(api.session_advance(s, 0.2) == PIXIE_OK);
    CHECK(api.session_stats(s, &stats) == PIXIE_OK && stats.steps == 12);
    uint32_t texture = 0;
    CHECK(api.session_render(s, &texture) == PIXIE_OK && TextureHasContent(texture, 640, 360));
    CHECK(api.session_stop(s) == PIXIE_OK && api.session_mode(s) == PIXIE_MODE_EDIT);
    CHECK(api.session_stats(s, &stats) == PIXIE_OK && stats.steps == 0 && stats.entities == before.entities);
    CHECK(api.project_unsaved(s) == 0);
    printf("%s: %d entities\n", name, before.entities);
    api.session_destroy(s);
}

int main(int argc, char** argv) {
    const int project = (argc == 4 || argc == 6) && strcmp(argv[2], "--project") == 0;
    const char* expectedType = argc == 6 && strcmp(argv[4], "--expect-type") == 0 ? argv[5] : NULL;
    const int open = (argc == 4 || argc == 5) && strcmp(argv[2], "--open") == 0;
    const int sceneOnly = open && argc == 5 && strcmp(argv[4], "--scene-only") == 0;
    if (argc != 2 && !(open && (argc == 4 || sceneOnly)) && !(project && (argc == 4 || expectedType != NULL))) {
        fprintf(stderr, "usage: capi-test <library> [--project <folder> [--expect-type <name>] | --open <folder> [--scene-only]]\n");
        return 2;
    }
    Library library = OpenLibrary(argv[1]);
    if (library == NULL) {
        fprintf(stderr, "cannot load %s\n", argv[1]);
        return 1;
    }
    if (!Load(library)) {
        return 1;
    }
    CHECK(api.api_version() == PIXIE_CAPI_VERSION);

    if (!glfwInit()) {
        fprintf(stderr, "GLFW initialization failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(640, 360, "capi-test", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "no OpenGL 4.5 core context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    if (project) {
        ExerciseProject(argv[3], expectedType);
    } else if (open) {
        ExerciseOpen(argv[3], sceneOnly);
    } else {
        PixieSession* session = NULL;
        CHECK(api.session_create(0, 360, 1, &session) == PIXIE_ERROR_ARGUMENT && session == NULL);
        CHECK(api.session_create(640, 360, 7, &session) == PIXIE_OK && session != NULL);
        if (session != NULL) {
            Exercise(session);
            api.session_destroy(session);
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    CloseLibrary(library);
    printf("%s: %d failed checks\n", argv[1], failures);
    return failures == 0 ? 0 : 1;
}
