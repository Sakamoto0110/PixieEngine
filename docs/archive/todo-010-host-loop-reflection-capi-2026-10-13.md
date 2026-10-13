# PIXIE-TODO-010: Host Loop, Render Target, Reflection, and the C API

**Document ID:** ARCHIVE-TODO-010-20261013

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completed PIXIE-TODO-010, its task breakdown, and its verification

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-022

**Type:** completion

**Origin:** PIXIE-TODO-010 (PIXIE-TASK-046 to PIXIE-TASK-051)

**Author:** claude

**Reference date:** 2026-10-13

**Commits:** `1edd1526275879fbbf22ce84366088b561be4898`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** [`docs/modules/capi/`](../modules/capi/README.md) and the [runtime reference](../modules/runtime/REFERENCE.md#projects-and-sessions)

## Entry as completed

**Status:** completed on 2026-10-13

**Source:** owner decision on 2026-10-13: the viewport, reflection, and gameplay-code recommendations, and [PIXIE-PROPOSAL-002](../proposals/editor-native-bridge.md)

**Accepted scope:** general engine features that an editor, tools, and tests use alike: a loop that a host drives (step and render called from outside, which `pixie::Run` also uses); rendering into an offscreen target whose color texture a host displays; input fed from host events. Field descriptors (name, type, offset) registered per component, for the built-in and the anchors' components. The `capi` module, with its module documents: a shared library whose interface is C only, which creates and destroys a World from a project, steps it, renders it to a texture, takes input, enumerates entities and their components, and gets and sets fields through the descriptors, returning result codes and letting no exception cross the boundary. Each anchor's game code becomes a static library linked into its player executable and into a per-project `capi` library. On the host side, the engine library never calls GLFW.

**Dependencies and gates:** 1. PIXIE-TODO-007; 2. PIXIE-TODO-009 (all three anchors exist)

**Completion criteria:** (1) a test program that includes only the C header drives each anchor in a hidden window, renders it to a texture, and reads and writes a field; (2) the players still run as before; (3) zero warnings in Debug and Release; (4) the `capi` documents match its header.

**Commits:** `1edd1526275879fbbf22ce84366088b561be4898` (engine, runtime, capi, samples, and tests); the module documents and READMEs are updated with this record.

## Breakdown as completed

- `PIXIE-TASK-046` — **Status:** done — A host-driven loop: fixed steps and rendering called from outside, which `pixie::Run` also uses; input fed from host events.
- `PIXIE-TASK-047` — **Status:** done — Offscreen render target: a framebuffer with a color texture, resized on demand, whose texture ID a host displays.
- `PIXIE-TASK-048` — **Status:** done — Field descriptors (name, type, offset) registered per component, for the built-in and the anchors' components.
- `PIXIE-TASK-049` — **Status:** done — Each anchor's game code as a static library, linked into its player executable and its `capi` library, with a project entry point that builds its World.
- `PIXIE-TASK-050` — **Status:** done — Create the `capi` module, its library, and `docs/modules/capi/`: the C header, result codes, World lifetime, step, render to texture, input, entity and component enumeration, and field get and set.
- `PIXIE-TASK-051` — **Status:** done — Verification: a test program that includes only the C header drives each anchor in a hidden window; the players run as before.

## Verification

Machine: Windows 11, AMD Radeon RX 7600, monitor at 100 Hz, Visual Studio 2026
(MSVC 14.51); commit `1edd152`.

- Debug and Release build with zero warnings, including `capi-test`, a C11
  program compiled with `/W4 /WX`.
- `ctest` in Debug and Release: 5 of 5 pass: `engine-tests` (13 tests),
  `runtime-tests` (27), and `boids-capi`, `flocks-capi`, `particles-capi`.
- The C API test, per game library, loaded at run time from the path CTest
  gives it, in a hidden window with an OpenGL 4.5 core context:
  boids (9 component types, 1,001 entities, the oldest named "controls"),
  flocks (9 types, 5,001 entities, "flock"), and particles (7 types, 2
  entities, "particles"); each writes `Transform2D.position` and reads it
  back, runs 12 steps from 0.2 s and 15 more from 1 s (the 0.25 s clamp),
  pauses and single-steps, renders into a 320x200 texture with content, gets
  `PIXIE_ERROR_ARGUMENT` for a bad type and `PIXIE_ERROR_BUFFER` for a short
  buffer, and resets to step 0.
- Broken on purpose and restored, each caught: `pixie_field_set` doing
  nothing (the three C API tests fail); a point field written with x and y
  swapped (the reflection test and the three C API tests fail); the stepper's
  frame clamp removed (`engine-tests` and the C API tests fail).
- The players still run as before, with real input through the operating
  system and the saved frames: boids repel around the cursor after Space;
  flocks react to Q; a particles drag adds an attractor at (406, 240). The
  step costs match the earlier records when nothing pulls the boids together:
  boids 0.11 ms, flocks 7.1 to 7.5 ms without the cursor in the window. With
  the cursor attracting at the middle, the flock packs into a ball and its
  step rises to 12 to 15 ms.
- `pixie::Run` now steps through `FixedStepper`; hello-gl still runs at about
  100 frames and 60 updates per second.
- Departure from the entry's wording: field descriptors carry read and write
  functions generated from member pointers instead of byte offsets, so
  components need not be standard layout; the C API reaches fields only
  through them.
- Not run: Linux (the header and the test have `dlopen` and visibility
  branches, unbuilt), a host other than the test program, the editor itself
  (PIXIE-TODO-011).
