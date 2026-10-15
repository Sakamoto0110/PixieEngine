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
    RemoveFolder(folder);
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
 * with a scenario, plays, and stops back to the scene as it was; nothing is saved. */
static void ExerciseOpen(const char* folder) {
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
    FindEntity(s, "scenario");
    int scenarios = 0;
    for (int32_t t = 0; t < api.type_count(s); ++t) {
        scenarios += (api.type_flags(s, t) & PIXIE_TYPE_SCENARIO) != 0;
    }
    CHECK(scenarios == 1);

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
    const int open = argc == 4 && strcmp(argv[2], "--open") == 0;
    if (argc != 2 && !open && !(project && (argc == 4 || expectedType != NULL))) {
        fprintf(stderr, "usage: capi-test <library> [--project <folder> [--expect-type <name>] | --open <folder>]\n");
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
        ExerciseOpen(argv[3]);
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
