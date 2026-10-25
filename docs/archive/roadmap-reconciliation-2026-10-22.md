# Roadmap Reconciliation and Preserved Planning Inputs

**Document ID:** ARCHIVE-ROADMAP-RECONCILIATION-20261022

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** separate current direction from chronology and preserve unresolved planning inputs

**Surface:** archive-record

**Boundary:** global

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-078

**Type:** roadmap

**Origin:** owner request to reconcile planning documents

**Author:** codex

**Reference date:** 2026-10-22

**Commits:** `a18fe6ba9ecadb94b148f6cce7d23019bd277c70`, which adds this record

**Original path:** ROADMAP.md; TODO.md; ROADMAP-PROMOTIONS-PENDING.md; CLAUDE-SESSION-REPORT.md

**Current state:** [Roadmap](../../ROADMAP.md), [scheduler](../../TODO.md), [policy](../governance/documentation-policy.md)

## Reconciliation performed

The owner requested reconciliation and allowed an unversioned staging folder for
edge cases to review individually. The roadmap's completed-block narrative was
replaced by current direction, next-capability gates and deferred questions.
All 40 completion pointers were retained in TODO-ID order. No implementation,
backend adoption, policy migration, task renumbering or archive deletion was
authorized by this cleanup. Existing dirty implementation work was preserved.

The two ungoverned Claude inputs are imported below as historical source text,
not as executable instructions or policy. Their root files were removed only
after preserving their complete text here and byte-exact local review copies.
Fenced snapshots deliberately retain original links/claims as text; they are
not current navigation or verified technical specifications. Existing archive
records 001-077 were left untouched, apart from adding this router row.

The later owner direction is a side-scrolling world with editable circuit
panels, with excellent atmosphere, lighting and fluid movement; the active
sandbox replaces the older one. These are direction, not a completed game or
authorization to implement every supporting engine feature. E01 telemetry is
already recorded in 040; other extraction candidates remain unpromoted.

## Unresolved dispositions

1. Grouped TODOs and hierarchical display labels versus stable existing IDs:
   preserve IDs; review an explicit policy/schema migration separately.
2. Decision-only archive records versus the imported alternative: preserve
   records and current policy until a new owner decision defines the change.
3. Physics/lighting design: direction is visible, but backend, API, event order,
   units, shadow technique and performance guarantees remain design decisions.
4. Active sandbox replacement: preserve the old checkout/history; repository
   integration and save migration are separate work.
5. Evidence durability: source snapshots do not restore missing screenshots.
   Uncommitted documents and ignored validation files are not remote archives.
6. Later engine/game handoffs: keep E02-E05 conditional and consumer requirements
   distinct from scheduled engine work; preserve their historical baselines.
7. Particles in blank projects: keep the postponed observation unconfirmed
   until reproduced, rather than silently treating the lead as a defect.

These dispositions are durable here; the machine-local review queue merely
provides one working file per case and is not authoritative documentation.

## Verification boundary

VAW binding resolved `bound_scoped` for route `documentation`, profile
`pixieengine` revision 1, VAW baseline `c9d81ba1fe538ddaaaa44f2ca48e1dc9d3dcfcb1`.
No builds, native runs, library research or gameplay validation were needed or
performed. Structural/link checks, completion-pointer coverage, snapshot hashes
and preservation of existing archive bodies/source files are checked separately.
No commit, push or remote backup is implied.

## Source hashes

SHA-256 values describe source bytes before this reconciliation, including their
original line endings. Fenced text below normalizes line endings for readability.

| Original path | SHA-256 |
|---|---|
| `ROADMAP.md` | `a54b4f7ec7d2e10ac210b97613084d2398412f169a20bb2826213b8150429f43` |
| `TODO.md` | `f0bbe0dd38331161ad7a8375bc1e81dfdb7b9f10c13d1039c4a9d696f9a6ffe1` |
| `ROADMAP-PROMOTIONS-PENDING.md` | `48a80859917788b027c5d5d6407e85c455b0061c50031de00c5235f075162259` |
| `CLAUDE-SESSION-REPORT.md` | `0ed31d37ac73d09d4cfb91e92533821d5cde28cd39941e7f8e4af8d019b8d1a8` |

## Previous roadmap

```markdown
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

Block 28 was completed on 2026-10-15: text is part of a scene, saved, picked, and drawn among the
sprites, and the editor treats fonts as assets. Then the owner promoted the logic sandbox
([PIXIE-PROPOSAL-009](docs/proposals/logic-sandbox.md)), the engine's first game, in a project folder
of its own:

29. The logic sandbox, stage 1: light the lamp (PIXIE-TODO-032).
30. The logic sandbox, stage 1.5: saves and templates (PIXIE-TODO-033).

Block 29 was completed on 2026-10-15: on the engine as it is, a player places gates, wires them
through bends, and lights a lamp, in a project of its own with its own tests. After playing it, the
owner put fixes to stage 1 before block 30
([promotion record](docs/archive/logic-sandbox-fixes-promotion-2026-10-16.md)):

31. The logic sandbox, stage 1 fixes: selecting and removing wires (PIXIE-TODO-034), before block 30.

Block 31 was completed on 2026-10-16: wires and their bends are selected and removed, and a right
click on a wired input removes its wire. Then the owner asked for a box selection and a toolbar,
before the templates of block 30
([promotion record](docs/archive/logic-sandbox-toolbar-promotion-2026-10-16.md)):

32. The logic sandbox: box selection and a toolbar (PIXIE-TODO-035).

Block 32 was completed on 2026-10-16: a box selects pieces and their wires, the selection is copied
and pasted with new IDs, and a toolbar deletes, copies, pastes, saves, and loads.

Block 30 was completed on 2026-10-16: the circuit is saved and loaded, and parts of it become
templates, placed from the hotbar's Templates page. PIXIE-PROPOSAL-009 is carried out.

Then the owner promoted a new anchor for the game
([promotion record](docs/archive/logic-sandbox-adder-promotion-2026-10-16.md)):

33. The logic sandbox, stage 2: chips and an 8-bit adder (PIXIE-TODO-036).

Block 33 was completed on 2026-10-16: templates come back as chips, boxes with labelled pins that
open by zoom down to their gates, and an 8-bit ripple adder of full-adder chips of half-adder chips
adds in the game's start scene.

Then the owner asked for drawing to skip what is off screen, as the zoom grows
([promotion record](docs/archive/view-culling-promotion-2026-10-16.md)):

34. Drawing only what is on screen (PIXIE-TODO-037).

Block 34 was completed on 2026-10-16: a session submits only the sprites, texts, paths, and shapes
that may show on screen.

Then the logic sandbox's calculator, thousands of chips each read as a small set of entities, took
minutes to load, nearly all of it in `ReadEntities`, and the owner asked for it to load fast
([promotion record](docs/archive/scene-index-promotion-2026-10-17.md)):

35. Reading many small sets of entities into a large World (PIXIE-TODO-038).

Block 35 was completed on 2026-10-17: the World keeps its scene entities by scene ID, so reading a set
of entities costs what the set holds, not the World's size.

On 2026-10-22, generic CPU telemetry and its active LogicSandbox integration (PIXIE-TODO-040)
completed in the uncommitted working tree: per-World metrics, inclusive scopes, headless export,
named systems and native display/control. See the [completion record](docs/archive/todo-040-telemetry-2026-10-22.md).

Later, after the logic sandbox, and not promoted yet: the owner observed on 2026-10-16 that a
particle system made in a blank project does not seem to work, and asked for it to be investigated
toward the end ([decision record](docs/archive/particles-in-blank-projects-postponed-2026-10-16.md)).
An unconfirmed lead: only the particles sample adds the particle step to its World
(`AddParticleStep`, in `samples/particles/src/game/ParticlesScene.cpp`), and a project without code
runs on the generic library, which does not; if that holds, its particle systems never step.

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
```

## Previous completion scheduler

```markdown
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

**Last promotion decision:** 2026-10-22

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

None. Generic telemetry (PIXIE-TODO-040) completed on 2026-10-22; see below.

The native-player debug controls (PIXIE-TODO-039), reading many small sets of entities into a large World
(PIXIE-TODO-038, [promotion record](docs/archive/scene-index-promotion-2026-10-17.md)), drawing only
what is on screen (PIXIE-TODO-037), and the logic sandbox's (PIXIE-TODO-032 to PIXIE-TODO-036) are
completed; see below. The game lives in its own repository,
[`Sakamoto0110/LogicSandbox`](https://github.com/Sakamoto0110/LogicSandbox). What comes next waits
for the owner's promotion.

## Completed

`PIXIE-TODO-040` (generic CPU telemetry and active LogicSandbox integration) completed on 2026-10-22
in the uncommitted working tree. Scope, task snapshot, verification and limits are in the
[completion record](docs/archive/todo-040-telemetry-2026-10-22.md).

`PIXIE-TODO-039` (native-player F3 rates and F3+C command bar) completed on 2026-10-20 in the
uncommitted working tree. Scope, implementation, validation, and limitations are preserved in
the [archive record](docs/archive/todo-039-player-debug-2026-10-20.md).

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

`PIXIE-TODO-005` (input, 2D camera, and instanced shapes) completed on
2026-10-13. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-005-input-camera-shapes-2026-10-13.md).

`PIXIE-TODO-006` (runtime module: World, entities, Behaviours, and phases)
completed on 2026-10-13. Its entry, breakdown, and verification are preserved
in the [archive record](docs/archive/todo-006-runtime-module-2026-10-13.md).

`PIXIE-TODO-007` (anchor 1: boids steered by the user) completed on
2026-10-13. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-007-anchor-1-boids-2026-10-13.md).

`PIXIE-TODO-008` (spatial grid and anchor 2: flocks) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-008-spatial-grid-flocks-2026-10-13.md).

`PIXIE-TODO-009` (particles and anchor 3: attractors) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-009-particles-attractors-2026-10-13.md).

`PIXIE-TODO-010` (engine side for editor hosts: host loop, render target,
reflection, and `capi`) completed on 2026-10-13. Its entry, breakdown, and
verification are preserved in the
[archive record](docs/archive/todo-010-host-loop-reflection-capi-2026-10-13.md).

`PIXIE-TODO-011` (C# ImGui editor over the anchors) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-011-imgui-editor-2026-10-13.md).

`PIXIE-TODO-012` (single source of field metadata) completed on 2026-10-13. Its
entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-012-single-source-metadata-2026-10-13.md).

`PIXIE-TODO-013` (scenario values, scene rebuild, and time control) completed on
2026-10-13. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-013-scenario-time-control-2026-10-13.md).

`PIXIE-TODO-014` (attractor size and death on contact) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-014-attractor-size-2026-10-13.md).

`PIXIE-TODO-015` (particle systems and spawners apart) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-015-particle-spawners-2026-10-13.md).

`PIXIE-TODO-017` (spawners placed by hand in the particles game) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-017-spawners-by-hand-2026-10-13.md).

`PIXIE-TODO-016` (selection and editing in the viewport) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-016-viewport-selection-2026-10-13.md).

`PIXIE-TODO-018` (save and load scenes) completed on 2026-10-14. Its entry,
breakdown, and verification are preserved in the
[archive record](docs/archive/todo-018-scenes-2026-10-14.md).

`PIXIE-TODO-019` (projects, edit mode, and play mode in the engine) completed
on 2026-10-14. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-019-projects-2026-10-14.md).

`PIXIE-TODO-020` (new and open projects in the editor) completed on
2026-10-14. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-020-editor-projects-2026-10-14.md).

`PIXIE-TODO-023` (editor polish, the editor's icon, and a viewport menu) was
promoted and completed on 2026-10-14. Its entry, breakdown, and verification
are preserved in the
[archive record](docs/archive/todo-023-editor-polish-2026-10-14.md).

`PIXIE-TODO-021` (code in a project) completed on 2026-10-14. Its entry,
breakdown, and verification are preserved in the
[archive record](docs/archive/todo-021-project-code-2026-10-14.md).

`PIXIE-TODO-022` (samples as projects) completed on 2026-10-14. Its entry,
breakdown, and verification are preserved in the
[archive record](docs/archive/todo-022-samples-projects-2026-10-14.md).

`PIXIE-TODO-024` (entity snapshots, scene-ID lookup, and names in the engine)
completed on 2026-10-14. Its entry, breakdown, and verification are preserved
in the [archive record](docs/archive/todo-024-snapshots-2026-10-14.md).

`PIXIE-TODO-025` (moving, undo and redo, duplicating, deleting, and renaming in
the editor) completed on 2026-10-15. Its entry, breakdown, and verification
are preserved in the
[archive record](docs/archive/todo-025-authoring-2026-10-15.md).

`PIXIE-TODO-026` (continuous integration) was promoted and completed on
2026-10-15. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-026-ci-2026-10-15.md).

`PIXIE-TODO-027` (images, sprites, and text fields in the engine) completed on
2026-10-15. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-027-sprites-engine-2026-10-15.md).

`PIXIE-TODO-028` (sprites in the editor, and a sprites sample) completed on
2026-10-15. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-028-sprites-editor-2026-10-15.md).

`PIXIE-TODO-029` (paths, list fields, and points of a sprite) completed on
2026-10-15. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-029-paths-2026-10-15.md).

`PIXIE-TODO-030` (a screen layer, text, player saves, and fragment metadata)
completed on 2026-10-15. Its entry, breakdown, and verification are preserved in
the [archive record](docs/archive/todo-030-screen-text-2026-10-15.md).

`PIXIE-TODO-031` (text in the scene) completed on 2026-10-15. Its entry,
breakdown, and verification are preserved in the
[archive record](docs/archive/todo-031-scene-text-2026-10-15.md).

`PIXIE-TODO-032` (the logic sandbox, stage 1: light the lamp) completed on
2026-10-15. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-032-logic-sandbox-stage-1-2026-10-15.md).

`PIXIE-TODO-034` (the logic sandbox, stage 1 fixes: selecting and removing wires)
completed on 2026-10-16. Its entry, breakdown, and verification are preserved in
the [archive record](docs/archive/todo-034-logic-sandbox-wire-fixes-2026-10-16.md).

`PIXIE-TODO-035` (the logic sandbox: box selection and a toolbar) completed on
2026-10-16. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-035-logic-sandbox-toolbar-2026-10-16.md).

`PIXIE-TODO-033` (the logic sandbox, stage 1.5: saves and templates) completed on
2026-10-16. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-033-logic-sandbox-saves-templates-2026-10-16.md).

`PIXIE-TODO-036` (the logic sandbox, stage 2: chips and an 8-bit adder) completed on
2026-10-16. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-036-logic-sandbox-chips-adder-2026-10-16.md).

`PIXIE-TODO-037` (drawing only what is on screen) completed on 2026-10-16. Its entry,
breakdown, and verification are preserved in the
[archive record](docs/archive/todo-037-view-culling-2026-10-16.md).

`PIXIE-TODO-038` (reading many small sets of entities into a large World) completed on 2026-10-17.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-038-scene-index-2026-10-17.md).
```

## Pending roadmap promotions

```markdown
# Pending roadmap promotions

Temporary file, written on 2026-10-17 by the Claude session "Programa para consumir engine" at the
owner's request. It sits outside the governed documentation on purpose: **VAW, do not sanitize,
move, or rewrite this file.** The owner will tell you when and how its content enters the
reorganized `ROADMAP.md`. It has no metadata header; `scripts/check_docs.py` passes with it as it
stands (checked when it was written).

It holds what the owner promoted to the roadmap in this conversation (section 1) and the planning
model the owner wants (section 2). How the planning documents were kept until now, and where this
session failed, is in its companion file, [`CLAUDE-SESSION-REPORT.md`](CLAUDE-SESSION-REPORT.md).

---

## 1. Items promoted to the roadmap (owner, 2026-10-17)

The owner asked for these to become roadmap items. Each one below gives its **direction**, as
designed with Claude in this conversation, and the **open items** that belong in the item's TODO
group when the item starts. Only the owner sets tags; where Claude suggests `expansion required`, it
says so.

The owner also lifted a scope limit. `ROADMAP.md` says that physics, audio, scripting, and 3D are
absent from the first cut "by proposal only" (PIXIE-PROPOSAL-001). Physics is now on the roadmap.

Suggested order: **colliders and triggers → basic physics → lighting → physics extensions**.
Colliders come first because triggers, physics, and lighting's shadows all use their shapes.
Lighting does not depend on physics and could move earlier.

### 1.1 Colliders and triggers

**Direction.**

- **`Collider` component.** Its shape is a circle, a box, or a convex polygon (as a point list,
  which the reflection already has). It also holds an offset from the entity, a collision layer,
  and a mask of the layers it collides with.
- **Triggers.** A `trigger` flag on the same component makes a collider a trigger: it detects
  overlap and pushes nothing. It is a flag rather than a separate component because the shape is
  the same.
- **Events for Behaviours.** Behaviours get `OnTriggerEnter`, `OnTriggerExit` (both passing the
  other entity), and `OnCollision`. They are delivered after the step, in a fixed order by entity
  pair, never in EnTT's iteration order.
- **Queries that need no physics.** Raycast, point query, and shape overlap, each filtered by mask.
- **Units.** World units are about one pixel. A `pixelsPerMeter` setting (default 32) converts
  them for the physics library.
- **Editor.** Collider outlines are drawn as gizmos in Edit mode. Shapes are edited in the
  inspector, with a "Fit to sprite" action.

**Open items for its TODO group (decisions, alternatives, costs).**

- **The physics library.** Claude recommends Box2D v3: MIT-licensed, C, SIMD-optimized, and
  deterministic across platforms. It would be vendored and kept private to the runtime, so the only
  surface a game sees is the engine's components, events, and queries.
  - Alternative 1: our own library from scratch.
  - Alternative 2: Chipmunk2D, which is less active.
  - The owner's motive for having our own: **optimization and edge cases**.
  - Recommendation reached in the discussion: forward to Box2D, which keeps its SIMD solver, and
    extend it where needed (see 1.4). A full replacement only if measurements demand one.
- **Mirroring cost.** The ECS and the library each hold a world, and they must be kept in step.
  Mitigation: copy back only the bodies that moved, using Box2D's body move events.
- **A benchmark and edge-case suite**, shared by everything physics-related (see 1.4).

### 1.2 Basic physics

**Direction.**

- **`Rigidbody2D` component** with these fields:
  - its type: dynamic, kinematic, or static;
  - mass;
  - linear and angular damping;
  - a gravity scale;
  - restitution;
  - friction;
  - a switch that fixes its rotation.
- **Existing components.** It reads and writes the existing `Velocity2D`.
- **Gravity.** Gravity is a scene setting, on an environment component. With Y pointing down,
  positive gravity falls.
- **Play and Edit.** Physics runs in Play only, and Stop restores the saved scene, as it does
  today.
- **Out of the first cut:** joints, continuous collision, and sleeping.

**Open items for its TODO group.** It depends on 1.1 and on the library decision there.

### 1.3 Lighting

**Direction.** A light map is rendered offscreen each frame and multiplies the lit scene.

- **Global light.** `AmbientLight` (color and intensity), one per scene. Black ambient light means
  only what is lit shows.
- **Point and spot lights.** A `Light` component is a point light (radius and falloff) or a spot
  light (a direction and a spread, edited with the inspector's existing arc editor). It has a
  color, an intensity, and a "casts shadows" switch.
- **Directional light.** It lights everything evenly, like a sun. Its role is the direction of
  parallel shadows.
- **Shadows.** The first cut has hard shadows only. Each `ShadowCaster` extrudes its edges away
  from the light. Its shape comes from the entity's `Collider` if it has one, else from the
  sprite's rectangle. Soft shadows come later.
- **Unlit drawables.** `Sprite`, `Text`, and `Path` gain a `lit` flag (default true). An unlit one
  shows at full brightness: a lamp, a glow, a label. The screen layer is never lit.
- **Editor.** Light gizmos (a circle, a cone) are drawn when a light is selected. A viewport switch
  compares the lit and unlit scene.
- **Cost.** One pass per visible light. The view culling from PIXIE-TODO-037 applies to lights.

**Open items for its TODO group.**

- **Light map resolution:** full or half.
- **Blending of overlapping lights:** additive, with a clamp.
- **Shadow technique:** geometry extrusion with a stencil, or a 1D shadow map per light.
- **Normal maps:** later.
- **Behaviour in the editor's Edit mode:** whether lighting shows there, or only in Play.

### 1.4 Physics extensions

Claude suggests the tag `expansion required`: the scope of this item is driven by measurements.

**Direction.** These are extensions written by us on top of the library, aimed at the owner's two
motives.

- **A kinematic character controller.** It is pixel-exact, has no contact gap, and is built on
  shape casts: move, hit, slide. A dynamic body used as a platformer character slides on slopes,
  sticks to walls, and jitters on steps; this controller avoids all three.
- **One-way platforms and pass-through rules**, through Box2D's pre-solve callback.
- **Particles against colliders, in batches, by our own code.** Up to 100,000 particles test
  against a copy of the static shapes kept in our `SpatialGrid`, with SIMD of our own where it pays
  off. Particles are not entities and must not become library bodies.
- **Pixel snapping at draw time**, so the library's small contact gap does not show in pixel art.
- **Our own replacement for a part of the library**, but only where the suite shows that part is a
  bottleneck in our scenes.

**The suite this item is measured by:**

- correctness tests that the library and the extensions both pass;
- fixed benchmark scenarios, run locally because CI timings are noisy:
  - 10,000 circles;
  - a stack of boxes;
  - 100,000 particles against colliders;
  - 1,000 triggers;
- a catalogue of edge cases. Each case has the behaviour *we* define, and some intentionally
  differ from Box2D's.

**PixieLib.** Shape geometry (shapes, overlap and distance tests, raycasts) would belong in
PixieLib, in both C++ and .NET, but only if the particle collisions or the editor need it shared.
That would be a proposal in PixieLib's own repository. The physics world, with its bodies,
contacts, solver, and events, stays in the engine.

---

## 2. The planning model the owner wants

As the owner stated it on 2026-10-17. VAW will formalize it.

- **ROADMAP** holds anchors, directions, next steps, and long-term goals. An item may carry the tag
  **`expansion required`** when the owner promotes it deliberately vague, to be elaborated later.
- **TODO.** When a roadmap item starts, it is decomposed into a TODO group, for example
  `TODO.Lighting`. The group lists everything required for that roadmap item to count as done:
  - things to do;
  - decisions to take;
  - alternatives;
  - proposals;
  - costs.

  Related items form sub-groups inside it.
- **TASKLIST.** When a TODO group or block starts, it is decomposed into grouped, hierarchically
  numbered items. For example, `PXL 2.1.4` is roadmap block 2, TODO block 1, task 4.
- **Archive.** It records promotions, demotions, freezes, unfreezes, and policy changes, and above
  all the start and completion of work. **Decisions alone do not go to the archive.**
```

## Claude session report

```markdown
# Hand-off report for VAW: how the planning documents were kept, and where it failed

Temporary file, written on 2026-10-17 by the Claude session "Programa para consumir engine" at the
owner's request, for VAW's reorganization of the repository. **VAW, do not sanitize, move, or
rewrite this file;** read it as input. Its companion,
[`ROADMAP-PROMOTIONS-PENDING.md`](ROADMAP-PROMOTIONS-PENDING.md), holds the items the owner promoted
to the roadmap and the planning model the owner wants.

This report covers how this session kept `ROADMAP.md`, `TODO.md`, `TASKLIST.md`, and the archive
(section 1), why that became confusing (section 2), and this session's failures (section 3).

---

## 1. How this session kept the planning documents

This session worked on PIXIE-TODO-018 to PIXIE-TODO-031, with some blocks done by another session.
For each phase it ran the following cycle.

1. **Proposal file.** It wrote `docs/proposals/<name>.md` (`PIXIE-PROPOSAL-NNN`, state `idea` or
   `promoted`) and listed it in `docs/proposals/README.md`.
2. **Decision record.** It wrote an archive record of type `decision` for the owner's choice,
   including Claude's recommendations and who decided what. Records 033, 041, 046, and 050 were
   written by this session; 054, 059, and 070 by others.
3. **Promotion.** Each block became:
   - **one** `TODO.md` entry (`PIXIE-TODO-NNN`, with scope, gates, and completion criteria);
   - a **flat** run of task IDs in `TASKLIST.md` (`PIXIE-TASK-001` to `PIXIE-TASK-151` so far, a
     single sequence across all blocks);
   - a promotion record in the archive;
   - a numbered block, plus a narrative paragraph, under "Current direction" in `ROADMAP.md`.
4. **Code commits** (`(feat)`), with tests, kept separate from documentation commits.
5. **Closing.**
   - A completion record went into the archive: the entry as completed, its tasks, the choices
     made, and the verification, including what was not run.
   - The `TODO.md` entry was replaced by a one-paragraph pointer under "Completed".
   - The `TASKLIST.md` section was removed.
   - `ROADMAP.md` gained a "Block N was completed on ..." paragraph.
   - The module documents and the proposal's outcome were updated.
6. **Verification evidence.** Screenshots and logs of scripted editor and player runs were kept in
   the session's temporary scratch folder, outside the repository, and only described in the
   records.

---

## 2. Why it became confusing

- **The ROADMAP became a log.** "Current direction" is a chronological narrative: blocks 1 to 34,
  with "Then the owner ..." and "Block N was completed ..." paragraphs, some out of order. The
  anchors, directions, next steps, and long-term goals the owner wants it to hold are buried in
  history. Finding "what is next" means reading the end of a 281-line file.
- **The archive mixes decisions with lifecycle events.** Decision records sit beside promotions and
  completions in one 70-row router, so the archive no longer answers "what started, what finished,
  what changed in policy" at a glance.
- **TODO entries are flat, one per block.** The things a roadmap item needs — decisions,
  alternatives, costs, proposals — lived in proposal files and in chat instead of in a TODO group.
  So the TODO never showed what remained for a roadmap item to be done.
- **Task IDs carry no structure.** A flat `PIXIE-TASK-NNN` does not say which roadmap item or TODO
  group it belongs to. `PXL 2.1.4` would.
- **"Completed" in `TODO.md` is an unordered pile** of 36 paragraphs, appended as blocks closed
  (020, 023, 021, ...).
- **Two sessions wrote in parallel.** PIXIE-TODO-030 and PIXIE-TODO-032 to 037 were built by the
  session "Push continuação"; PIXIE-TODO-031 was built here. One phase's history is spread across
  sessions and records.
- **Evidence is not versioned.** Records describe screenshots that no longer exist.

---

## 3. This session's failures

1. **It treated the archive as a log of every decision.** It wrote decision-only records (033, 041,
   046, 050) that, under the owner's model, do not belong there.
2. **It let `ROADMAP.md` accumulate history instead of direction.** At each promotion and closing it
   appended narrative paragraphs rather than keeping the roadmap a list of anchors, directions,
   next steps, and goals.
3. **It never decomposed a roadmap item into a TODO group.**
   - It wrote one TODO entry per block, with no sub-groups.
   - Decisions, alternatives, and costs went into proposals and chat instead.
   - The physics and lighting design in `ROADMAP-PROMOTIONS-PENDING.md` is the latest example: the Box2D-versus-own-library
     alternatives belong in `TODO.Physics`, not in a proposal.
4. **It numbered tasks in a single flat sequence**, unrelated to the roadmap or TODO structure.
5. **It built PIXIE-TODO-031 in this session**, while that work belonged to the other session
   because of this session's context limits. It informed the other session only after the owner
   asked whether it had.
6. **It kept verification evidence only in a temporary folder.**
7. **It offered to restructure the documentation system itself:** a new ROADMAP layout, tables,
   evidence folders, a generated status file. Restructuring is VAW's responsibility, not this
   session's; the offer added to the confusion.

What this session will do from now on: follow the model in section 2 of `ROADMAP-PROMOTIONS-PENDING.md` and whatever format VAW sets
up, and not change the planning documents' structure on its own.
```

## Owner dispositions after review

Appended on 2026-10-22; earlier snapshots and disposition text retain their
original baseline. This section supersedes the open dispositions above.

1. Ignore the planning-model/ID case; do not pursue a policy/schema migration.
2. The redundancy concern is recording "may implement" separately from the
   implementation report, not an instruction to erase all decision history.
   Codex did the same separation for 039/040. The documentation policy now
   distinguishes redundant permission logging from useful scope/lifecycle records.
3. Physics/lighting were parked as directions for later expansion while quota
   was short, as already done here. Keep direction and open design details.
4. The sandbox replacement is already handled; close this reconciliation case.
5. Missing evidence without any verification record cannot substantiate proof.
   An explicit written record of screenshot-based or human visual verification
   remains valid testimony. Distinguish it from surviving artifacts and fresh
   reproduction. Policy clarified; no blanket invalidation of historical reports.
6. Link the existing [requirements analysis](../circuit-puzzle-requirements.md),
   now clone-accessible, rather than create another analysis. This is consumer
   design input; it does not schedule all engine candidates.
7. The owner reconfirmed an editor-authoring failure: a blank project could not
   recreate the sample's particles entity/effect. Supplied images show a running
   particle sample and a different scene in Edit mode, Steps 0, emission rate 0,
   with ParticleSystem and ParticleSpawner components. These support the report
   and configuration context, but do not isolate the failure's cause. Static
   inspection still finds AddParticleStep in sample setup, not SceneProject.
   Reproduce nonzero emission in Play; inspect defaults, authoring configuration
   and runtime installation. No native reproduction or particle fix ran here.

The original requirements text is preserved with a source hash in the linked
consumer document. No extra authorization-only archive record was created for
these answers. Original attached images were copied to the ignored local review
folder for case 7; their continued presence is separate from this written record.

### Particle report clarification and postponement

Owner clarification, 2026-10-22: the screenshot preceded approximately 15 more
minutes of adjusting emission/configuration values without success. The visible
zero defaults do not explain away the report. Investigation is postponed until
2026-10-23 for consultation with Claude; no root cause has been established.
No message, scheduled reminder or implementation was performed.

## Final planning consistency review

Appended on 2026-10-22 following the owner's request for the NekoLib level of
documentation rigor. Earlier bodies and imported snapshots remain unchanged.

The roadmap now separates promoted work routing, accepted directions requiring
expansion, conditional investigations, deferred investigation and boundary
decisions. Conversation context and completed-work narratives no longer form
its current direction. Direction metadata reflects the latest reconciled owner
decision, rather than retaining only the 2026-10-14 phase date.

The current particle observation is owned by RUNTIME-FINDING-001 in the runtime
finding register; its deferred status and lack of native reproduction remain
explicit. Requirements own consumer design input, the scheduler owns execution,
and historical records retain prior states. The shared-engine proposal's old
"next after sprites" rationale was corrected to historical motivation. Archive
policy wording now agrees with the owner's nonredundant-authorization rule.

Review scope: roadmap, scheduler/task routing, consumer requirements, proposal
010/012 state, runtime finding routes, archive policy and relevant source leads
for particles and telemetry. Structural checks and semantic review are separate.
This is not a full engine/source audit, a revalidation of every past delivery,
or gameplay/visual acceptance. No particle fix, implementation promotion, native
run, build, commit or push was performed in this documentation pass.
