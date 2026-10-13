# PIXIE-TODO-011: C# ImGui Editor over the Anchors

**Document ID:** ARCHIVE-TODO-011-20261013

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completed PIXIE-TODO-011, its task breakdown, and its verification

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-023

**Type:** completion

**Origin:** PIXIE-TODO-011 (PIXIE-TASK-052 to PIXIE-TASK-057)

**Author:** claude

**Reference date:** 2026-10-13

**Commits:** `b4a5f084016cf7347d248e4d9e193cb3a5036192`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** [`editor/README.md`](../../editor/README.md)

## Entry as completed

**Status:** completed on 2026-10-13

**Source:** owner decision on 2026-10-13: the editor starts as an ImGui view through Hexa.NET.ImGui; [PIXIE-PROPOSAL-002](../proposals/editor-native-bridge.md)

**Accepted scope:** a C# editor under `editor/`, in its own solution, referencing `PixieInspector.ImGui` 0.1.0 from NuGet with the GLFW and OpenGL3 backends of Hexa.NET.ImGui, on an OpenGL 4.5 core context. P/Invoke over `capi` through source-generated `LibraryImport`; a docked viewport that shows the World's texture with `ImGui.Image` and forwards the mouse to the engine; an entity list; an inspector bound by PixieInspector to hand-written C# proxies whose properties call into `capi`; pause, resume, single step, and reset. It opens the `capi` library of any of the three anchors.

**Dependencies and gates:** 1. PIXIE-TODO-010

**Completion criteria:** (1) for each anchor, the editor shows it running, the mouse steers it through the viewport, and editing a value in the inspector changes the running World; (2) the editor builds with zero warnings; (3) its `README.md` describes building and running it; (4) nothing is copied from PixieInspector's repository.

**Commits:** `b4a5f084016cf7347d248e4d9e193cb3a5036192` (the editor); its README and the documentation are updated with this record.

## Breakdown as completed

- `PIXIE-TASK-052` — **Status:** done — `editor/` project and solution: `PixieInspector.ImGui`, the GLFW and OpenGL3 backends of Hexa.NET.ImGui, an OpenGL 4.5 core window, and docking.
- `PIXIE-TASK-053` — **Status:** done — P/Invoke bindings over `capi` with source-generated `LibraryImport`, loading the library of the chosen anchor.
- `PIXIE-TASK-054` — **Status:** done — Viewport: the World's texture in a docked window with `ImGui.Image`, and the mouse forwarded in viewport coordinates.
- `PIXIE-TASK-055` — **Status:** done — Entity list and inspector: hand-written proxies whose properties call into `capi`, bound by PixieInspector.
- `PIXIE-TASK-056` — **Status:** done — Pause, resume, single step, and reset.
- `PIXIE-TASK-057` — **Status:** done — Its `README.md`; verification with each of the three anchors.

## Verification

Machine: Windows 11, AMD Radeon RX 7600 (driver 32.0.31041.1004), monitor at
100 Hz, .NET SDK 10.0.401; engine built in Release at `b4a5f08`; editor
commit `b4a5f08`.

- `dotnet build editor/PixieEditor.slnx` in Release and Debug, with warnings
  as errors: zero warnings. NuGet restored `PixieInspector.ImGui` 0.1.0 (with
  the core, PixieLib 0.2.0, and Hexa.NET.ImGui 2.2.9), Hexa.NET.GLFW 1.0.5,
  and the two backends 1.0.18.
- Each anchor in the editor, with real input through the operating system
  and the editor's saved last frame (1600x900):
  - boids: `controls` selected; `BoidControls.Mode = Repel` and
    `Radius = 300` set through the inspector (the `--set` option calls the
    inspector node's `SetValue`, the path an edit in the view takes); the
    inspector shows the new values, and with the cursor held at the
    viewport's middle the boids clear a 300-unit disc around it, the red ring
    centered on the cursor. A click into the viewport, then Space, Up, Up,
    and Down: the log reports repelling, then 1,100, 1,200, and 1,100 boids.
  - flocks: `flock` selected; `FlockControls.PointerMode = Repel` and
    `PointerRadius = 250` set through the inspector; the flocks part around a
    250-unit ring at the cursor.
  - particles: the center `attractor` selected; `Attractor.Strength =
    9000000` and `Softening = 30` set through the inspector and shown in it;
    a left drag in the viewport from (560, 270) by (+120, +60) added a second
    attractor (logged) at the end of the drag, and the particles gathered
    around both.
- The session panel shows the steps, time, entity count, and the last
  step's cost (boids 0.11 ms, flocks 9.4 ms, particles 9.0 ms in those runs);
  the inspector shows "No proxy for this component yet." for `Pointer` and
  `Bounds`.
- Found and fixed during the work:
  - an untyped PixieInspector (`Inspector.Create()`) is manual and showed no
    members; the editor now creates typed inspectors per proxy type;
  - the session's cursor starts at (0, 0), inside the view, so the pointer
    showed in the corner before the mouse came in; the viewport now sends a
    position outside the view until the mouse is over it;
  - ImGui's keyboard navigation took the arrow keys away from the viewport;
    it is off, and keys reach the game through a GLFW key callback that
    ImGui's backend chains.
- Testing note: synthetic key events sent with scan code 0 reach the GLFW of
  Hexa.NET.GLFW 1.0.5 as
  unknown keys (`key = -1`, scan code 256); the test script now sends each
  key's real scan code, as a keyboard does.
- Not resolved: the editor process faults with `0xC0000005` while exiting in
  about half the runs, after its window and every engine object are released.
  It does so too when it creates no engine session (the game's library is
  then never loaded): 3 of 6 runs; and PixieInspector's own ImGui host, which
  sets up GLFW and the ImGui backends the same way, faulted in 4 of 6 runs
  closed after 2 seconds. Unloading the game's library before exiting did not
  change the rate. The cause lies outside the engine and is not isolated; it
  is noted in the editor's README and reported to the owner.
- Not run: another platform (the editor is meant to run on Linux too, with
  `lib<game>-capi.so`), a display with system scaling, the Debug build of the
  engine under the editor.
