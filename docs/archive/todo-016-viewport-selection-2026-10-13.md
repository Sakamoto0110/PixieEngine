# PIXIE-TODO-016: Selection and Editing in the Viewport

**Document ID:** ARCHIVE-TODO-016-20261013

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completed PIXIE-TODO-016, its task breakdown, and its verification

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-032

**Type:** completion

**Origin:** PIXIE-TODO-016 (PIXIE-TASK-074 to PIXIE-TASK-077)

**Author:** claude

**Reference date:** 2026-10-13

**Commits:** `c7988f3cda3d3c8a12e3ebc2521c48e6bc373357`, `a1606b2fa719a20fe1c7b4545a2f910647f2255d`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** the [editor README](../../editor/README.md)

## Entry as completed

**Status:** completed on 2026-10-13

**Source:** owner decision on 2026-10-13 (request C, answers 34, 40, and 41)

**Accepted scope:** the viewport gets a "Game" and a "Select" mode. In Select mode: a left click on an entity (a boid, an attractor, a spawner) selects it, Shift + left click adds it to the selection, and a drag on empty space selects by box; Ctrl + left click on empty space asks the project to create something there (`Project::SpawnAt`: a random boid in boids and flocks, a particle at rest in particles), and Ctrl + left click on a boid or a particle deletes it. Particles are never selected. The entity list takes Ctrl and Shift clicks too. The inspector shows the components every selected entity has, bound to the first and with `AddBind` to the rest. Selected entities are outlined in the viewport. The C API gains picking (a point, a box), destruction, particle removal, and spawning.

**Dependencies and gates:** 1. PIXIE-TODO-012; 2. PIXIE-TODO-015; 3. PixieInspector.ImGui 0.2.1, for criterion (2): 0.2.0 draws a mixed number that scrubs with the first object's value and no marker; 0.2.1 draws every mixed row's label in amber with a tooltip saying the objects differ ([Sakamoto0110/PixieInspector#8](https://github.com/Sakamoto0110/PixieInspector/pull/8), published on 2026-10-13)

**Completion criteria:** (1) each gesture works in each anchor as described, with real input; (2) editing a value with several attractors selected changes all of them, and different values show as mixed; (3) in Game mode the mouse still goes to the game; (4) `capi-test` covers the new functions; (5) zero warnings.

**Commits:** `c7988f3cda3d3c8a12e3ebc2521c48e6bc373357` (picking, SpawnAt, the C API, the editor's modes, gestures, and multi-entity inspector, and tests) and `a1606b2fa719a20fe1c7b4545a2f910647f2255d` (PixieInspector.ImGui 0.2.1, for the mixed marker); the runtime and capi references and the editor README were updated in `ff486ed`, and DEPENDENCIES.md with this record.

## Breakdown as completed

- `PIXIE-TASK-074` — **Status:** done — C API: picking at a point and in a box, entity destruction, particle removal at a point, `pixie_spawn_at`, and the view bounds of an entity; `Project::SpawnAt` in each anchor.
- `PIXIE-TASK-075` — **Status:** done — Viewport modes, Game and Select, with the click, Shift, Ctrl, and box gestures, and outlines of the selection.
- `PIXIE-TASK-076` — **Status:** done — Multi-selection in the entity list, and inspectors over the components common to the selection, with `AddBind`.
- `PIXIE-TASK-077` — **Status:** done — Verification with real input in each anchor; `capi-test` for the new functions; the documents.

## Verification

Machine: Windows 11, AMD Radeon RX 7600, monitor at 100 Hz, Visual Studio 2026
(MSVC 14.51), .NET SDK 10.0.401; commits `c7988f3` and `a1606b2`.

- Debug and Release build with zero warnings, and the editor with zero
  warnings; `ctest` passes 5 of 5 in both, with 39 runtime tests.
- New runtime tests: picking at a point, nearest center first, within the
  tolerance, and widened by the transform's scale; only entities with a
  Transform2D and a Shape picked; a box listing centers only, oldest first;
  the nearest particle within a radius removed. Broken on purpose and
  restored, each fails its test: the tolerance ignored, the box order not
  reversed, the first particle in reach removed instead of the nearest.
- `capi-test`, in each of the three games, paused: the first pickable entity
  is found again by `pixie_pick` at its view bounds' center and by
  `pixie_pick_box` around it, then destroyed (a second destroy and its bounds
  report not found); `pixie_spawn_at` creates one there; in particles the
  particle spawned there is removed by `pixie_particle_remove_at`, elsewhere it
  reports not found; argument errors are checked.
- Real input through the operating system, in the editor:
  - particles, Select mode, paused: a click on the spawner and a Shift +
    click on the attractor selected both, outlined, with the list showing "2
    selected" and the inspector the components they share; Ctrl + click on
    empty space created a particle at rest, twice, and a third Ctrl + click
    on the second deleted it; a `--set` of the transform's scale to 2 with
    the spawner selected grew its ring.
  - boids, 40 boids, Select mode, paused: a box drag selected 4, a Shift +
    click added a fifth, all outlined, and the inspector showed "5
    entities" over Transform2D, Velocity2D, and Shape; Ctrl + click deleted a
    boid and Ctrl + click on empty space created one.
  - flocks, 5,000 boids: a box selected 102, outlined, at 109 frames per
    second with 102 binds; two Ctrl + clicks deleted the boids under the
    cursor.
  - particles, Game mode: a left click on empty space added an attractor, so
    the mouse still goes to the game.
- Mixed values: with PixieInspector.ImGui 0.2.0 a mixed number that scrubs
  showed the first entity's value with no marker; reported to PixieInspector,
  fixed in 0.2.1 (Sakamoto0110/PixieInspector#8). With 0.2.1, the spawner and
  the attractor selected show `position x`, `size x`, `size y`, and `color`
  with amber labels and the shared rows in white; hovering `position x` shows
  "The objects hold different values; this shows the first one's."
- One Shift + click in an automated run was missed once and not reproduced;
  the viewport now judges hover from the current frame's mouse instead of the
  previous frame's item, and later runs did not miss.
- Not run: Linux.
