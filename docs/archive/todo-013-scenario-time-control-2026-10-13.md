# PIXIE-TODO-013: Scenario Values, Scene Rebuild, and Time Control

**Document ID:** ARCHIVE-TODO-013-20261013

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completed PIXIE-TODO-013, its task breakdown, and its verification

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-028

**Type:** completion

**Origin:** PIXIE-TODO-013 (PIXIE-TASK-063 to PIXIE-TASK-066)

**Author:** claude

**Reference date:** 2026-10-13

**Commits:** `9beb8de560743a643b02c10241667d40741cbd2b`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** the [runtime reference](../modules/runtime/REFERENCE.md#projects-and-sessions) and [`editor/README.md`](../../editor/README.md)

## Entry as completed

**Status:** completed on 2026-10-13

**Source:** owner decision on 2026-10-13 (requests D and time control)

**Accepted scope:** each anchor's starting values (counts, attractors, interaction) move into a scenario struct owned by its Project and mirrored in a `…Scenario` component that the editor inspects; rebuilding the scene reads the component back, so the World is built again with the edited values; a component type marked as a scenario gets a "Rebuild the scene" button in the editor. The players' options fill the same struct. The session gets a time scale (`pixie_session_set_time_scale`) from the steps 0.1, 0.25, 0.5, 0.75, 1, 1.25, 1.5, 1.75, and 2; buttons at the top of the editor slow time down, speed it up, and pause, showing the scale. The fixed step stays 1/60 s.

**Dependencies and gates:** 1. PIXIE-TODO-012

**Completion criteria:** (1) editing a count in an anchor's scenario and pressing the button rebuilds that anchor with it; (2) at 0.25 times a World's simulated time advances a quarter as fast as wall time, measured, and at 2 times twice as fast; (3) the players still accept their options; (4) zero warnings.

**Commits:** `9beb8de560743a643b02c10241667d40741cbd2b` (runtime, anchors, C API, editor, and tests); the references and READMEs are updated with this record.

## Breakdown as completed

- `PIXIE-TASK-063` — **Status:** done — Scenario structs owned by each anchor's Project, mirrored in `BoidsScenario`, `FlocksScenario`, and `ParticlesScenario` components, read back when the scene is rebuilt; the players' options fill them.
- `PIXIE-TASK-064` — **Status:** done — A scenario mark on component types, exported by the C API; the editor adds a "Rebuild the scene" button to a scenario's inspector.
- `PIXIE-TASK-065` — **Status:** done — Session time scale and `pixie_session_set_time_scale`; editor buttons to slow down, speed up, and pause, with the scale shown.
- `PIXIE-TASK-066` — **Status:** done — Verification: a rebuilt scene with edited counts, measured time at 0.25 and 2 times, the players' options.

## Verification

Machine: Windows 11, AMD Radeon RX 7600, monitor at 100 Hz, .NET SDK 10.0.401,
Visual Studio 2026 (MSVC 14.51); commit `9beb8de`, Release.

- Engine Debug and Release and the editor (warnings as errors): zero
  warnings. `ctest`: 5 of 5 pass; `capi-test` checks, per game, that a time
  scale of 0 is refused, that 0.25 turns 0.2 s of wall time into 3 steps and 2
  turns 0.1 s into 12, that `PixieStats.timeScale` reports it, and that
  exactly one type carries `PIXIE_TYPE_SCENARIO`.
- Rebuild: the editor on boids, the `scenario` entity selected,
  `BoidsScenario.count` set to 300 and the "Rebuild the scene" button pressed,
  both through the inspector nodes; the log shows the rebuild with 300 boids,
  and the simulated time restarts.
- Time scale, the editor's simulated time over about 4 seconds of wall time:

  | Game | Scale | Simulated | Wall |
  |---|---|---|---|
  | boids | 0.1 | 0.400 s | 4.009 s |
  | boids | 0.25 | 1.000 s | 4.003 s |
  | boids | 2 | 8.000 s | 4.005 s |
  | flocks (5,000) | 0.25 | 1.000 s | 4.005 s |
  | flocks (5,000) | 1 | 4.000 s | 4.005 s |
  | flocks (5,000) | 2 | 7.617 s | 4.011 s |

  At 2 times the flock needs two 7 ms steps per frame and falls about 5%
  behind, as the proposal expected.
- Real mouse: one click on "<< Slower" in the toolbar took the scale from 1
  to 0.75; the saved frame shows "time 0,75x" (the toolbar formats with the
  user's culture; the log is invariant).
- The players take their options into the scenario: `boids --count 300`,
  `flocks --count 700`, and `particles --count 50 --attractors 3
  --interaction off` start as asked.
- Not run: the pause button with the real mouse (the C API's pause is covered
  by `capi-test`), a rebuild of flocks or particles from the editor.
