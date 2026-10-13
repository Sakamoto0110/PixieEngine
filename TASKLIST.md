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
their parents. Completed breakdowns are in their archive records, as the one of
PIXIE-TODO-002 is in its
[archive record](docs/archive/todo-002-main-loop-hello-gl-2026-10-11.md).

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
