# PixieEngine Promoted Work

**Document ID:** GLOBAL-TODO

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** formally promoted work, gates, dependencies, and completion criteria

**Surface:** work-scheduler

**Boundary:** global

**Authority role:** scheduler

**Mutation:** authored

**Indexing:** include

**Last promotion decision:** 2026-10-13

## Purpose and admission rule

This file is the only scheduler. It holds work that the owner has formally
promoted, with enough scope to execute and close it. It does not hold ideas
([`docs/proposals/`](docs/proposals/README.md)), direction
([`ROADMAP.md`](ROADMAP.md)), or completion narratives
([`docs/archive/`](docs/archive/README.md)).

An entry whose scope is large, or that decomposes into many small tasks, keeps
one entry here and its breakdown in [`TASKLIST.md`](TASKLIST.md). The rules are
in the [documentation policy](docs/governance/documentation-policy.md#scheduling-todomd-and-tasklistmd).

Unchecked or `promoted` entries are not automatically authorized to start; an
entry may keep an explicit gate after promotion.

## Current promoted work

Promoted on 2026-10-13 from
[PIXIE-PROPOSAL-004](docs/proposals/scene-model-and-anchors.md) and the
owner's answers in the
[decision record](docs/archive/object-model-and-editor-decisions-2026-10-13.md),
with the owner's authorization to execute them. The entries are ordered so
that simple work, and work that later entries depend on, comes first; an entry
starts when its gates are met. Scene serialization is postponed and is not
promoted.

Out of scope for every entry below, unless an entry says otherwise: scene
serialization and the `scene` boundary, stable entity IDs, play mode as a copy
of a Scene into a World, hot reload, C# scripting, sprites loaded from image
files, audio, and physics.

### `PIXIE-TODO-005` — Input, 2D camera, and instanced shapes

**Status:** promoted

**Source:** [PIXIE-PROPOSAL-004](docs/proposals/scene-model-and-anchors.md), anchors 1 to 3

**Accepted scope:** in the provisional engine library: input state (keys, mouse buttons, cursor position, scroll) fed by the player's GLFW window and readable by the App, with presses that are not lost between fixed updates; a 2D camera (position and zoom, screen-to-world and world-to-screen, an orthographic projection) on PixieLib math; small OpenGL helpers (shader programs, buffers); an instanced renderer for triangles, circles, and quads with per-instance position, rotation, scale, and color. A `shapes` sample draws thousands of shapes, pans and zooms the camera with the mouse, and marks the cursor's world position. The `platform` and `render` boundaries stay undecided, and this code stays provisional.

**Dependencies and gates:** 1. PIXIE-TODO-003 (PixieLib math)

**Completion criteria:** (1) Debug and Release build with zero warnings; (2) the `shapes` sample draws 10,000 instanced shapes, pans and zooms, and its cursor marker stays under the cursor at every zoom; (3) its measured frame and update rates are recorded with machine, configuration, and commit; (4) its `README.md` describes it.

**Breakdown:** [`TASKLIST.md`](TASKLIST.md#pixie-todo-005--input-2d-camera-and-instanced-shapes)

### `PIXIE-TODO-006` — Runtime module: World, entities, Behaviours, and phases

**Status:** promoted

**Source:** [PIXIE-PROPOSAL-004](docs/proposals/scene-model-and-anchors.md), part 1; the `runtime` boundary of [PIXIE-PROPOSAL-001](docs/proposals/engine-module-boundaries.md)

**Accepted scope:** the `runtime` module, with its `MANIFEST.md`, `README.md`, and `REFERENCE.md`: a World over an EnTT registry, with its own time and a seeded random number generator; an EC-style entity handle (create, destroy, and add, get, has, and remove components); `Behaviour` classes with `Update()`, stored as components; the update phases `PreUpdate`, `Update` (Behaviours), and `PostUpdate`, with engine systems registered per phase; destruction deferred to the end of a step; the built-in components `Transform2D`, `Velocity2D`, and `Shape`, with the previous transform kept for render interpolation; and a render system that draws shapes through the instanced renderer. Tests run through CTest.

**Dependencies and gates:** 1. PIXIE-TODO-003 (EnTT, PixieLib); 2. PIXIE-TODO-005 (renderer)

**Completion criteria:** (1) Debug and Release build with zero warnings; (2) tests pass for phase order, Behaviour lifecycle, deferred destruction, and identical random sequences from the same seed, and a deliberately broken copy of each makes its test fail; (3) the module's documents exist and match the public headers.

**Breakdown:** [`TASKLIST.md`](TASKLIST.md#pixie-todo-006--runtime-module-world-entities-behaviours-and-phases)

### `PIXIE-TODO-007` — Anchor 1: boids steered by the user

**Status:** promoted

**Source:** [PIXIE-PROPOSAL-004](docs/proposals/scene-model-and-anchors.md), anchor 1

**Accepted scope:** a `boids` sample. Each boid is an entity with a `Boid` Behaviour that steers toward or away from the mouse within a radius, with a maximum speed and force, and wraps at the view edges. Boids are drawn as triangles oriented by their velocity, interpolated. Keys switch between attracting and repelling and change the count.

**Dependencies and gates:** 1. PIXIE-TODO-006

**Completion criteria:** (1) 1,000 boids at 60 updates per second, measured and recorded; (2) the steering responds to the mouse as the `README.md` describes; (3) zero warnings in Debug and Release.

**Breakdown:** [`TASKLIST.md`](TASKLIST.md#pixie-todo-007--anchor-1-boids-steered-by-the-user)

### `PIXIE-TODO-008` — Spatial grid and anchor 2: flocks

**Status:** promoted

**Source:** [PIXIE-PROPOSAL-004](docs/proposals/scene-model-and-anchors.md), anchor 2

**Accepted scope:** a uniform spatial grid in the `runtime` module, rebuilt in `PreUpdate`, that answers radius queries. A `flocks` sample: boids steered by separation, alignment, and cohesion with their neighbours, and by the mouse; each writes only its own acceleration in `Update`, and `PostUpdate` integrates. Keys tune the weights until the editor exists.

**Dependencies and gates:** 1. PIXIE-TODO-007

**Completion criteria:** (1) grid tests agree with a brute-force search over random points; (2) the result of a step does not depend on update order (a test shuffles it); (3) 5,000 flocking boids at 60 updates per second, measured and recorded; (4) zero warnings in Debug and Release.

**Breakdown:** [`TASKLIST.md`](TASKLIST.md#pixie-todo-008--spatial-grid-and-anchor-2-flocks)

### `PIXIE-TODO-009` — Particles and anchor 3: attractors

**Status:** promoted

**Source:** [PIXIE-PROPOSAL-004](docs/proposals/scene-model-and-anchors.md), anchor 3

**Accepted scope:** in the `runtime` module: a `ParticleEmitter` component that owns a contiguous particle pool (emission rate, lifetime, capacity); batch affectors that receive the whole pool; `Attractor` entities and an affector that pulls particles toward them; a short-range particle-to-particle interaction through a spatial grid; particles drawn as instanced circles straight from the pool. A `particles` sample places and drags attractors with the mouse and switches between 1, 10, 100, and thousands of particles.

**Dependencies and gates:** 1. PIXIE-TODO-006; 2. PIXIE-TODO-008 (spatial grid)

**Completion criteria:** (1) 100,000 particles against up to 16 attractors, and 10,000 particles with short-range interaction, each at 60 updates per second, measured and recorded; (2) orbits form around an attractor as the `README.md` describes; (3) zero warnings in Debug and Release.

**Breakdown:** [`TASKLIST.md`](TASKLIST.md#pixie-todo-009--particles-and-anchor-3-attractors)

### `PIXIE-TODO-010` — Engine side for editor hosts: host loop, render target, reflection, and `capi`

**Status:** promoted

**Source:** owner decision on 2026-10-13: the viewport, reflection, and gameplay-code recommendations, and [PIXIE-PROPOSAL-002](docs/proposals/editor-native-bridge.md)

**Accepted scope:** general engine features that an editor, tools, and tests use alike: a loop that a host drives (step and render called from outside, which `pixie::Run` also uses); rendering into an offscreen target whose color texture a host displays; input fed from host events. Field descriptors (name, type, offset) registered per component, for the built-in and the anchors' components. The `capi` module, with its module documents: a shared library whose interface is C only, which creates and destroys a World from a project, steps it, renders it to a texture, takes input, enumerates entities and their components, and gets and sets fields through the descriptors, returning result codes and letting no exception cross the boundary. Each anchor's game code becomes a static library linked into its player executable and into a per-project `capi` library. On the host side, the engine library never calls GLFW.

**Dependencies and gates:** 1. PIXIE-TODO-007; 2. PIXIE-TODO-009 (all three anchors exist)

**Completion criteria:** (1) a test program that includes only the C header drives each anchor in a hidden window, renders it to a texture, and reads and writes a field; (2) the players still run as before; (3) zero warnings in Debug and Release; (4) the `capi` documents match its header.

**Breakdown:** [`TASKLIST.md`](TASKLIST.md#pixie-todo-010--engine-side-for-editor-hosts-host-loop-render-target-reflection-and-capi)

### `PIXIE-TODO-011` — C# ImGui editor over the anchors

**Status:** promoted

**Source:** owner decision on 2026-10-13: the editor starts as an ImGui view through Hexa.NET.ImGui; [PIXIE-PROPOSAL-002](docs/proposals/editor-native-bridge.md)

**Accepted scope:** a C# editor under `editor/`, in its own solution, referencing `PixieInspector.ImGui` 0.1.0 from NuGet with the GLFW and OpenGL3 backends of Hexa.NET.ImGui, on an OpenGL 4.5 core context. P/Invoke over `capi` through source-generated `LibraryImport`; a docked viewport that shows the World's texture with `ImGui.Image` and forwards the mouse to the engine; an entity list; an inspector bound by PixieInspector to hand-written C# proxies whose properties call into `capi`; pause, resume, single step, and reset. It opens the `capi` library of any of the three anchors.

**Dependencies and gates:** 1. PIXIE-TODO-010

**Completion criteria:** (1) for each anchor, the editor shows it running, the mouse steers it through the viewport, and editing a value in the inspector changes the running World; (2) the editor builds with zero warnings; (3) its `README.md` describes building and running it; (4) nothing is copied from PixieInspector's repository.

**Breakdown:** [`TASKLIST.md`](TASKLIST.md#pixie-todo-011--c-imgui-editor-over-the-anchors)

## Completed

`PIXIE-TODO-001` (documentation foundation) completed on 2026-10-11. Its
entry, the policy review, and its verification are preserved in the
[archive record](docs/archive/todo-001-documentation-foundation-2026-10-11.md).

`PIXIE-TODO-002` (main loop and the hello-gl sample) completed on 2026-10-11.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-002-main-loop-hello-gl-2026-10-11.md).

`PIXIE-TODO-003` (PixieLib, GLM, and EnTT vendored) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-003-vendor-pixielib-glm-entt-2026-10-13.md).

`PIXIE-TODO-004` (documentation verifier) completed on 2026-10-13. Its entry,
breakdown, and verification are preserved in the
[archive record](docs/archive/todo-004-documentation-verifier-2026-10-13.md).
