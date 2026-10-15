# Runtime Technical Reference

**Document ID:** RUNTIME-REFERENCE

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** the contract of the runtime module: World, Entity, Behaviour, systems, phases, time, randomness, the built-in components, and scenes

**Surface:** technical-reference

**Boundary:** runtime

**Authority role:** normative

**Mutation:** authored

**Indexing:** include

## Mental model

The object model is an EC-style API over ECS storage
([`ROADMAP.md`](../../../ROADMAP.md#active-guardrails)). Components are data-only structs, each
type in its own contiguous EnTT storage. Game logic is a `Behaviour`: a class with `Update()` that
is itself a component, so every Behaviour class also has contiguous storage. Work shared by many
entities (integration, and later spatial queries) is a system: a function over the whole World.

## Public entry points

All under `runtime/include/pixie/runtime/`:

| Header | Provides |
|---|---|
| `World.hpp` | `World`, `SystemPhase`, `System`, and the template members of `Entity` |
| `Entity.hpp` | `Entity`, a handle (World pointer and EnTT entity) |
| `Behaviour.hpp` | `Behaviour`, the base of logic classes |
| `Random.hpp` | `Random`, the seeded generator |
| `Transform2D.hpp` | `Transform2D` and `PreviousTransform2D` |
| `Velocity2D.hpp` | `Velocity2D` |
| `Acceleration2D.hpp` | `Acceleration2D` |
| `SpatialGrid.hpp` | `SpatialGrid`, radius queries over points |
| `SpatialIndex.hpp` | `AddSpatialIndex`, the system that keeps a grid filled |
| `ParticleSystem.hpp` | `ParticleSystem`, a component owning a particle pool |
| `ParticleSpawner.hpp` | `ParticleSpawner`, a component that feeds a system, and `EmitFromSpawner` |
| `ParticleAffector.hpp` | `ParticleAffector`, the base of batch logic over a pool |
| `ParticleStep.hpp` | `AddParticleStep` and `DrawParticles` |
| `Attractor.hpp`, `AttractorAffector.hpp` | Attractor entities and the affector that pulls toward them |
| `InteractionAffector.hpp` | Short-range push between the particles of a pool |
| `ComponentTypes.hpp` | Field descriptors: `ComponentTypes`, `FieldKind`, `FieldValue`, `DescribeRuntimeComponents` |
| `Scene.hpp` | `SceneId`, `AddToScene`, `SceneEntityWith`, reading and writing scene files, and entity snapshots (`WriteEntities`, `ReadEntities`) |
| `Picking.hpp` | `PickAt`, `PickIn`, `PickRadius`, and `RemoveParticleAt`, for a tool's selection |
| `Project.hpp` | `Project`, a game as the engine sees it, and `CreateProject` |
| `ProjectFile.hpp` | Project folders: `ProjectFile`, `OpenProjectFolder`, `CreateProjectFolder`, `WriteProjectFile` |
| `SceneProject.hpp` | `SceneProject`, a project whose World is only its scene, and `PanAndZoom` |
| `ProjectSession.hpp` | `ProjectSession`, a running project, `SessionSettings`, and `SessionMode` |
| `ProjectApp.hpp` | `ProjectApp`, which runs a project in `pixie::Run`'s window |
| `Shape.hpp` | `Shape`, an entity's look |
| `Name.hpp` | `Name`, an optional label |
| `ShapeDrawing.hpp` | `SubmitShapes`, the render system for shapes |

Use `Entity`'s templates through `World.hpp`, which defines them.

## The step

`World::Step(delta, input)` advances the World by `delta` seconds:

1. `PreUpdate` systems, in the order added. The first is the World's own: it copies every
   `Transform2D` into the entity's `PreviousTransform2D`.
2. Behaviours: for each Behaviour class, in the order the classes were first added to the World,
   every instance in its storage's order. An instance's `Start` runs right before its first
   `Update`. A class first added during a step is updated from the next step on.
3. `PostUpdate` systems, in the order added. The first is the World's own: it adds each
   `Acceleration2D` to its `Velocity2D` (`linear += acceleration * delta`), then integrates every
   `Velocity2D` into its `Transform2D` (`position += linear * delta`, `rotation += angular *
   delta`), so the position moves with the new velocity (semi-implicit Euler).
4. The destructions requested during the step, then the deferred actions, in request order.

Each system and Behaviour receives the World's `FrameTime`: `delta`, `elapsed` (the World's
simulated seconds before this step), and `index` (the World's step count before this step); `alpha`
is 0. `Steps()` and `Elapsed()` report the totals. The World's time is its own: two Worlds, or a
paused one, keep separate clocks.

## Behaviours

- A Behaviour derives from `Behaviour` and overrides any of `Start`, `Update(time, input)`, and
  `Stop`. `Self()` is its entity; `GetWorld()` its World.
- `Entity::AddBehaviour<T>(args...)` constructs it in place, replacing a `T` already there.
  (`Entity::Add<T>` returns the component, or nothing for an empty tag type, which EnTT stores
  without instances.)
- `Stop` runs once, if `Start` ran, when the Behaviour leaves: removed with `Remove<T>()`, its
  entity destroyed, or the World destroyed. At World destruction, Behaviours stop class by class
  before any other component is removed, so `Stop` can still read its entity.
- `Update` should write only its own entity's components and read the rest. Shared results come
  from systems; that keeps a step independent of update order.
- Behaviours move when their storage grows or shrinks. Keep `Entity` handles, not pointers or
  references to Behaviours or components, across steps.

## Structural changes and destruction

- `World::Destroy(entity)` and `Entity::Destroy()` take effect at the end of the step when called
  during one, and immediately otherwise. Until then the entity stays alive and readable.
- Creating entities, or adding and removing components, while a step runs may move storage that a
  system or a Behaviour class is iterating. Do it through `World::Defer(action)`, which runs
  `action` at the end of the step (immediately outside a step).
- `Entity::Add<T>` replaces an existing `T`. An `Entity` stays valid to copy after its entity is
  gone; `IsAlive()` tells.

## Built-in components

- `Transform2D`: `position` (`pxPoint`, world units), `rotation` (radians, from +X toward +Y, so
  clockwise on screen, since Y points down), `scale` (`pxVec2`).
- `PreviousTransform2D`: added with every `Transform2D` and refreshed at each step's start;
  `World::ResetInterpolation(entity)` makes it equal the current transform, so a jump is not
  drawn as motion.
- `Velocity2D`: `linear` (`pxVec2`, units per second) and `angular` (radians per second).
- `Acceleration2D`: `linear` (`pxVec2`, units per second squared). It persists until changed; a
  steering Behaviour overwrites it every step.
- `Shape`: a `ShapeKind`, a half extent `size` before the transform's scale, and a `pxColorRgba`.
- `Name`: a string, added by `World::Create(name)` when the name is not empty.

## Order independence and neighbours

A step's result does not depend on the order Behaviours run in when each Behaviour writes only its
own components and reads others' state that no Behaviour writes during the step. The runtime is
shaped for that: positions and velocities change only in `PostUpdate`, and a steering Behaviour
writes its `Acceleration2D`.

Neighbour queries read a snapshot. `SpatialGrid` is a uniform grid of square cells whose occupied
cells are found through a hash table sized to the items, so it has no bounds. Fill it with `Clear`,
`Add`, and `Build`; `ForEachWithin(center, radius, fn)` then calls `fn(id, position,
distanceSquared)` once for each item within `radius`, the boundary included. A cell size near the
usual query radius keeps queries cheap. `AddSpatialIndex<Member>(world, owner, cellSize)` gives
`owner` a `SpatialGrid` component and adds a `PreUpdate` system that refills it, at the start of
every step, from each entity with a `Transform2D` and a `Member` component.

## Particles

Particles are not entities. A `ParticleSystem` component owns a contiguous `std::vector` of
`Particle` (32 bytes, in float: position, previous position, velocity, age, lifetime; declared by
the engine library, whose `ParticleRenderer` reads it as its vertex format), up to `capacity`, with
its draw `style` and its affectors. It is move-only, since it owns its affectors. `Emit` adds one
particle directly, and fails while the pool is full.

A `ParticleSpawner` is a component on an entity of its own, with a `Transform2D`, that feeds the
system named by its `system` field; several spawners may feed one system, and a spawner whose
`system` is none emits nothing. It emits `rate` particles per second that live `lifetime` seconds
(0 is forever), at a speed in `[speedMin, speedMax]`:

- `shape` (`EmissionShape`) is where a particle starts, around the spawner and turned with its
  rotation: `Point`; `Circle`, anywhere inside a disc of `radius`; `Ring`, on a circle of `radius`;
  `Rectangle`, anywhere inside half extents `size`; `Segment`, anywhere from the spawner to the
  offset `segmentEnd`.
- `directionMode` (`DirectionMode`) is the angle the direction is measured from: `Fixed`, the
  spawner's rotation; `ShapeNormal`, outward from the shape where the particle starts (radial for a
  point, a circle, or a ring; the nearest edge's normal for a rectangle; the segment's left normal,
  +90 degrees); `TowardTarget`, toward the `target` entity's `Transform2D` (the rotation without
  one).
- The particle leaves at that angle plus `direction`, within a cone `spread` wide. Angles are in
  radians.

`AddParticleStep(world)` adds the `particles.step` system to `PostUpdate`, after the World's
integration, so spawners and attractors are where the step left them. For each system, in turn:

1. every particle's `previous` becomes its `position` and its age grows by the step; a particle
   past a non-zero lifetime leaves the pool, and the last particle takes its place, so pool order
   is not stable;
2. every spawner that feeds the system emits what it owes for the step, in storage order, while
   the pool is below `capacity` (a spawner that meets a full pool drops the rest of its step);
3. the enabled affectors run, in the order added; then the particles an affector killed
   (`KillParticle`, which marks the particle as past its lifetime) leave the pool, so they are not
   drawn again;
4. every particle moves by its velocity.

A `ParticleAffector` is the particles' counterpart of a Behaviour: a class whose
`Apply(world, system, particles, time)` receives the whole pool. Affectors should change
velocities and read positions, which none of them changes, so their result does not depend on
particle order. The runtime provides two:

- `AttractorAffector`: every entity with a `Transform2D` and an `Attractor` pulls each particle
  with `strength * offset / (distance^2 + softening^2)^(3/2)`, ignoring particles beyond a
  non-zero `reach`. A circular orbit at distance `r` has speed `sqrt(strength / r)`. The
  attractor's `size` is its body's radius, times the transform's larger scale; it does not change
  the pull. With `killOnContact`, a particle that comes within the body is removed.
- `InteractionAffector`: particles closer than `radius` push each other apart with
  `stiffness * overlap`, and `damping` pulls their velocities together while in contact. It builds
  a `SpatialGrid` over the pool every step (the grid's ids are then pool indices: grid ids are
  opaque 32-bit values), computes every change from the state before it, then applies them all.

`DrawParticles(world, renderer, camera, alpha)` draws every pool with its `style`, interpolated on
the GPU.

## Picking

`Picking.hpp` finds things by where they are, for a tool's selection. Only entities with a
`Transform2D` and a `Shape` can be picked: what is drawn can be clicked. An entity covers
`PickRadius`, its shape's larger half extent times the transform's larger scale.
`PickAt(world, point, tolerance)` returns the entity whose radius plus `tolerance` covers the
point, nearest center first, or `entt::null`; `PickIn(world, box)` lists the entities whose center
lies in the box, oldest first. Particles are not entities: `RemoveParticleAt(world, point, radius)`
removes the particle nearest the point within the radius, from whichever pool holds it, at once.
Positions are the current ones, in world units.

## Reflection

`ComponentTypes` lists the component types a tool may inspect and edit, each a
`ComponentDescriptor`: a name, the EnTT type hash, its fields, a version (1 unless `Version(n)`
sets it; saved with every instance in a scene), a `find` function that returns the component of an
entity or null (null for a destroyed entity too), an `add` function that returns the component,
added with its defaults when the entity has none (a Behaviour is added as `AddBehaviour` adds it;
null for a type that is not default-constructible, and the result is null for a destroyed entity),
and a `remove` function. `add` and `remove` are not for use during a step. A type is described in
code, once:

```cpp
types.Add<Transform2D>("Transform2D")
    .Field<&Transform2D::position>("position")
    .Field<&Transform2D::rotation>("rotation");
```

Each `FieldDescriptor` has a name, a `FieldKind` deduced from the member's type (`bool`, other
integers, `float`, `double`, an enum, `pxPoint`, `pxVec2`, `pxRect`, `pxColorRgba`, `entt::entity`; any other type
does not compile), optional names for an enum's values 0, 1, ..., and `read` and `write` functions
that move the value in and out of a component instance as a `FieldValue` (four doubles; the kind
says which are used). Colors are clamped to 0 to 255 and rounded when written.

Each field also carries a `FieldMetadata`: how a tool shows and edits it, in the vocabulary of
PixieInspector's node options. Builder calls chained after a `Field` set it for that field:
`Label`, `Tooltip`, `Help`, `Order` (by default the declaration position), `ReadOnly`, `Editor`
(`FieldEditor`: Auto, Number, Slider, Toggle, Choice, Color, Arc), `Range(min, max, step)`,
`Scrub(perPixel, axis)`, `Unit(suffix, factor, offset)` (shown = held * factor + offset),
`Degrees(suffix)` (radians shown in degrees), `Arc(spread)` (this angle is the center of an
arc whose width is the sibling field `spread`), and `References(type)` (for an entity field, the
component type an entity needs to be a valid choice). Range, step, and scrub are in the displayed unit.
The registry is the single source of this metadata
([`ROADMAP.md`](../../../ROADMAP.md#active-guardrails)): the C API exports it and the editor builds
its inspectors from it, so nothing about a field is written twice. Every field of the runtime and of
the three anchors carries a tooltip. Fields are reached
through accessor functions generated from member pointers rather than byte offsets, so a
component need not be standard layout. Empty tag types cannot be described.

A type marked with `Scenario()` holds a scenario: the values a scene is built from rather than live
state. A tool offers to rebuild the scene after editing one.

`DescribeRuntimeComponents` describes `Transform2D`, `Velocity2D`, `Acceleration2D`, `Shape`,
`Attractor`, `ParticleSystem` (its capacity), and `ParticleSpawner`. The C API and scenes read and
write fields only through these descriptors.

A Behaviour can be described like any component (`types.Add<Spinner>("Spinner").Field<...>`); it is
then saved in scenes and added back as a Behaviour.

## Scenes

A scene is the document saved to disk; the World is the live instance built from it
([`ROADMAP.md`](../../../ROADMAP.md#active-guardrails)). An entity belongs to the scene when it has
a `SceneId`: a random 64-bit ID, never 0, that stays the same through saving and loading.
`AddToScene(entity)` gives an entity one if it has none and returns it; `NewSceneId()` makes one
from its own generator, so a World's `Random` is not touched. Entities without a `SceneId`, such as
those project code generates from a count, are not saved.

`SceneEntityWith<T>(world, name, value)` is how a project's `Build` finds what its scene holds: an
entity with a `T`, or, when there is none (no scene was loaded, or it lacks one), a new scene entity
named `name` with `value`, which a save then keeps. The samples find their scenario and controls
this way, so they build the same World with and without a scene.

`WriteScene(world, types)` returns the scene as JSON text; `ReadScene(world, types, text)` adds its
entities to a World and returns a `SceneReport` (`ok`, `error`, and `warnings`).
`SaveSceneFile` writes through a temporary file beside the target (`<file>.saving`) and then
replaces it, so a failed save leaves the old file; `LoadSceneFile` reads one. Reading is not
allowed during a step.

The format, version 1 (`kSceneFormat`, `kSceneVersion`):

```json
{
  "format": "pixie-scene",
  "version": 1,
  "seed": 1,
  "entities": [
    {
      "id": "e1749eb32e65f035",
      "name": "pool",
      "components": {
        "Transform2D": {"v": 1, "position": [12.5, -3.0], "rotation": 0.75, "scale": [2.0, 0.5]},
        "ParticleSystem": {"v": 1, "capacity": 5000}
      }
    }
  ]
}
```

- `seed` is the World's seed; reading a scene reseeds the World's `Random` with it.
- `entities` lists the scene's entities oldest first, each with its `id` (16 hexadecimal digits;
  reading accepts 1 to 16), its `name` when it has a non-empty `Name`, and its `components` by type
  name, in the order of `ComponentTypes`. `parent` is not written: there is no hierarchy yet.
- Each component holds its descriptor's version as `v`, then its fields in declaration order. A
  bool is `true` or `false`; an integer, float, or double is a number (a float as the shortest
  decimal that reads back as the same float); an enum is the name of its value (a number when it
  has none); a point or a vector is `[x, y]`; a rectangle is `[x, y, width, height]`; a color is
  `[red, green, blue, alpha]`; an entity is the referenced entity's `id`, or `null` for none or
  for an entity outside the scene (which also adds a warning).
- Arrays of plain values stay on one line; everything else is indented by two spaces.

Reading checks the format, the version (a newer one is refused), the list, and every ID (present,
not repeated, not already in the World) before adding anything, so a file that cannot be read adds
nothing. Then, per value:

- A field missing from the file keeps its default; a value of the wrong type, an unknown enum name,
  or a reference to an ID that is not in the file adds a warning and keeps the default.
- A component saved at a newer `v` than the descriptor's is read field by field, with a warning.
- Nothing the build does not know is lost: unknown components, unknown fields of known
  components, unknown keys of an entity (such as `parent`), and unknown keys of the document are
  kept, and written back unchanged by the next `WriteScene` of that World. A component that cannot
  be added (not default-constructible, or not an object in the file) is kept the same way.

Particle pools' contents, affectors, systems, and anything else without a descriptor are not
saved; project code builds them.

### Entity snapshots

`WriteEntities(world, types, entities)` writes some scene entities apart from their scene, for a
tool that brings them back or copies them: a document of the scene's form, with format
`pixie-entities` (`kEntitiesFormat`), the scene's version, and only the list of entities, in the
order given, each written as a scene writes it. An `Entity` field holds its target's scene ID,
whether the target is in the set or not. An entity that is gone or has no `SceneId` is left out,
with a warning.

`ReadEntities(world, types, text, ids)` adds such a set to a World and returns an `EntitiesReport`:
a `SceneReport` with the entities added, in the document's order. `SceneIds::Keep` gives them the
IDs written, none of which may be in the World (bringing back entities that were destroyed);
`SceneIds::New` gives them new IDs (a copy), and then an `Entity` field that named an entity of the
set names its copy, while one that named another entity keeps naming it, if it is in the World.
The checks and the kept unknown data are a scene's: a set that cannot be read, or, with `Keep`, one
with an ID already in the World, adds nothing. The World's seed is not part of a set and is left
alone. Not during a step.

## Projects and sessions

A `Project` is a game as the engine sees it: `Name`, `Background` (the clear color), `Describe`
(its own component types), `Build` (fills a new World; the camera's viewport is set), `Capture`
(before the World is rebuilt: read back what the next `Build` starts from, such as an edited
scenario component),
`BeforeStep` (outside the step: copy the host's view and input into the World, react to keys, move
the camera; structural changes are allowed), `SpawnAt` (a tool asks for something at a point, such
as an editor's "Ask the game to create one here" on empty space: a boid in boids and flocks, a particle at rest in
particles; false when the game creates nothing), and `DrawOverlay` (shapes drawn over the World). Every
game library defines `pixie::CreateProject()`, which returns the project with its defaults; one
binary links one game.

A `ProjectSession` runs one project: its World (seeded from `SessionSettings::seed`), a
`Camera2D`, the shape and particle renderers, an `Input`, and a `FixedStepper`. `Start(size)`
creates the GL objects and builds the World; `Stop` releases them; both need the context current.

- `Update(delta, input)`: the project's `BeforeStep`, then `World::Step`; it records the step's
  wall time (`LastStepMilliseconds`).
- `Advance(frameDelta)`: for a host that drives the session; runs the fixed steps the frame makes
  due with the session's own `Input`, forgetting presses after each, unless paused.
  `StepOnce` runs one step regardless; `SetPaused` pauses or resumes; `SetTimeScale` makes
  simulated time run faster or slower than wall time (0.01 to 8; the fixed step stays the same, so a
  faster scale runs more steps per frame); `Reset` lets the project `Capture`, then builds the World
  again with the same seed.
- `Render(alpha)` clears to the project's background and draws particles, then shapes, then the
  overlay, into the bound framebuffer; `RenderToTarget()` does it into the session's own
  `RenderTarget`, sized to the viewport, and returns its texture.

### Project folders

A project is a folder that holds one `<Name>.pixie` file, the project file, and its scenes. The
project file is JSON:

```json
{
  "format": "pixie-project",
  "version": 1,
  "name": "My Game",
  "startScene": "scenes/main.scene"
}
```

Its paths are relative to the folder, with forward slashes and no `.` or `..` part
(`IsProjectRelativePath`), so a project never depends on the machine it is on or on where the
engine is ([`ROADMAP.md`](../../../ROADMAP.md#active-guardrails)). `OpenProjectFolder(folder)`
reads it and fails when the folder holds no `.pixie` file or more than one, when the format or a
newer version does not match, or when the start scene is not such a path; a missing `name` is the
file's own name, and a missing `startScene` is `scenes/main.scene`. Keys it does not know are kept
in `ProjectFile::extras` and written back by `WriteProjectFile`. `CreateProjectFolder(folder)`
makes a new project in a folder that is empty or does not exist yet: the project takes the
folder's name, and gets its project file and a start scene whose one entity, `shape`, is a circle
at the origin.

### Edit and play

`OpenProject(folder)`, before or after `Start`, opens a project folder in a session; it fails, with
`Error()`, when the project or its start scene cannot be read. From then on the World is built
from a scene: `ReadScene` adds the scene's entities and reseeds the World's `Random` with the
scene's seed, then the project's `Build` adds what is not saved (systems, pools, generated
entities) and may read the scene's entities, such as a scenario. Warnings are logged.

A session with a project has two modes (`SessionMode`):

- `Edit`, where it starts: `Advance`, `StepOnce`, and `Update` run no step and the project's
  `BeforeStep` is not called; `Advance` pans and zooms the camera (`PanAndZoom`: middle button and
  wheel). Edits change the World, which stays the only copy of the state.
- `Play`: `Play()` writes the scene's entities to memory and builds the World again from them,
  unpaused; `StopPlaying()` builds it again from that copy and returns to `Edit`, so nothing done in
  play stays. The camera is kept across both.

`Reset` builds the World again from the scene's entities as they are now. `Save()` writes the
scene's entities to the start scene's file through `SaveTextFile`; in `Play`, it writes the ones
`Play` kept. `MarkChanged()` notes an edit, and is ignored outside `Edit`; `HasUnsavedChanges()`
reports it until the next save. `ProjectName()` is the project file's name, or the `Project`'s
without a project. A session without a project always plays, as before.

`SceneProject` is a `Project` whose `Build` adds nothing and whose `BeforeStep` pans and zooms: a
project folder without code runs as one, and a game with code can start from it.

`ProjectApp` is the player side: an `App` whose fixed updates call `Update` with `pixie::Run`'s
time and input and whose frames call `Render`. It logs the step cost once per second. With a
project folder open in its session (`OpenProject`, before `Run`), it plays the start scene.

### Projects with code

A project folder gets code of its own from the engine's templates (`templates/project-code/`),
written by `cmake -DPIXIE_PROJECT_DIR=<folder> -P cmake/PixieAddCode.cmake`, which the editor's
Add code runs:

- `CMakeLists.txt`, which includes `${PIXIE_ENGINE_DIR}/cmake/PixieProject.cmake` and calls
  `pixie_game`;
- `src/<Id>Project.hpp` and `.cpp`, where `<Id>` is the project's name as a C++ identifier (its
  letters and digits, each word capitalized: "my game" is `MyGame`): a `SceneProject` that adds
  nothing yet, and `Spinner`, an example Behaviour that turns its entity, declared with its `speed`
  field so the editor can add and edit it.

The script reads the name from the folder's `.pixie` file, and refuses a folder that is not a
project or that already has a `CMakeLists.txt`.

`cmake/PixieProject.cmake`, included after `project()`, adds the engine found at
`PIXIE_ENGINE_DIR` with `add_subdirectory`, and puts every binary of the project in
`<project>/bin/<config>/`. A project never records where the engine is: whoever builds it passes
`PIXIE_ENGINE_DIR`, and the editor passes the engine it runs from. The engine, when it is not the
top-level project, builds neither its samples nor its tests, does not write to its own `bin/`, and
does not build `pixie-scene-capi`.

`pixie_game(<name> SOURCES <file>... [INCLUDE_DIRS <dir>...] [PROJECT_DIR <dir>])` (in
`cmake/PixieGame.cmake`) makes three targets: `<name>-game`, a static library with the game's code, which defines
`pixie::CreateProject()` (its public include directories are `INCLUDE_DIRS`, `src/` by default);
`<name>`, its player; and `<name>-capi`, its C API library. The player is the Visual Studio startup
project of the calling directory. Every target links the C++ runtime statically, as the engine's do.
`PROJECT_DIR` is for a game built in another tree than its project, such as the engine's samples,
whose players land in the engine's `bin/`: the player opens that folder, and, when the engine's
tests are built, a CTest test `<name>-project` opens the project with the game's library
(`capi-test --open`).

The player's `main` is the engine's (`runtime/player/PlayerMain.cpp`, compiled into each player,
not into the runtime library). It opens the project folder given with `--project`, or else the
folder of `PROJECT_DIR`, or else the first folder above the executable that holds a `.pixie` file, and plays its start scene with
`ProjectApp`, titled with the project's name. It takes the options every player takes
(`pixie/RunOptions.hpp` in the engine library: `--ups`, `--fps`, `--vsync`, `--quit-after`,
`--screenshot`).

## Drawing

`SubmitShapes(world, renderer, alpha)` submits each entity with `Transform2D` and `Shape` to a
`ShapeRenderer`, at the transform interpolated `alpha` of the way from `PreviousTransform2D`:
position and scale linearly, rotation the short way around. The caller flushes the renderer with
its camera. It needs the GL context current, as the renderer does.

## Randomness

`World(seed)` seeds the World's `Random`. `Random` is xoshiro256** seeded through splitmix64; its
sequences depend only on the seed, on every compiler and platform. The first value for seed 0 is
pinned by a test, so a change to the generator does not go unnoticed.

## Threading

None. A World and its Behaviours are used from one thread at a time.

## Non-goals

No prefabs, nested scenes, or hierarchy, no binary scene format, no undo, no physics, no multithreaded
systems, no long-range all-pairs forces between particles.

## Validation

`tests/runtime/` (CTest `runtime-tests`) covers the phase order, the Behaviour lifecycle,
deferred destruction and actions, the World's time, velocity integration with the previous
transform, the Entity handle, the generator's determinism, the spatial grid against a brute-force
search, a step's independence from the Behaviours' update order (with a negative control that
breaks the rule and is caught), particle emission, aging, expiry, and capacity, a circular orbit
that keeps its radius, the interaction's independence from particle order, and the field
descriptors (kinds, round trips, color clamping, finding, adding, and removing components on
entities, metadata, and a tooltip on every runtime field), and scenes (a round trip over every
built-in component, a Behaviour, and an entity reference that writes back the same text; a loaded
Behaviour that runs; generated entities left out; the values as written; unknown components,
fields, and keys written back unchanged; files that cannot be read adding nothing; wrong values
warned about and left at their defaults; a reference outside the scene saved as none; and saving
and loading a file), entity snapshots (destroyed entities brought back with their IDs and values,
copies with new IDs whose references follow the copies inside the set and keep their targets
outside it, a copied Behaviour that runs, IDs already in the World refused, entities outside the
scene left out, and unknown data carried along), and project folders (a new project named after its folder, with its one
shape and no path outside it; a folder that is not empty refused; only a folder with one valid
project file opening; unknown keys written back; and which paths count as inside the folder).
Sessions, with their modes, are exercised by the samples and, through the C API, by `capi-test`,
which drives a whole project through `pixie-scene-capi`. `SubmitShapes` is exercised by the
samples that draw a World.
