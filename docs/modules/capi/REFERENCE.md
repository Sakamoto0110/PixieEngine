# C API Technical Reference

**Document ID:** CAPI-REFERENCE

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** the contract of the capi module: building a game's library, the session, rendering, input, reflection, errors, and threading

**Surface:** technical-reference

**Boundary:** capi

**Authority role:** normative

**Mutation:** authored

**Indexing:** include

The header `capi/include/pixie_capi.h` owns the exact declarations; this reference owns their
meaning.

## Building a game's library

`pixie_add_capi_library(<name> <game>)` (in `capi/CMakeLists.txt`) makes the shared library
`<name>` in `bin/<config>/` from the API's object files and the static library `<game>`, which must
define `pixie::CreateProject()` (see the runtime's `Project.hpp`). The API's code is an object
library rather than a static one, so the exported functions, which nothing in the library calls,
are not dropped by the linker. On Windows the functions are exported with `__declspec(dllexport)`
(`PIXIE_CAPI_BUILD`); elsewhere with default visibility. The library links the C++ runtime
statically, like every target, so a host needs no Visual C++ Redistributable.

## Rules for every function

- The OpenGL 4.5 core context the session was created with must be current on the calling thread.
  A session is used from one thread at a time.
- No exception crosses the boundary. Functions return a `PixieResult`; on failure,
  `pixie_last_error` gives the reason as text, per thread. It returns the full length, which may
  exceed the buffer.
- Strings are UTF-8, copied with a terminating NUL. A buffer too small receives the truncated
  string, and the call returns `PIXIE_ERROR_BUFFER`.
- Null sessions, null outputs, and indices out of range return `PIXIE_ERROR_ARGUMENT`.
- `pixie_api_version` returns `PIXIE_CAPI_VERSION` (2), raised when a function's meaning or a
  struct's layout changes. Version 2 added the field metadata; a host built for version 1 does not
  load a version 2 library.

## Session

`pixie_session_create(width, height, seed, &session)` loads the GL functions for the current
context (`pixie::LoadGl`), creates the game's `Project` through `pixie::CreateProject`, and starts a
`ProjectSession` with a view of `width` x `height` pixels and the World seeded with `seed`. It
returns `PIXIE_ERROR_GL` if GL cannot be loaded or the renderers cannot be made.
`pixie_session_destroy` releases the session and its GL objects; the same context must be current.

| Function | Effect |
|---|---|
| `pixie_session_resize` | Sets the view size; the next render matches it |
| `pixie_session_advance(seconds)` | Adds wall time and runs the fixed steps it makes due (60 per second), unless paused; one call counts for 0.25 s at most |
| `pixie_session_step` | Runs one fixed step now, paused or not |
| `pixie_session_set_paused` | Pauses or resumes; resuming drops the leftover fraction of a step |
| `pixie_session_reset` | Rebuilds the World from the game with the same seed; steps start over |
| `pixie_session_stats` | Steps, simulated seconds, last step's wall time, entity count, paused |
| `pixie_project_name` | The game's name |

Before each step the session calls the project's `BeforeStep` with the session's input, then
steps the World; after each step it forgets the step's presses, releases, scroll, and movement.

## Rendering

`pixie_session_render(&texture)` draws the World (particles, shapes, the project's overlay) into
the session's own RGBA8 texture, sized to the view, interpolated between steps, and returns the GL
texture name. The texture's first row is the bottom of the image, as OpenGL has it; a host that
draws with Y down flips it with its texture coordinates (in Dear ImGui, `uv0 = (0, 1)` and
`uv1 = (1, 0)`). The name stays valid until the next render, resize, or destroy. The function
leaves the default framebuffer bound.

## Input

Input arrives as events, in view pixels with the origin at the top-left: `pixie_input_cursor`,
`pixie_input_mouse_button`, `pixie_input_key`, and `pixie_input_scroll`. Key and button codes are
GLFW's (`pixie::Key`, `pixie::MouseButton`), so a host built on GLFW passes its codes through.
Events are kept until the next step reads them.

## Entities and fields

- `pixie_entities` lists live entities oldest first: it fills up to `capacity` handles and sets
  `count` to the total, so a call with capacity 0 asks for the count. A handle is the entity's 32
  bits (index and version); a destroyed entity's handle is reported as not found, never confused
  with a newer entity in the same slot.
- `pixie_entity_name` gives the entity's `Name`, or an empty string.
- Component types are indexed from 0 to `pixie_type_count` - 1: the runtime's own
  (`DescribeRuntimeComponents`), then the game's (`Project::Describe`). `pixie_type_name`,
  `pixie_field_count`, `pixie_field_name`, `pixie_field_kind`, `pixie_field_enum_count`, and
  `pixie_field_enum_name` describe them; `pixie_entity_has` tells whether an entity has one.
- `pixie_field_info` fills a `PixieFieldInfo` (72 bytes): the kind, the editor
  (`PixieEditorKind`), flags (`PIXIE_FIELD_READ_ONLY`, `PIXIE_FIELD_HAS_RANGE`,
  `PIXIE_FIELD_HAS_UNIT`), the order, the scrub per pixel and its axis, the number of enum names,
  the range and step, and the unit's factor and offset (shown = held * factor + offset). Range, step,
  and scrub are in the displayed unit. `pixie_field_text` gives the label (the field's name when none
  was set), tooltip, help, unit suffix, or, for an arc, the name of the field with its width
  (`PixieFieldText`). They export the runtime's `FieldMetadata`; a host builds its editors from
  them.
- `pixie_field_get` and `pixie_field_set` read and write one field through its descriptor, as a
  `PixieValue`; `PixieFieldKind` says which members are used. Writing outside a step is safe;
  the value takes effect at the next step. A missing entity or component returns
  `PIXIE_ERROR_NOT_FOUND`.

## Non-goals

No saving or loading, no entity creation or destruction through the API, no Name editing, and no
fields of nested structs (such as a particle pool's style) yet.

## Validation

`tests/capi/CapiTest.c` is a C11 program that includes only `pixie_capi.h` from the engine, loads a
game's library at run time (`LoadLibrary` or `dlopen`, as P/Invoke does), creates a hidden GLFW
window with an OpenGL 4.5 core context, and checks: the version; type and field lookup; the
metadata of a field shown in degrees, and a tooltip on every field; the entity
list and the oldest entity's name; writing a field and reading it back; time, the 0.25 s clamp,
pause, and single steps; a key press; rendering into a texture at a new size, with content; errors
for a bad type and a short buffer; and reset. CTest runs it once per game library.
