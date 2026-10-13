# PixieEngine Task List

**Document ID:** GLOBAL-TASKLIST

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** task breakdowns of promoted TODO.md entries

**Surface:** task-scheduler

**Boundary:** global

**Authority role:** scheduler

**Mutation:** authored

**Indexing:** include

## Purpose

This file is a child scheduler of [`TODO.md`](TODO.md). Each section breaks
one promoted `TODO.md` entry into tasks, so that large entries, or entries made
of many small tasks, do not pollute `TODO.md`.

- Every section names its parent in `Parent`, and the parent links back here
  in `Breakdown`.
- Tasks decompose the parent's accepted scope and never extend it.
- When the parent completes, its section moves to
  [`docs/archive/`](docs/archive/README.md).

The full rules are in the
[documentation policy](docs/governance/documentation-policy.md#scheduling-todomd-and-tasklistmd),
and the section shape is in the
[template](docs/templates/TASKLIST-SECTION.md).

## Open breakdowns

Opened on 2026-10-13 for PIXIE-TODO-003 to PIXIE-TODO-011, in the order of
their parents. The breakdown of PIXIE-TODO-002 is in its
[archive record](docs/archive/todo-002-main-loop-hello-gl-2026-10-11.md).

## `PIXIE-TODO-003` — Vendor PixieLib, GLM, and EnTT

**Parent:** [PIXIE-TODO-003](TODO.md#pixie-todo-003--vendor-pixielib-glm-and-entt)

- `PIXIE-TASK-011` — **Status:** open — Download the PixieLib v0.2.0 source archive, verify it, and copy it unmodified to `external/pixielib/`.
- `PIXIE-TASK-012` — **Status:** open — Download the GLM 1.0.3 release archive, verify it, and copy it unmodified to `external/glm/`.
- `PIXIE-TASK-013` — **Status:** open — Download the EnTT v4.0.0 source archive, verify it, and copy it unmodified to `external/entt/`.
- `PIXIE-TASK-014` — **Status:** open — Wire the targets in `external/CMakeLists.txt`: PixieLib through its own project with `FETCHCONTENT_SOURCE_DIR_PIXIE_GLM` set to `external/glm/`, EnTT as an interface target, both with system include directories; link them to the engine and include their headers from one engine source.
- `PIXIE-TASK-015` — **Status:** open — Record the three in `DEPENDENCIES.md` and `.gitattributes`; verify a disconnected configure and both configurations with zero warnings.

## `PIXIE-TODO-004` — Documentation verifier

**Parent:** [PIXIE-TODO-004](TODO.md#pixie-todo-004--documentation-verifier)

- `PIXIE-TASK-016` — **Status:** open — `scripts/check_docs.py`: collect the versioned Markdown (outside `external/`, `build/`, `bin/`, and `.local/`) and check relative links and their anchors.
- `PIXIE-TASK-017` — **Status:** open — Check metadata: the required fields of indexable documents and their values against `docs/schemas/documentation-schema.json`, and the extra fields of archive records.
- `PIXIE-TASK-018` — **Status:** open — Check the schedulers and the archive router: section parents and back-links, unique task IDs, reasons on dropped tasks, and one router row per record with the same ID.
- `PIXIE-TASK-019` — **Status:** open — Make one deliberate break of each kind in a copy and watch the script fail; describe the script in `docs/development.md` and in the policy's enforcement section.

## `PIXIE-TODO-005` — Input, 2D camera, and instanced shapes

**Parent:** [PIXIE-TODO-005](TODO.md#pixie-todo-005--input-2d-camera-and-instanced-shapes)

- `PIXIE-TASK-020` — **Status:** open — Input state fed by GLFW in the player loop: keys, mouse buttons, cursor position, and scroll, with presses and releases kept until the next fixed update reads them.
- `PIXIE-TASK-021` — **Status:** open — OpenGL helpers: a shader program built from sources, with errors logged, and owning wrappers for buffers and vertex arrays.
- `PIXIE-TASK-022` — **Status:** open — `Camera2D` on PixieLib math: position, zoom, viewport size, view-projection matrix, and screen-to-world and world-to-screen conversion.
- `PIXIE-TASK-023` — **Status:** open — Instanced shape renderer: triangles, circles (distance-based edge in the fragment shader), and quads, with per-instance position, rotation, scale, and color, drawn with one call per shape kind.
- `PIXIE-TASK-024` — **Status:** open — `shapes` sample: 10,000 moving shapes, panning by dragging and zooming around the cursor with the scroll wheel, a marker at the cursor's world position; its `README.md`.
- `PIXIE-TASK-025` — **Status:** open — Verification: both configurations, the cursor marker at several zoom levels, and measured rates recorded with machine, configuration, and commit.

## `PIXIE-TODO-006` — Runtime module: World, entities, Behaviours, and phases

**Parent:** [PIXIE-TODO-006](TODO.md#pixie-todo-006--runtime-module-world-entities-behaviours-and-phases)

- `PIXIE-TASK-026` — **Status:** open — Create the `runtime` module: the `runtime/` library target and `docs/modules/runtime/` with `MANIFEST.md`, `README.md`, and `REFERENCE.md`.
- `PIXIE-TASK-027` — **Status:** open — `World` over an EnTT registry and an `Entity` handle: create, destroy, and add, get, has, and remove components.
- `PIXIE-TASK-028` — **Status:** open — Update phases: engine systems registered for `PreUpdate` and `PostUpdate`, Behaviours run in `Update`, and destruction deferred to the end of the step.
- `PIXIE-TASK-029` — **Status:** open — `Behaviour`: a base class with `Start`, `Update`, and `Stop`, stored as a component, with access to its entity and World.
- `PIXIE-TASK-030` — **Status:** open — Time and randomness: the World counts its own steps and simulated time, and owns a random number generator created from a seed.
- `PIXIE-TASK-031` — **Status:** open — Built-in components `Transform2D`, `Velocity2D`, and `Shape`; the previous transform kept each step; integration in `PostUpdate`; a render system that draws interpolated shapes.
- `PIXIE-TASK-032` — **Status:** open — CTest tests for phase order, Behaviour lifecycle, deferred destruction, and identical sequences from one seed, each seen failing on a broken copy.

## `PIXIE-TODO-007` — Anchor 1: boids steered by the user

**Parent:** [PIXIE-TODO-007](TODO.md#pixie-todo-007--anchor-1-boids-steered-by-the-user)

- `PIXIE-TASK-033` — **Status:** open — `boids` sample: the `Boid` Behaviour steering toward or away from the mouse within a radius, with maximum speed and force, wrapping at the view edges.
- `PIXIE-TASK-034` — **Status:** open — Keys for attract or repel and for the count; triangles oriented by velocity; its `README.md`.
- `PIXIE-TASK-035` — **Status:** open — Verification: 1,000 boids at 60 updates per second, measured and recorded, and the steering checked against the README.

## `PIXIE-TODO-008` — Spatial grid and anchor 2: flocks

**Parent:** [PIXIE-TODO-008](TODO.md#pixie-todo-008--spatial-grid-and-anchor-2-flocks)

- `PIXIE-TASK-036` — **Status:** open — `SpatialGrid` in the `runtime` module: a uniform grid rebuilt in `PreUpdate`, with radius queries; tests against a brute-force search over random points.
- `PIXIE-TASK-037` — **Status:** open — `flocks` sample: separation, alignment, and cohesion over grid neighbours, plus the mouse; each boid writes only its own acceleration; keys tune the weights; its `README.md`.
- `PIXIE-TASK-038` — **Status:** open — A test that shuffles the Behaviour update order and gets the same step result.
- `PIXIE-TASK-039` — **Status:** open — Verification: 5,000 flocking boids at 60 updates per second, measured and recorded.

## `PIXIE-TODO-009` — Particles and anchor 3: attractors

**Parent:** [PIXIE-TODO-009](TODO.md#pixie-todo-009--particles-and-anchor-3-attractors)

- `PIXIE-TASK-040` — **Status:** open — `ParticleEmitter`: a contiguous pool with emission rate, lifetime, and capacity, updated by an engine system.
- `PIXIE-TASK-041` — **Status:** open — Batch affectors that receive the whole pool; `Attractor` entities and the affector that pulls particles toward them.
- `PIXIE-TASK-042` — **Status:** open — Short-range particle-to-particle interaction through a spatial grid.
- `PIXIE-TASK-043` — **Status:** open — Particles drawn as instanced circles straight from the pool.
- `PIXIE-TASK-044` — **Status:** open — `particles` sample: place and drag attractors with the mouse, switch between 1, 10, 100, and thousands of particles; its `README.md`.
- `PIXIE-TASK-045` — **Status:** open — Verification: 100,000 particles against up to 16 attractors, and 10,000 with interaction, at 60 updates per second, measured and recorded.

## `PIXIE-TODO-010` — Engine side for editor hosts: host loop, render target, reflection, and `capi`

**Parent:** [PIXIE-TODO-010](TODO.md#pixie-todo-010--engine-side-for-editor-hosts-host-loop-render-target-reflection-and-capi)

- `PIXIE-TASK-046` — **Status:** open — A host-driven loop: fixed steps and rendering called from outside, which `pixie::Run` also uses; input fed from host events.
- `PIXIE-TASK-047` — **Status:** open — Offscreen render target: a framebuffer with a color texture, resized on demand, whose texture ID a host displays.
- `PIXIE-TASK-048` — **Status:** open — Field descriptors (name, type, offset) registered per component, for the built-in and the anchors' components.
- `PIXIE-TASK-049` — **Status:** open — Each anchor's game code as a static library, linked into its player executable and its `capi` library, with a project entry point that builds its World.
- `PIXIE-TASK-050` — **Status:** open — Create the `capi` module, its library, and `docs/modules/capi/`: the C header, result codes, World lifetime, step, render to texture, input, entity and component enumeration, and field get and set.
- `PIXIE-TASK-051` — **Status:** open — Verification: a test program that includes only the C header drives each anchor in a hidden window; the players run as before.

## `PIXIE-TODO-011` — C# ImGui editor over the anchors

**Parent:** [PIXIE-TODO-011](TODO.md#pixie-todo-011--c-imgui-editor-over-the-anchors)

- `PIXIE-TASK-052` — **Status:** open — `editor/` project and solution: `PixieInspector.ImGui`, the GLFW and OpenGL3 backends of Hexa.NET.ImGui, an OpenGL 4.5 core window, and docking.
- `PIXIE-TASK-053` — **Status:** open — P/Invoke bindings over `capi` with source-generated `LibraryImport`, loading the library of the chosen anchor.
- `PIXIE-TASK-054` — **Status:** open — Viewport: the World's texture in a docked window with `ImGui.Image`, and the mouse forwarded in viewport coordinates.
- `PIXIE-TASK-055` — **Status:** open — Entity list and inspector: hand-written proxies whose properties call into `capi`, bound by PixieInspector.
- `PIXIE-TASK-056` — **Status:** open — Pause, resume, single step, and reset.
- `PIXIE-TASK-057` — **Status:** open — Its `README.md`; verification with each of the three anchors.
