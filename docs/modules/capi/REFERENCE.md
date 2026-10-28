# C API Technical Reference

**Document ID:** CAPI-REFERENCE

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** the C interface in the shared engine, development game modules, sessions, rendering, reflection, errors and threading

**Surface:** technical-reference

**Boundary:** capi

**Authority role:** normative

**Mutation:** authored

**Indexing:** include

The header `capi/include/pixie_capi.h` owns the exact declarations; this reference owns their
meaning.

## Shared engine and development game modules

The root `pixie_shared` target produces `pixie.dll` on Windows (`libpixie.so`
on the other native path). It contains the Core/Platform/Render/App, Runtime
and C API objects. `pixie_capi_objects` keeps every exported C function in
that shared artifact. Windows uses `PIXIE_CAPI_BUILD`/`__declspec(dllexport)`;
other platforms use default symbol visibility. Native targets share /MDd in
Debug and /MD in Release. C++ game extensions require matching toolchain,
headers and runtime configuration; the C API remains the editor boundary.

`pixie_game` builds directly linked game players and, only with
`PIXIE_DEVELOPMENT`, `<game>-module` libraries containing the game's code and
a versioned factory entry. They replace per-game copies of the C API/engine.
The old `pixie_add_capi_library` path is no longer the build contract. The
generic `pixie-player --project <folder>` still runs a scene-only project.

In a development-enabled engine, `pixie_module_load(path)` selects a game
factory for subsequent sessions. A null path resets that selection to the
scene-only project. A missing/unloadable image returns an I/O error; a missing
factory/version entry or incompatible API version returns an argument error.
A failed load does not replace the current selection. Each created session
retains its module until its ProjectSession, World, descriptors, callbacks and
game objects are destroyed. Changing or clearing the default module does not
invalidate an existing session. Hosts must destroy sessions before releasing
their remaining library/context ownership.

Module loading is development-only, not a player export mechanism. Ordinary
game players link their game directly. App-local runtime packaging and
development-free export/MCP barriers remain separately qualified under
[PIXIE-TODO-051](../../../TODO.md#pixie-todo-051--shared-engine-game-export-and-local-mcp-development-channel).
See the [qualification matrix](../../shared-engine-export-validation.md) for
configuration-specific evidence and pending delivery gates.

## Rules for every function

- The OpenGL 4.5 core context the session was created with must be current on the calling thread.
  A session is used from one thread at a time.
- No exception crosses the boundary. Functions return a `PixieResult`; on failure,
  `pixie_last_error` gives the reason as text, per thread. It returns the full length, which may
  exceed the buffer.
- Strings are UTF-8, copied with a terminating NUL. A buffer too small receives the truncated
  string, and the call returns `PIXIE_ERROR_BUFFER`.
- Null sessions, null outputs, and indices out of range return `PIXIE_ERROR_ARGUMENT`.
- `pixie_api_version` returns `PIXIE_CAPI_VERSION` (17), raised when a function's meaning or a
  struct's layout changes, or functions are added. Version 2 added the field metadata, version 3
  picking, destruction, and spawning, version 4 projects, modes, and authoring, version 5 the
  conversion between view pixels and world units, version 6 scene IDs, snapshots, and names,
  version 7 text fields, sprites' rectangles, and images, version 8 point lists and paths' lines,
  version 9 movement diagnostics and Edit-mode gizmo selection, and version 10 component
  admission queries and singleton type metadata, version 11 typed record-list access, and
  version 12 layer editor metadata, value-type flags and collision layer name queries, and
  version 13 the read-only body mass query, and version 14 native collider handle
  queries and drag editing, and version 15 capsule radius/height handles and the
  expanded handle-result layout, version 16 session physics debug options,
  and version 17 development module loading in the shared engine; a
  host checks the version and does not load a library of another one.
- A project or scene that cannot be read or written returns `PIXIE_ERROR_IO`. Folder paths are
  UTF-8.

## Session

`pixie_physics_debug_get/set` read/write session presentation bits: colliders (1),
body origins (2), dynamic centers of mass (4), velocities (8), sensors (16),
awake/asleep (32), contact normals (64). The header provides named constants.
Unknown bits or null outputs are ARGUMENT errors. Options are isolated per
session, survive Play/Stop/rebuild, and are never saved into scene components or
marked as authored edits. Explicit zero hides all native debug geometry.
Before an explicit selection, Edit retains its legacy authored collider/sensor
preview (reported as 17) and Play defaults to zero. After selection, geometry
comes from the latest synchronized physics state in both modes; invalid or
disabled backend bodies are omitted. Selection and manipulation overlays are
separate editor presentation and may still be visible.

Velocities show 0.25 seconds of travel; centers use dynamic backend mass data.
Awake markers and solid outlines are green for awake dynamic bodies and gray for
sleeping ones when enabled; sensors are pink. Contact segments show shape A to B
manifold normals, length 12 world units, once per manifold point, including
speculative contacts. They do not represent character/body coupling or all motor
contacts. Native debug reads/rendering never solve physics or consume events.

`pixie_collider_handles_2d` copies the shape origin, resize handle and rotation
handle in world units. Shape -1 addresses Collider2D; 0..255 addresses a compound
item. Drawables are not consulted. `pixie_collider_drag_2d` takes an absolute
world pointer: kind 0 resizes symmetrically about the shape origin, kind 1 changes
the shape's local rotation, and kind 2 edits an upright character capsule's height.
The rotation snap flag uses 15-degree increments.
Box dimensions resize independently; circles change radius and have no rotation
handle; polygons scale uniformly about their local origin. Entity transforms,
shape offsets, other compound items and compatibility data remain unchanged.

For CharacterMotor2D, shape -1 returns a lateral radius handle and a top-cap
height handle (`canHeight = 1`, `canRotate = 0`). Radius dragging changes radius
from horizontal distance to the center while retaining halfSegment. Height
dragging changes halfSegment from vertical distance minus radius, clamped at zero;
the full height is 2*(halfSegment + radius). Radius clamps just above the strict
float backend minimum at the world's unitsPerMeter. The same native capsule
validator guards both tools and simulation without allocating a backend to query
handles. Character transforms must be upright and unit-scale; conflicting motion
or geometry owners and duplicate/invalid world scales have no capsule handles.

Both calls require Edit mode. Missing/unsupported geometry returns NOT_FOUND;
invalid arguments, rejected drags and Play-mode calls return ARGUMENT. Rejected
drags do not change authored data. Successful drags mark an authored scene
changed and never advance time. Singular transforms and nonuniformly scaled circles
have no handles. These are authoring operations, not a
claim of numerical qualification for every resulting physics configuration;
normal scene physics validation and diagnostics still apply.

`pixie_body_mass_2d(session, entity, &mass)` copies effective native body properties:
mass in kg, solid area in m², body-local center in world units, and inertia about
that center in kg*worldUnits². Area sums enabled nonsensor geometry, including
zero-density shapes; overlaps add. Static/kinematic bodies have zero effective
mass, center and inertia, while retaining their solid area. A mass override changes
mass/inertia without changing area or center. Fixed rotation suppresses angular
response; the query reports the backend's effective properties.

In Edit the query synchronizes authored physics without stepping time or marking
the scene unsaved. In Play it reads the latest synchronized state. Missing,
disabled or invalid rigid bodies return `PIXIE_ERROR_NOT_FOUND` without writing
the output; movement diagnostics explain invalid configuration. Null session or
output returns `PIXIE_ERROR_ARGUMENT`. Results are copied, never serialized.

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
  `pixie_component_add_status` reads the native authoring admission reason without changing
  the entity; empty means allowed. `required` includes NUL, and null buffer/capacity 0 measures.
  Invalid type/entity returns argument/not-found. Conflicts return text with `PIXIE_OK`;
  `pixie_component_add` enforces the rule and returns `PIXIE_ERROR_ARGUMENT` on rejection.
  Existing components are retained; scene/snapshot loading stays permissive for repair.
  `PIXIE_TYPE_SINGLETON` marks a component limited to one entity, so hosts can preflight
  multi-entity additions before applying any change.
  `PIXIE_FIELD_RECORD_LIST` (12) carries no PixieValue. `pixie_record_info` returns
  the registered item type and count; ordinary `pixie_field_*` metadata for that type
  describes item fields. `pixie_record_edit` inserts a default item or removes one,
  with a 256-item bound and validated indices. Numeric, point and text item data use
  `pixie_record_get/set`, `pixie_record_get/set_points`, `pixie_record_get/set_text`.
  Every call includes owner entity/type/list, item index and child field. Indices
  are invalidated by structural list changes/restoration; no native pointers escape.
  Point capacity/count is in points, stored as interleaved doubles; capacity 0 measures,
  and smaller buffers receive a prefix with the full count, matching root point lists.
  Record text `required` includes NUL; capacity 0 measures and truncation returns BUFFER.
  Wrong kinds, negative/out-of-range indices and read-only writes return ARGUMENT;
  stale entities return NOT_FOUND. The owner is marked changed after successful writes.
  Default-constructible registered item types are required. This API edits one list
  level; nested record data can be serialized but requires a future path-aware tool.
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
  for an entity with neither. In Edit, a collider/character gizmo may instead return
  its world-axis-aligned outline bounds; Play keeps drawable-only behavior.
- `pixie_entity_view_path(entity, xy, capacity, &count)` gives the line a path was last drawn along,
  corners rounded, in view pixels, filling and counting as `pixie_entities` does, or
  `PIXIE_ERROR_NOT_FOUND` for an entity without a path.
- `pixie_entity_destroy` destroys an entity at once. It is meant for what a host can pick:
  destroying an entity the game holds on to, such as its controls, is the host's risk.
- `pixie_particle_remove_at` removes the particle nearest a point within `radius` pixels, or
  returns `PIXIE_ERROR_NOT_FOUND`.
- `pixie_spawn_at` asks the game to create something at a point (`Project::SpawnAt`) and reports
  through `created` whether it did.

## Collision layer metadata (version 12)

`PIXIE_EDITOR_LAYER_MASK` marks a native Int field as a 31-checkbox layer filter;
its `references` text identifies CollisionLayers2D. Category and mask values and
scene format remain unchanged. `PIXIE_EDITOR_LAYER_NAMES` marks the configuration's
RecordList. Its CollisionLayer2D items contain `slot` (1..31) and UTF-8 `name`.
Item order is not the bit number. Use the existing typed record APIs to author
configuration; empty names never clear collider bits.

`PIXIE_TYPE_VALUE` marks reflected record metadata rather than an addable component.
The host omits these types from Add component menus; native admission rejects them.
Their fields remain discoverable for record-list editors and serialization.

`pixie_collision_layer_name(session, slot, buffer, capacity, &required)` resolves a
slot to its current display name, including **Unnamed layer N** fallback. Without
a configuration, slot 1 is **Default**. With an explicit empty configuration, all
slots are unnamed. Duplicate slots are diagnosed and unnamed; duplicate
configurations make all slots unnamed. These naming errors do not change filters.
The query allocates no physics backend and advances no simulation. Capacity 0
measures; required includes NUL. Negative capacity, slots outside 1..31, missing
required output, or null buffer with positive capacity return ARGUMENT. A short
buffer returns BUFFER with a terminated prefix. Movement status includes naming
diagnostics even in scenes without geometry.

## Non-goals

No Name editing, no choice of scene other than the start scene, and no fields of nested structs
(such as a particle pool's style) yet.

## Validation

`tests/capi/CapiTest.c` is a C11 program that includes only `pixie_capi.h` from the engine, loads a
shared engine at run time (`LoadLibrary` or `dlopen`, as P/Invoke does), optionally selects a
development module with `--module`, creates a hidden GLFW
window with an OpenGL 4.5 core context, and checks: the version; type and field lookup; the
metadata of a field shown in degrees, and a tooltip on every field; the entity
list and the oldest entity's name; writing a field and reading it back; time, the 0.25 s clamp,
pause, and single steps; a key press; rendering into a texture at a new size, with content; errors
for a bad type and a short buffer; finding a type by name; the view point of an entity turned
into its transform's position and back; an entity's scene ID found again, a rename, and a snapshot
that brings the entity back under the same ID after it is destroyed (and is refused while it
exists); that a session without a project
plays and refuses play, stop, and save; creating an entity, adding and removing a component, and
destroying it; and reset. CTest runs it against the shared engine with each selected game module.

With `--project <folder>`, on the shared engine without a game module, it drives a whole project instead: it creates one
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

## Scene movement diagnostics

`pixie_movement_status(session, buffer, capacity, &required)` returns a summary
from the latest native movement synchronization/update and last particle-physics
tick, without stepping time. Invalid particle pool settings appear with their
entity number; the pool falls back to ballistic motion until corrected.
`required` includes the trailing NUL. Query with null buffer and capacity 0;
then provide that capacity. A short buffer returns PIXIE_ERROR_BUFFER. The
summary shows up to eight errors and a remaining count; an empty result needs
one byte. The editor polls it separately from simulation. Edit-mode picking
falls back to native collider/character outlines when no drawable was picked;
box selection includes those gizmos. Current authoring and runtime ownership
are documented in [scene movement](../../editor-movement.md).
