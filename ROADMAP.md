# PixieEngine Roadmap

**Document ID:** GLOBAL-ROADMAP

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** what the engine is and is not, direction, guardrails, and freezes

**Surface:** roadmap

**Boundary:** global

**Authority role:** roadmap

**Mutation:** authored

**Indexing:** include

**Direction decision date:** 2026-10-28

## Purpose

This document owns what PixieEngine is, what it is not, where it is heading,
and which boundaries must be preserved. It does not authorize implementation;
[`TODO.md`](TODO.md) is the only scheduler. Unpromoted ideas live in
[`docs/proposals/`](docs/proposals/README.md), and the rationale behind the
decisions below is in the [archive](docs/archive/README.md).

## What PixieEngine is

- A game engine whose first target is **2D**.
- A **C++ runtime** rendering through **OpenGL 4.5 core**.
- A **standalone player** executable as the deployable form of a game.
- A **C# editor** that uses PixieInspector to inspect and edit engine state,
  with hosts on more than one platform.
- A consumer of **PixieLib**, which supplies the basic primitives and forwards
  to math libraries.

## What PixieEngine is not

- Not a 3D engine in its first cut. 3D is not ruled out, but 2D comes first.
- Not an OpenGL 4.1 or older renderer.
- Not a macOS engine: macOS is not a target platform, for the engine, the
  player, or the editor.
- Not an owner of its own primitive or math types. These belong to PixieLib.

## Current direction

PixieEngine develops reusable 2D runtime and authoring capabilities from concrete
consumer needs. Boids, flocks and particles remain small engine/editor anchors;
LogicSandbox is the circuit consumer. The side-scrolling circuit puzzle game
adds three quality priorities: atmosphere, lighting and fluid movement.

Circuit simulation and puzzle rules remain game-owned. Engine capabilities
should be proven in a representative scene before broadening their scope.
Confirmed consumer needs and proposed acceptance scenarios are separated in the
[requirements analysis](docs/circuit-puzzle-requirements.md).

## Planning horizons

### Promoted work

[TODO.md](TODO.md#current-promoted-work) owns the current execution queue,
scope and gates. [TASKLIST.md](TASKLIST.md) owns task decomposition.
Neither a roadmap direction nor a requirement schedules implementation.
Completed work is routed through [TODO.md](TODO.md#completed) and the
[archive](docs/archive/README.md), rather than repeated here.

### Accepted direction: expansion required

The owner retained colliders, physics and lighting as roadmap directions and
confirmed the game's atmosphere/movement priorities. Detailed architecture,
dependencies, budgets and completion gates remain to be expanded when scoped
work starts. The [direction record](docs/archive/roadmap-reconciliation-2026-10-22.md#owner-dispositions-after-review)
preserves the decisions and the earlier design suggestions.

| Direction | Intended result | Decisions to expand |
|---|---|---|
| Colliders, triggers and queries | Reusable geometry queries and predictable interaction events | Shapes/transforms, filtering, event order, backend and edge-case behavior |
| Basic 2D physics | Bodies, gravity and coherent scene lifecycle | Library/boundary, units, ECS synchronization and verification scenarios |
| 2D lighting | Scene lighting and occlusion with readable gameplay and panels | Compositing, shadows, occluders, editor preview and quality budgets |
| Fluid movement | Responsive traversal and reliable platform interactions | Character rules, controller design and representative movement tests |
| Atmosphere and presentation | Coherent world composition, sound and camera behavior | Art/sound direction and the smallest reusable audio, camera and animation facilities |
| Dynamic particles | A spawner marks its particles dynamic; static and dynamic particles share one pool, and only the dynamic ones feel gravity and colliders (each particle's look, set by its spawner, was delivered by PIXIE-TODO-047) | Per-kind payload, collider responses (bounce, stop, die), and how the dynamic part's cost is reported |

**Particle data design note (owner discussion, 2026-10-26).** A particle is born complete: a
`ParticleSpawner` creates almost all of them, and `ParticleSystem::Emit` (code and tools) shares the
same field-filling function; whoever creates a particle sets its kind, which does not change during
its life. After birth the system owns it, and the step and affectors only evolve it (age, velocity,
death). PIXIE-TODO-047 delivered the 40-byte layout: RGBA8 color (4), size as a 16-bit half float
(2), a flags byte (1), and one spare byte. The flags byte holds the dynamic bit and the
shape (2 bits for 4 shapes, up to 7 bits for 128), so with 4 shapes 13 bits remain free; 48 bytes
would be needed only for data that does not fit, such as a float size, rotation, or a second color.
Data for mutually exclusive kinds of particle shares a fixed payload slot read through typed
accessors (`std::bit_cast`), not a C++ `union`, whose inactive-member reads are undefined; the
shader reads the slot as integers and decodes it by the flags. Data that only a few particles use,
or that only the CPU reads (a dynamic particle's contacts or bounces), lives in a side table kept
in step with the pool's removals, so its cost follows the particles that use it. Collision runs
only for dynamic particles, so its cost scales with their number, not the pool's; it is reported
separately in telemetry rather than capped.

The owner requested detailed body-dynamics planning before proceeding to lighting
and representative test rooms. The [physics requirements and proposed design](docs/rigid-body-physics-requirements.md)
expand that sequence, including character coupling and staged acceptance. This
planning expansion does not itself promote implementation work.

These directions do not settle a library, public API, rendering technique or
execution order. Collision queries support movement; lighting and movement
quality should be explored early in the representative scene. The consumer's
proposed slice and performance reference remain proposals in the requirements
analysis, not accepted engine budgets.

### Conditional investigations

Each candidate needs evidence of useful reuse and a bounded scope before
promotion. None is an automatic follow-up to completed telemetry work.

| Candidate | Entry condition |
|---|---|
| Shared path geometry and accelerated picking | Loaded-consumer profiling justifies the cost; preserve drawing, interpolation and selection semantics |
| Presentation membership and shared visible queries | Hidden-entity traversal is material; visibility changes preserve simulation timing and state |
| Active-work queues and reusable scratch storage | A second consumer demonstrates reusable semantics beyond circuit scheduling |
| Shared serialized definitions | A second consumer establishes identity, overrides, mutable-instance isolation and compatibility needs |
| Single-build player startup | Owner disposition of [PIXIE-PROPOSAL-012](docs/proposals/player-builds-once.md), preserving editor Play/Stop behavior |

### Deferred investigation

- **Particle authoring through the editor:** reported inability to recreate the
  sample effect in a blank project. Review is deferred to 2026-10-23; root cause
  and a fix remain unconfirmed. The [runtime finding](docs/modules/runtime/FINDINGS.md#runtime-finding-001-particle-authoring-in-a-blank-project)
  owns the report, evidence limits and reproduction questions.

### Boundary decisions

The approved native split keeps Core, Platform, Render and App as separate
ownership/dependency boundaries. Platform and Render depend on Core and never
on each other; App composes them. The split implementation is the baseline for
the distribution work, not evidence that all of its documentation has been reconciled.

On 2026-10-28 the owner promoted [PIXIE-PROPOSAL-010](docs/proposals/shared-engine-and-export.md):
one shared native engine, directly linked game players, development-only game
modules and allowlisted folder exports. Packaging those boundaries into one
DLL does not remove their dependency directions. The owner also included a
separate local MCP development channel for Engine/Inspector operations. Its
operations and dependencies must be absent from game exports, publish and
remote-release artifacts. The owner included proposals 013/014 for distinct
World and Inspector/history paths, repeatable capture and approved snapshot
qualification; this does not approve reference images or enable channel save.
Execution, the EnTT-first gate and qualification are
owned by [PIXIE-TODO-051](TODO.md#pixie-todo-051--shared-engine-game-export-and-local-mcp-development-channel).

## Active guardrails

- **2D first.** Accepted 2026-10-09.
- **OpenGL 4.5 core.** Accepted 2026-10-09. Rationale and cost in the
  [inception record](docs/archive/inception-2026-10-09.md#rationale-opengl-45-core).
- **The engine never depends on the editor.** Dependencies point one way: the
  editor depends on the engine, never the reverse. What the editor needs on
  the native side (picking, debug drawing, scene saving, play mode) enters the
  engine as a general feature usable by the player, tools, and tests, never as
  an editor-specific one. The player building and running with nothing from
  the editor is the proof. Accepted 2026-10-10.
- **C++20, without modules.** Accepted 2026-10-10. Rationale in the
  [inception record](docs/archive/inception-2026-10-09.md#recommendations-not-yet-accepted).
  Moving to C++26 is to be evaluated later; it is an intention, not a plan.
- **CMake builds the native side.** Accepted 2026-10-10. The C# editor keeps
  its own solution.
- **Object model: an EC-style API over ECS storage.** Game logic is written as
  C++ `Behaviour` classes with `Update()`; components are data-only structs in
  contiguous storage, iterated by engine systems. There is one runtime, and no
  object model is chosen at boot. A step runs in phases: `PreUpdate` (engine),
  `Update` (Behaviours, which write only their own state), and `PostUpdate`
  (engine). Accepted 2026-10-13.
- **The editor is a C# ImGui application first.** It uses Hexa.NET.ImGui
  through PixieInspector.ImGui and shows the engine's output as a texture
  rendered offscreen in a shared OpenGL 4.5 core context. Accepted 2026-10-13.
- **The editor reaches the engine through a C API.** A shared library with a
  C-only interface (`capi`), consumed through P/Invoke; the native World is the
  single source of truth, and C# holds handles only; its inspectors are built
  from the engine's field descriptors
  ([PIXIE-PROPOSAL-002](docs/proposals/editor-native-bridge.md)). Accepted
  2026-10-13.
- **The native registry is the single source of field metadata.** Names,
  kinds, and inspector metadata (labels, tooltips, ranges, scrub, units) are
  declared once, in `ComponentTypes`; the C API exports them and tools build
  on them, with no per-component class on the tool side. C++26 reflection may
  generate the same descriptors once MSVC supports it; until then the project
  stays on C++20. Accepted 2026-10-13.
- **The scene is the document; the World is the instance.** A scene file
  holds the entities that belong to the scene, written through the field
  descriptors; the World is built from it. In the editor, edits change the
  World, Save writes it back, and Stop builds the World again from the scene
  as it was at Play. Accepted 2026-10-14.
- **Projects are portable folders, created and opened from the editor.** A
  project holds only paths relative to its folder and never records where the
  engine is; the editor that opens it supplies its own engine. Making or
  opening a project needs no command-line tool, `PATH` change, or environment
  variable, and a project without code needs no compiler. The editor creates
  no version-control files: versioning belongs to the user. Accepted
  2026-10-14.
- **Time is passed explicitly.** `Update` and `Render` receive a `FrameTime`
  struct (the step or frame delta, the render interpolation alpha, simulated
  elapsed time, and the call index); there is no global timestep. Time belongs to a World, not to the
  process. Accepted 2026-10-10.
- **Players install no Visual C++ runtime prerequisite.** The 2026-10-28
  shared-engine decision replaces the static-runtime mechanism accepted on
  2026-10-11: native targets share /MD (/MDd in Debug), and game exports carry
  the required Visual C++ runtime DLLs app-locally. This is an accepted target;
  its implementation and deployment proof remain gated by PIXIE-TODO-051.
- **GLEW is the GL loader to start with.** Accepted 2026-10-09; glad 2 remains
  a candidate in [`DEPENDENCIES.md`](DEPENDENCIES.md).
- **PixieLib owns primitives and math forwarding.** Accepted 2026-10-09.
- **macOS is not a target.** No choice is weighed against macOS support.
  Accepted 2026-10-12.
- **Editor portability.** The editor is gaining hosts on more than one
  platform; engine-side choices must not assume a Windows-only editor.
  Accepted 2026-10-09.
- **Documentation in English** under the
  [documentation policy](docs/governance/documentation-policy.md). Accepted
  2026-10-09.
- **Repository location.** The repository stays in its current directory until
  the agent workplace tooling is reliable enough to host it. Accepted
  2026-10-09.

## Freezes

None.

## Explicit non-goals

None accepted beyond "What PixieEngine is not". The first-cut exclusions in
[PIXIE-PROPOSAL-001](docs/proposals/engine-module-boundaries.md) are proposal
history, not a blanket prohibition on the physics and atmosphere directions
above. Scripting and any later 3D work still require their own direction/scope
decisions.
