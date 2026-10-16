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
statically, like every target, so a host needs no Visual C++ Redistributable. A game's project calls
it through `pixie_game` ([runtime reference](../runtime/REFERENCE.md#projects-with-code)); it works
from outside the engine's tree, and the library then lands in that project's `bin/<config>/`.

When it is the top-level project, the engine builds one library of its own, `pixie-scene-capi`, from
the game `pixie-scene-game`,
whose project is the runtime's `SceneProject`: a World that is only its scene. It runs a project
folder that has no code. Beside it, `pixie-player` links the same game with the engine's player
(`runtime/player/PlayerMain.cpp`), so such a project also plays on its own:
`pixie-player --project <folder>`.

## Rules for every function

- The OpenGL 4.5 core context the session was created with must be current on the calling thread.
  A session is used from one thread at a time.
- No exception crosses the boundary. Functions return a `PixieResult`; on failure,
  `pixie_last_error` gives the reason as text, per thread. It returns the full length, which may
  exceed the buffer.
- Strings are UTF-8, copied with a terminating NUL. A buffer too small receives the truncated
  string, and the call returns `PIXIE_ERROR_BUFFER`.
- Null sessions, null outputs, and indices out of range return `PIXIE_ERROR_ARGUMENT`.
- `pixie_api_version` returns `PIXIE_CAPI_VERSION` (8), raised when a function's meaning or a
  struct's layout changes, or functions are added. Version 2 added the field metadata, version 3
  picking, destruction, and spawning, version 4 projects, modes, and authoring, version 5 the
  conversion between view pixels and world units, version 6 scene IDs, snapshots, and names,
  version 7 text fields, sprites' rectangles, and images, version 8 point lists and paths' lines; a
  host checks the version and does not load a library of another one.
- A project or scene that cannot be read or written returns `PIXIE_ERROR_IO`. Folder paths are
  UTF-8.

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
| `pixie_session_set_time_scale` | Simulated seconds per wall second given to advance, from 0.01 to 8 (clamped); not positive is an argument error |
| `pixie_session_reset` | Rebuilds the World from the game with the same seed, after the game reads back its scenario; with a project open, from the scene's entities as they are now; steps start over |
| `pixie_session_stats` | Steps, simulated seconds, last step's wall time, entity count, paused, time scale |
| `pixie_project_name` | The project file's name when a project is open, else the game's |

Before each step the session calls the project's `BeforeStep` with the session's input, then
steps the World; after each step it forgets the step's presses, releases, scroll, and movement.

## Projects and modes

A project is a folder with one `<Name>.pixie` file and its scenes
([runtime projects](../runtime/REFERENCE.md#projects-and-sessions)).

- `pixie_project_create(folder)` makes a new project in a folder that is empty or does not exist
  yet: the project takes the folder's name, and its start scene holds one shape at the origin. It
  needs no session and no GL context. A folder that is not empty returns `PIXIE_ERROR_IO`.
- `pixie_session_open_project(width, height, folder, &session)` is `pixie_session_create` for the
  project in `folder`, with the game's library: the World is built from the start scene (its
  entities, then the game's `Build`), seeded with the scene's seed, and the session starts in edit
  mode. A folder that is not a project, or whose scene cannot be read, returns `PIXIE_ERROR_IO` and
  no session.
- `pixie_session_mode` returns `PIXIE_MODE_PLAY` or `PIXIE_MODE_EDIT`. A session without a project
  always plays. In edit mode, advance and step run no step, the game sees no input, and the middle
  button and the wheel pan and zoom the camera.
- `pixie_session_play` keeps the scene's entities and builds the World again from them, playing
  and unpaused; `pixie_session_stop` builds it again from what play kept, in edit mode, so nothing
  done in play stays. The camera is kept. Without a project both return `PIXIE_ERROR_ARGUMENT`.
- `pixie_project_save` writes the scene's entities to the start scene's file, through a temporary
  file; while playing, it writes the ones play kept. `pixie_project_unsaved` returns 1 when the
  scene was edited since it was opened or saved: in edit mode, creating an entity, adding or
  removing a component of a scene entity, writing one of its fields, and destroying it are edits;
  changes to entities the game generates, and anything done in play, are not.

## Rendering

`pixie_session_render(&texture)` draws the World (sprites, paths, particles, shapes, the project's overlay) into
the session's own RGBA8 texture, sized to the view, interpolated between steps, and returns the GL
texture name. The texture's first row is the bottom of the image, as OpenGL has it; a host that
draws with Y down flips it with its texture coordinates (in Dear ImGui, `uv0 = (0, 1)` and
`uv1 = (1, 0)`). The name stays valid until the next render, resize, or destroy. The function
leaves the default framebuffer bound.

`pixie_image(path, &texture, &width, &height)` gives an image of the session as a `Sprite` names
it, relative to the project folder: the session's own GL texture of it (RGBA8, premultiplied alpha,
first row at the top, so a host draws it with `uv0 = (0, 0)`), valid until the image changes on
disk or the session stops, and its size in pixels, for a preview. An image that cannot be read
returns `PIXIE_ERROR_IO`, with the reason (a missing file, not a PNG) in `pixie_last_error`. It
needs the session's GL context, as rendering does.

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
- `pixie_entity_create(name, &entity)` creates an entity that belongs to the scene (it gets a
  `SceneId`), named `name`, or unnamed for null or an empty string. `pixie_entity_set_name` writes
  an entity's `Name`; null or an empty string removes it.
- An entity of the scene keeps its scene ID through play, stop, reset, and restoring; its handle
  does not. `pixie_entity_scene_id` gives the ID, or `PIXIE_ERROR_NOT_FOUND` for an entity that is
  gone or has none; `pixie_entity_find` gives the live entity with an ID, or `PIXIE_ERROR_NOT_FOUND`.
  A host that remembers entities across those changes, such as an editor's undo, remembers IDs.
- `pixie_entities_snapshot(entities, count, buffer, capacity, &length)` writes scene entities as
  text (the runtime's `WriteEntities`): `length` is the full length, a null buffer with capacity 0
  asks for it, and a buffer too small returns `PIXIE_ERROR_BUFFER`. Entities that are gone or not in
  the scene are left out. `pixie_entities_restore(snapshot, ids, entities, capacity, &count)` adds a
  snapshot's entities (`ReadEntities`): `PIXIE_RESTORE_SAME_IDS` with their IDs, which must not be
  in the World, or `PIXIE_RESTORE_NEW_IDS` with new ones, references following copies inside the
  snapshot. It fills and counts the entities added, as `pixie_entities` does; a snapshot it cannot
  restore returns `PIXIE_ERROR_ARGUMENT` and adds nothing. In edit mode, restoring and renaming a
  scene entity mark the scene unsaved.
- Component types are indexed from 0 to `pixie_type_count` - 1: the runtime's own
  (`DescribeRuntimeComponents`), then the game's (`Project::Describe`). `pixie_type_name`,
  `pixie_field_count`, `pixie_field_name`, `pixie_field_kind`, `pixie_field_enum_count`, and
  `pixie_field_enum_name` describe them; `pixie_type_find` gives the index of a type by name, or
  `PIXIE_ERROR_NOT_FOUND`; `pixie_entity_has` tells whether an entity has one.
  `pixie_component_add` adds a component of a type with its default values, or keeps the one the
  entity has (a type without a default value is an argument error), and `pixie_component_remove`
  removes it; both return `PIXIE_ERROR_NOT_FOUND` for a destroyed entity, and remove does for an
  entity without the component.
  `pixie_type_flags` returns `PIXIE_TYPE_SCENARIO` for a type that holds a scenario: values the
  scene is built from, which a host offers to rebuild with after an edit.
- `pixie_field_info` fills a `PixieFieldInfo` (72 bytes): the kind, the editor
  (`PixieEditorKind`), flags (`PIXIE_FIELD_READ_ONLY`, `PIXIE_FIELD_HAS_RANGE`,
  `PIXIE_FIELD_HAS_UNIT`), the order, the scrub per pixel and its axis, the number of enum names,
  the range and step, and the unit's factor and offset (shown = held * factor + offset). Range, step,
  and scrub are in the displayed unit. `pixie_field_text` gives the label (the field's name when none
  was set), tooltip, help, unit suffix, for an arc the name of the field with its width, or, for an
  entity field, the component type a valid choice has, or, for a text field that names a file, its
  kind as an extension, such as `png` (`PIXIE_TEXT_ASSET`; `PixieFieldText`). They export the runtime's `FieldMetadata`; a host builds its editors from
  them.
- `pixie_field_get` and `pixie_field_set` read and write one field through its descriptor, as a
  `PixieValue`; `PixieFieldKind` says which members are used. A `PIXIE_FIELD_ENTITY` field holds an
  entity handle in `x`, with 4294967295 for none. Writing outside a step is safe;
  the value takes effect at the next step. A missing entity or component returns
  `PIXIE_ERROR_NOT_FOUND`.
- A `PIXIE_FIELD_TEXT` field holds UTF-8 text, such as a sprite's image path, and goes only through
  `pixie_field_get_text(entity, type, field, buffer, capacity, &length)`, which fills and measures
  as `pixie_entities_snapshot` does, and `pixie_field_set_text(entity, type, field, text)`. Each
  pair refuses the other's kinds with `PIXIE_ERROR_ARGUMENT`. In edit mode, writing a scene
  entity's field, of any kind, marks the scene unsaved.
- A `PIXIE_FIELD_POINT_LIST` field holds points, such as a path's, and goes only through
  `pixie_field_get_points(entity, type, field, xy, capacity, &count)`, which fills up to `capacity`
  points as `x, y` pairs (`2 * capacity` doubles) and counts them as `pixie_entities` does, and
  `pixie_field_set_points(entity, type, field, xy, count)`, which replaces them. They refuse other
  kinds, and `pixie_field_get` and `pixie_field_set` refuse point lists, with `PIXIE_ERROR_ARGUMENT`.

## Picking, destruction, and spawning

`pixie_view_to_world(x, y, &worldX, &worldY)` turns a point in view pixels (origin at the
top-left) into world units through the session's camera as it is now, and `pixie_world_to_view`
turns it back, so a host can place things where the user points.

For a host's selection, in view pixels like the input. Only entities with a `Transform2D` and a
`Shape`, a `Sprite`, a `Text`, or a `Path` can be picked ([runtime picking](../runtime/REFERENCE.md#picking));
particles are not entities.

- `pixie_pick` returns the entity drawn on top under a point, widened by `tolerance` pixels: a
  shape whose radius covers it, nearest center first; else a sprite or a text (by its block of
  lines) of a layer of 0 or more; else a path within half its width; else a sprite or a text of a
  negative layer; or `PIXIE_ERROR_NOT_FOUND`.
  `pixie_pick_box` lists the entities whose position lies in a box given by two corners in any
  order, and the paths whose whole line does, oldest first, filling and counting as
  `pixie_entities` does.
- `pixie_entity_view_bounds` gives where an entity was drawn by the last render (between the
  previous step and this one, as drawn) and a radius, in pixels, for outlining it: for a shape, the
  radius it can be picked within, from its position; for a sprite or a text, half its rectangle's or
  block's diagonal, from its center; for a path, half the diagonal of the box around its line, from the
  box's center.
- `pixie_entity_view_quad(entity, corners)` gives the four corners of a sprite's rectangle, or,
  without a sprite, of a text's block of lines, as the last render drew it, in view pixels
  (`corners[0..7]`: the top-left, top-right, bottom-right, bottom-left), or `PIXIE_ERROR_NOT_FOUND`
  for an entity with neither. The text's outline needed no new function, so the version stays 8.
- `pixie_entity_view_path(entity, xy, capacity, &count)` gives the line a path was last drawn along,
  corners rounded, in view pixels, filling and counting as `pixie_entities` does, or
  `PIXIE_ERROR_NOT_FOUND` for an entity without a path.
- `pixie_entity_destroy` destroys an entity at once. It is meant for what a host can pick:
  destroying an entity the game holds on to, such as its controls, is the host's risk.
- `pixie_particle_remove_at` removes the particle nearest a point within `radius` pixels, or
  returns `PIXIE_ERROR_NOT_FOUND`.
- `pixie_spawn_at` asks the game to create something at a point (`Project::SpawnAt`) and reports
  through `created` whether it did.

## Non-goals

No Name editing, no choice of scene other than the start scene, and no fields of nested structs
(such as a particle pool's style) yet.

## Validation

`tests/capi/CapiTest.c` is a C11 program that includes only `pixie_capi.h` from the engine, loads a
game's library at run time (`LoadLibrary` or `dlopen`, as P/Invoke does), creates a hidden GLFW
window with an OpenGL 4.5 core context, and checks: the version; type and field lookup; the
metadata of a field shown in degrees, and a tooltip on every field; the entity
list and the oldest entity's name; writing a field and reading it back; time, the 0.25 s clamp,
pause, and single steps; a key press; rendering into a texture at a new size, with content; errors
for a bad type and a short buffer; finding a type by name; the view point of an entity turned
into its transform's position and back; an entity's scene ID found again, a rename, and a snapshot
that brings the entity back under the same ID after it is destroyed (and is refused while it
exists); that a session without a project
plays and refuses play, stop, and save; creating an entity, adding and removing a component, and
destroying it; and reset. CTest runs it once per game library.

With `--project <folder>`, on `pixie-scene-capi`, it drives a whole project instead: it creates one
(and fails to create it again in the same folder), opens it in edit mode with its one shape drawn
and no step run, creates an entity with a transform and a velocity, saves, plays 24 steps and finds
the entity moved 40 units without marking the scene unsaved, stops and finds it back at the origin,
opens the project again and finds the saved velocity, checks that the project file names its scene
relatively and holds no absolute path, and fails to open a folder that is not a project. CTest runs
it as the test `pixie-scene-capi`, in a folder of the build tree that it removes before and after.
`--expect-type <name>` adds a check that the library describes that type.

In the `--project` run it also copies a spawner together with the system it feeds, and finds the
copied spawner feeding the copied system, then copies the spawner alone and finds the copy feeding
the original system. Then it writes a 4 x 4 PNG into the project, adds an entity with a sprite
whose image names it, checks that the image field is text naming a `png` and that the text and
value functions refuse each other's fields, renders and finds the image's exact color where the
sprite is drawn, picks the sprite inside its rectangle and nothing outside it, checks its corners
and bounds, reads the image's texture and size and the error of a missing one, saves, and finds
the image's path again after opening the project anew. Last, it adds a path, checks that its points
are a point list that the value functions refuse, writes and reads the points (also into a buffer
too small), renders it and finds its fill color before the fill and its color after it, picks it
within its width and not beside it, reads its line and bounds in the view, finds it drawn over a
sprite of layer -1 and under the same sprite on layer 0, saves, and reads the points again after
opening the project anew. Then it copies a font into the project and adds a text: its font field is
of asset kind `ttf`; drawn, it lights pixels of its color inside its outline (the view quad) and none
outside it, centered where its transform puts it; it is picked at one of its lit pixels; a sprite at
the same place lies under it on layer -1 and over it on layer 1, where the sprite is picked instead;
and its text is read again after saving and opening anew.

With `--open <folder>`, on a game's library, it opens that existing project and changes nothing in
it: edit mode, a `scenario` entity and one scenario type, then play for 12 steps with something
drawn, and stop back to the same number of entities, with no step and nothing unsaved. CTest runs
it for each sample as `<sample>-project`, on the sample's folder.

The test `project-code` (`tests/project-code/ProjectCodeTest.cmake`) makes a project named "Code
Test" with code from the templates, refuses to add code twice, configures and builds it as a
project of its own against this engine (with the same generator and configuration), checks that its
`bin/<config>/` holds the player, one C API library, and not the generic one, and runs `capi-test`
on that library with `--project` and `--expect-type Spinner`. It builds the engine again, so it
takes a minute or two.
