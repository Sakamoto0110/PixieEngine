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

**Direction decision date:** 2026-10-14

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

### Consumer anchors

Develop the engine through concrete consumers. The initial anchors are boids
steered by the user, flocks, and particle systems with attractors
([PIXIE-PROPOSAL-004](docs/proposals/scene-model-and-anchors.md)).
[LogicSandbox](https://github.com/Sakamoto0110/LogicSandbox) is a separate
consumer project that exercises interactive circuit construction, saves,
templates, and nested chips
([PIXIE-PROPOSAL-009](docs/proposals/logic-sandbox.md)).

The engine supplies general capabilities usable by players, tools, and tests.
The editor consumes those capabilities through the C API. The accepted
dependency and object-model constraints remain in [Active guardrails](#active-guardrails).

### Boundaries and navigation

The decided module boundaries are [runtime](docs/modules/runtime/README.md)
and [capi](docs/modules/capi/README.md). The proposed `platform` and
`render` boundaries remain undecided; their code stays in the provisional
engine library until their boundaries are accepted
([PIXIE-PROPOSAL-001](docs/proposals/engine-module-boundaries.md)).
The anchor direction supersedes the vertical slice of
[PIXIE-PROPOSAL-003](docs/proposals/vertical-slice.md).

Current technical contracts belong to the module references and public
headers, as routed by the [documentation index](docs/README.md). Completed
implementation blocks belong to the [completed-work pointers](TODO.md#completed)
and their archive records. The previous chronological account is preserved
in the [phase 1 sanitation record](docs/archive/documentation-sanitation-phase1-2026-10-18.md#roadmap-chronology-at-the-reference-baseline).

### Next work and deferred investigation

[TODO.md](TODO.md#current-promoted-work) owns the executable work queue and
its gates. This roadmap does not select or authorize a new implementation
block. The [proposal index](docs/proposals/README.md) distinguishes ideas from
recorded promotion outcomes; an idea still needs the owner's promotion.
In particular, shared-engine/export work
([PIXIE-PROPOSAL-010](docs/proposals/shared-engine-and-export.md)) and building
a player game once ([PIXIE-PROPOSAL-012](docs/proposals/player-builds-once.md))
remain proposals.

Investigating particle systems in blank projects remains deferred until after
the logic sandbox work, and is not promoted. The owner's observation and an
unconfirmed source lead are preserved in the
[dated record](docs/archive/particles-in-blank-projects-postponed-2026-10-16.md).
That record does not establish a reproduced defect or authorize a fix.

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
- **Executables do not need the Visual C++ Redistributable.** MSVC builds link
  the C++ runtime statically into every target. Accepted 2026-10-11.
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

None accepted beyond "What PixieEngine is not". Physics, audio, scripting, and
3D are absent from the first cut by proposal only
([PIXIE-PROPOSAL-001](docs/proposals/engine-module-boundaries.md)).
