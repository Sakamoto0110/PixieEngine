/*
 * The C API of PixieEngine: a shared library, one per game, whose interface is C only, so any
 * language that calls C (the C# editor through P/Invoke, tools, tests) can run the game, draw it
 * into a texture, feed it input, and read and write its components.
 *
 * Rules for every function:
 * - The OpenGL 4.5 core context the session was created with must be current on the calling
 *   thread. A session is used from one thread.
 * - Nothing throws across this boundary: functions return a PixieResult, and on failure
 *   pixie_last_error describes it.
 * - Strings are UTF-8, copied into the caller's buffer with a terminating NUL. A buffer too small
 *   gets the truncated string and PIXIE_ERROR_BUFFER.
 * - Entities are 32-bit handles (index and version); a handle to a destroyed entity is reported as
 *   not found, never confused with a newer one.
 */
#ifndef PIXIE_CAPI_H
#define PIXIE_CAPI_H

#include <stdint.h>

#if defined(_WIN32)
#if defined(PIXIE_CAPI_BUILD)
#define PIXIE_API __declspec(dllexport)
#else
#define PIXIE_API __declspec(dllimport)
#endif
#else
#define PIXIE_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Raised when a function's meaning or a struct's layout changes. */
#define PIXIE_CAPI_VERSION 2

typedef struct PixieSession PixieSession;

typedef enum PixieResult {
    PIXIE_OK = 0,
    PIXIE_ERROR_ARGUMENT = 1,  /* a null pointer, an index out of range, a bad size */
    PIXIE_ERROR_NOT_FOUND = 2, /* the entity is gone, or lacks the component */
    PIXIE_ERROR_GL = 3,        /* OpenGL could not be loaded, or a GL object could not be made */
    PIXIE_ERROR_BUFFER = 4,    /* the string did not fit; it was truncated */
    PIXIE_ERROR_INTERNAL = 5   /* an unexpected failure inside the engine */
} PixieResult;

/* What a field holds, and which members of PixieValue carry it. */
typedef enum PixieFieldKind {
    PIXIE_FIELD_BOOL = 0,   /* x: 0 or 1 */
    PIXIE_FIELD_INT = 1,    /* x */
    PIXIE_FIELD_FLOAT = 2,  /* x */
    PIXIE_FIELD_DOUBLE = 3, /* x */
    PIXIE_FIELD_ENUM = 4,   /* x: the value; pixie_field_enum_name names 0, 1, ... */
    PIXIE_FIELD_POINT = 5,  /* x, y */
    PIXIE_FIELD_VECTOR = 6, /* x, y */
    PIXIE_FIELD_RECT = 7,   /* x, y, z: width, w: height */
    PIXIE_FIELD_COLOR = 8,  /* x, y, z, w: red, green, blue, alpha, 0 to 255 */
    PIXIE_FIELD_ENTITY = 9  /* x: an entity handle; 4294967295 is none */
} PixieFieldKind;

/* The editor a tool should use for a field; AUTO lets it choose from the kind. */
typedef enum PixieEditorKind {
    PIXIE_EDITOR_AUTO = 0,
    PIXIE_EDITOR_NUMBER = 1,
    PIXIE_EDITOR_SLIDER = 2,
    PIXIE_EDITOR_TOGGLE = 3,
    PIXIE_EDITOR_CHOICE = 4,
    PIXIE_EDITOR_COLOR = 5,
    PIXIE_EDITOR_ARC = 6 /* an angle and the width around it; PIXIE_TEXT_ARC_SPREAD names the width */
} PixieEditorKind;

/* PixieFieldInfo.flags */
#define PIXIE_FIELD_READ_ONLY 1u
#define PIXIE_FIELD_HAS_RANGE 2u
#define PIXIE_FIELD_HAS_UNIT 4u

/* How a tool shows and edits a field. Range, step, and scrub are in the displayed unit: shown =
 * held * unitFactor + unitOffset. */
typedef struct PixieFieldInfo {
    int32_t kind;      /* PixieFieldKind */
    int32_t editor;    /* PixieEditorKind */
    uint32_t flags;    /* PIXIE_FIELD_* */
    int32_t order;     /* fields are shown by order */
    int32_t scrubAxis; /* 0: horizontal, 1: vertical */
    int32_t enumCount; /* names of an enum's values; pixie_field_enum_name */
    double rangeMin;
    double rangeMax;
    double rangeStep;  /* 0: no step */
    double scrub;      /* change per pixel dragged; 0: no scrubbing */
    double unitFactor;
    double unitOffset;
} PixieFieldInfo;

/* The texts of a field, for pixie_field_text. */
typedef enum PixieFieldText {
    PIXIE_TEXT_LABEL = 0,       /* the name shown; the field's name when none was set */
    PIXIE_TEXT_TOOLTIP = 1,
    PIXIE_TEXT_HELP = 2,
    PIXIE_TEXT_UNIT_SUFFIX = 3,
    PIXIE_TEXT_ARC_SPREAD = 4,  /* for an ARC field, the name of the field with the arc's width */
    PIXIE_TEXT_REFERENCES = 5   /* for an ENTITY field, the component type a valid entity has */
} PixieFieldText;

typedef struct PixieValue {
    double x;
    double y;
    double z;
    double w;
} PixieValue;

typedef struct PixieStats {
    uint64_t steps;              /* steps the World has run */
    double elapsed;              /* simulated seconds */
    double lastStepMilliseconds; /* wall time of the last step */
    int32_t entities;
    int32_t paused;
    double timeScale;            /* simulated seconds per wall second */
} PixieStats;

/* pixie_type_flags */
#define PIXIE_TYPE_SCENARIO 1u /* values the scene is built from; offer to rebuild after editing */

/* PIXIE_CAPI_VERSION of the library. */
PIXIE_API int32_t pixie_api_version(void);
/* The last failure on this thread, as text. Returns the full length, which may exceed capacity. */
PIXIE_API int32_t pixie_last_error(char* buffer, int32_t capacity);

/* Session: the game, its World, and its time. Create it with the host's GL context current; the
 * view is width x height pixels. Loads the GL functions for that context. */
PIXIE_API PixieResult pixie_session_create(int32_t width, int32_t height, uint64_t seed,
                                           PixieSession** session);
/* Releases the session and its GL objects; the same context must be current. Null is ignored. */
PIXIE_API void pixie_session_destroy(PixieSession* session);
PIXIE_API PixieResult pixie_project_name(PixieSession* session, char* buffer, int32_t capacity);
PIXIE_API PixieResult pixie_session_resize(PixieSession* session, int32_t width, int32_t height);
/* Adds `seconds` of wall time and runs the fixed steps it makes due, unless paused. */
PIXIE_API PixieResult pixie_session_advance(PixieSession* session, double seconds);
/* Runs one fixed step now, paused or not. */
PIXIE_API PixieResult pixie_session_step(PixieSession* session);
PIXIE_API PixieResult pixie_session_set_paused(PixieSession* session, int32_t paused);
/* How fast simulated time runs against the wall time given to advance, from 0.01 to 8 (clamped). */
PIXIE_API PixieResult pixie_session_set_time_scale(PixieSession* session, double scale);
/* Rebuilds the World from the game, with the same seed; the game first reads back its scenario. */
PIXIE_API PixieResult pixie_session_reset(PixieSession* session);
/* Draws the World into the session's own texture (RGBA8, view-sized, first row at the bottom) and
 * returns the GL texture name. The texture stays valid until the next render, resize, or destroy. */
PIXIE_API PixieResult pixie_session_render(PixieSession* session, uint32_t* texture);
PIXIE_API PixieResult pixie_session_stats(PixieSession* session, PixieStats* stats);

/* Input, in view pixels with the origin at the top-left; key and button codes are GLFW's. */
PIXIE_API PixieResult pixie_input_cursor(PixieSession* session, double x, double y);
PIXIE_API PixieResult pixie_input_mouse_button(PixieSession* session, int32_t button, int32_t down);
PIXIE_API PixieResult pixie_input_key(PixieSession* session, int32_t key, int32_t down);
PIXIE_API PixieResult pixie_input_scroll(PixieSession* session, double dx, double dy);

/* Entities, oldest first: fills up to `capacity` handles and sets `count` to how many there are. */
PIXIE_API PixieResult pixie_entities(PixieSession* session, uint32_t* entities, int32_t capacity,
                                     int32_t* count);
/* The entity's Name, or an empty string when it has none. */
PIXIE_API PixieResult pixie_entity_name(PixieSession* session, uint32_t entity, char* buffer,
                                        int32_t capacity);
/* 1 when the entity exists, else 0. */
PIXIE_API int32_t pixie_entity_alive(PixieSession* session, uint32_t entity);

/* Component types that can be inspected, by index from 0 to pixie_type_count - 1. */
PIXIE_API int32_t pixie_type_count(PixieSession* session);
PIXIE_API PixieResult pixie_type_name(PixieSession* session, int32_t type, char* buffer,
                                      int32_t capacity);
/* PIXIE_TYPE_* flags of a component type. */
PIXIE_API uint32_t pixie_type_flags(PixieSession* session, int32_t type);
/* 1 when the entity has a component of the type, else 0. */
PIXIE_API int32_t pixie_entity_has(PixieSession* session, uint32_t entity, int32_t type);
PIXIE_API int32_t pixie_field_count(PixieSession* session, int32_t type);
PIXIE_API PixieResult pixie_field_name(PixieSession* session, int32_t type, int32_t field,
                                       char* buffer, int32_t capacity);
PIXIE_API PixieResult pixie_field_kind(PixieSession* session, int32_t type, int32_t field,
                                       int32_t* kind);
/* The field's metadata (sizeof(PixieFieldInfo) is 72), and one of its texts (empty when unset). */
PIXIE_API PixieResult pixie_field_info(PixieSession* session, int32_t type, int32_t field,
                                       PixieFieldInfo* info);
PIXIE_API PixieResult pixie_field_text(PixieSession* session, int32_t type, int32_t field,
                                       int32_t which, char* buffer, int32_t capacity);
PIXIE_API int32_t pixie_field_enum_count(PixieSession* session, int32_t type, int32_t field);
PIXIE_API PixieResult pixie_field_enum_name(PixieSession* session, int32_t type, int32_t field,
                                            int32_t value, char* buffer, int32_t capacity);
PIXIE_API PixieResult pixie_field_get(PixieSession* session, uint32_t entity, int32_t type,
                                      int32_t field, PixieValue* value);
PIXIE_API PixieResult pixie_field_set(PixieSession* session, uint32_t entity, int32_t type,
                                      int32_t field, const PixieValue* value);

#ifdef __cplusplus
}
#endif

#endif /* PIXIE_CAPI_H */
