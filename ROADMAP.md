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

Decided on 2026-10-13
([decision record](docs/archive/object-model-and-editor-decisions-2026-10-13.md)):
three anchor scenarios drive the first cut, each with user interaction and an
editor: boids steered by the user, flocks, and particle systems with
attractors ([PIXIE-PROPOSAL-004](docs/proposals/scene-model-and-anchors.md)).
The work is scheduled in [`TODO.md`](TODO.md) as blocks, simple and
foundational work first, each broken into tasks in
[`TASKLIST.md`](TASKLIST.md):

1. Dependencies: PixieLib, GLM, and EnTT vendored (PIXIE-TODO-003).
2. A documentation verifier (PIXIE-TODO-004).
3. Input, a 2D camera, and instanced shapes (PIXIE-TODO-005).
4. The `runtime` module: World, entities, Behaviours, and update phases
   (PIXIE-TODO-006).
5. Anchor 1, boids (PIXIE-TODO-007).
6. A spatial grid and anchor 2, flocks (PIXIE-TODO-008).
7. Particles and anchor 3, attractors (PIXIE-TODO-009).
8. The engine side for editor hosts: host loop, render target, reflection, and
   `capi` (PIXIE-TODO-010).
9. The C# ImGui editor over the three anchors (PIXIE-TODO-011).

All nine blocks were completed on 2026-10-13: the three anchors run as
players and in the editor (see [`TODO.md`](TODO.md#completed) and the
[archive](docs/archive/README.md)). The owner's review of them, the same day,
added five more blocks
([decision record](docs/archive/editor-review-decisions-2026-10-13.md)):

10. A single source of field metadata in the native registry (PIXIE-TODO-012).
11. Scenario values, scene rebuild, and time control (PIXIE-TODO-013).
12. Attractor size and death on contact (PIXIE-TODO-014).
13. Particle systems and spawners apart (PIXIE-TODO-015).
14. Selection and editing in the viewport (PIXIE-TODO-016).
15. Spawners placed by hand in the particles game (PIXIE-TODO-017), asked for
    by the owner after the review.

Blocks 10 to 15 were completed on 2026-10-13. On 2026-10-14 the owner chose
the next phase
([decision record](docs/archive/projects-and-scenes-decisions-2026-10-14.md)):
games as project folders created and opened from the editor, together with
scene serialization ([PIXIE-PROPOSAL-005](docs/proposals/projects-and-scenes.md)):

16. Saving and loading scenes (PIXIE-TODO-018).
17. Projects, edit mode, and play mode in the engine (PIXIE-TODO-019).
18. New and open projects in the editor (PIXIE-TODO-020).
19. Code in a project (PIXIE-TODO-021).
20. Samples as projects (PIXIE-TODO-022).
21. A centered start screen, the editor's icon, a viewport menu, and view-to-world conversion
    (PIXIE-TODO-023), asked for by the owner after seeing the editor.

Blocks 16 to 21 were completed on 2026-10-14: scene entities are saved and loaded, the engine opens
project folders and edits and plays their scenes, the editor makes and opens projects, a project can
have code of its own, which the editor builds against its engine, and the three anchor samples are
projects of that kind.

Next, the owner chose authoring in the viewport, before sprites and exporting a game
([PIXIE-PROPOSAL-006](docs/proposals/authoring-in-the-viewport.md),
[decision record](docs/archive/authoring-decisions-2026-10-14.md)):

22. Entity snapshots, scene-ID lookup, and names in the engine (PIXIE-TODO-024).
23. Moving, undo and redo, duplicating, deleting, and renaming in the editor (PIXIE-TODO-025).

Blocks 22 and 23 were completed on 2026-10-15: a scene is built in the editor with the mouse, and
every change to it can be undone. The continuous integration followed (PIXIE-TODO-026).

Next, the owner chose sprites ([PIXIE-PROPOSAL-007](docs/proposals/sprites.md),
[decision record](docs/archive/sprites-decisions-2026-10-15.md)):

24. Images, sprites, and text fields in the engine (PIXIE-TODO-027).
25. Sprites in the editor, and a sprites sample (PIXIE-TODO-028).

Blocks 24 and 25 were completed on 2026-10-15: a scene draws PNG images, placed in the editor by
dropping them on the viewport, and a project without code plays in the engine's generic player.
Next, the owner proposed a first game on the engine, a sandbox of logic circuits
([decision record](docs/archive/logic-sandbox-decisions-2026-10-15.md)), recorded as three
unpromoted proposals: what the engine needs for it
([PIXIE-PROPOSAL-008](docs/proposals/interactive-2d-games.md)), the game itself
([PIXIE-PROPOSAL-009](docs/proposals/logic-sandbox.md)), and one shared engine library with
exporting a game ([PIXIE-PROPOSAL-010](docs/proposals/shared-engine-and-export.md)). The owner
promoted the engine's part first:

26. Paths, list fields, and points of a sprite (PIXIE-TODO-029).
27. A screen layer, text, player saves, and fragment metadata (PIXIE-TODO-030).

Block 26 was completed on 2026-10-15: the engine draws lines with rounded corners and a moving fill,
and the editor edits lists of points. Block 27 was completed the same day: a game draws its interface
and text on a screen layer, keeps its own files in a user data folder, and saves sets of entities
with metadata. PIXIE-PROPOSAL-008 is carried out; the logic sandbox
([PIXIE-PROPOSAL-009](docs/proposals/logic-sandbox.md)) waits on the owner's promotion. Before it, the
owner promoted text in the World as data
([PIXIE-PROPOSAL-011](docs/proposals/scene-text.md); screen text stays code):

28. Text in the scene (PIXIE-TODO-031).

The anchor blocks supersede the vertical slice
of [PIXIE-PROPOSAL-003](docs/proposals/vertical-slice.md), with anchor 1 as the
first scenario. The `platform` and `render` boundaries stay undecided; their
code grows in the provisional engine library until they are.

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
