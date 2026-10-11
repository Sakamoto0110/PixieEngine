# C++20, CMake, and an Explicit FrameTime

**Document ID:** ARCHIVE-CPP20-CMAKE-FRAMETIME-20261010

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** language standard, native build system, and how time reaches Update and Render

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-005

**Type:** decision

**Origin:** owner decision (open questions 1 and 3; timestep discussion)

**Author:** claude

**Reference date:** 2026-10-10

**Commits:** `ccf1db9e7537f20f3d9eb127c492984ee1eefa5e`

**Current state:** [`ROADMAP.md`](../../ROADMAP.md#active-guardrails)

- **C++20 without modules.** Reasons are in the
  [inception record](inception-2026-10-09.md#recommendations-not-yet-accepted).
  Moving to C++26 is an intention to evaluate later.
- **CMake for the native side.** The editor is gaining hosts beyond Windows,
  and the native code must build where they run; GLFW also ships its own CMake
  project.
- **`FrameTime` passed to `Update` and `Render`.** The owner proposed a static
  timestep read through a getter. A struct was chosen instead: one getter would
  mean the fixed step in `Update` and the frame delta in `Render`; several
  Worlds (play mode, a paused preview) need their own time; and a hidden
  dependency makes tests and substeps harder.

Accepted on 2026-10-10 and written into `ROADMAP.md` in the commit above.
