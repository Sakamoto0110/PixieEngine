# PIXIE-TODO-002: Main Loop and the hello-gl Sample

**Document ID:** ARCHIVE-TODO-002-20261011

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completed PIXIE-TODO-002, its task breakdown, and its verification

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-006

**Type:** completion

**Origin:** PIXIE-TODO-002 (PIXIE-TASK-001 to PIXIE-TASK-010)

**Author:** claude

**Reference date:** 2026-10-11

**Commits:** `1b67d02430e54a51c492db4d30f16e8bf018a77d`, `f99f121a7a9f770bc1db620a8db6116d87d42cf9`, `ccf1db9e7537f20f3d9eb127c492984ee1eefa5e`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** [`README.md`](../../README.md) for building and running; the code under `engine/` and `samples/hello-gl/`

## Entry as completed

**Status:** completed on 2026-10-11

**Source:** owner decision on 2026-10-11

**Accepted scope:**

- A root CMake build with GLEW (static) and GLFW built from `external/`.
- A provisional engine library: a window with an OpenGL 4.5 core context,
  GLEW initialization, GL debug output, and a main loop that calls `Update` at
  a fixed, configurable rate and `Render` under vsync, a frame cap, or
  uncapped, both receiving a `FrameTime`. It logs the measured update and
  frame rates.
- A `hello-gl` sample, a separate project that only consumes the engine: balls
  bouncing off the window edges, with their structs defined in the sample,
  "hello GL" drawn in the middle, and the measured frame and update rates drawn
  in a corner.
- Added by owner request on 2026-10-11, during the work: executables in
  `bin/<config>/`; a generated `PixieEngine.slnx` at the repository root that
  shows only the projects worth opening; the `PIXIE_BUILD_SAMPLES` option; and
  the static MSVC runtime.

Out of scope: the module split (PIXIE-PROPOSAL-001), `capi`, the editor, a
general renderer in the engine, interaction, and physics beyond reflecting off
the edges. The engine layout is provisional until the module split is decided.

**Commits:** `1b67d02430e54a51c492db4d30f16e8bf018a77d` (vendored GLEW, GLFW,
and stb_easy_font) and `f99f121a7a9f770bc1db620a8db6116d87d42cf9` (build,
engine, and sample).

## Breakdown as completed

- `PIXIE-TASK-001` — **Status:** done — Root CMake build, presets, and the GLEW
  and GLFW targets from `external/`.
- `PIXIE-TASK-002` — **Status:** done — Engine window, OpenGL 4.5 core context,
  GLEW initialization, and GL debug output.
- `PIXIE-TASK-003` — **Status:** done — Main loop: fixed-rate `Update`,
  `Render` with `FrameTime` and interpolation alpha, vsync, frame cap, and
  measured rates.
- `PIXIE-TASK-004` — **Status:** done — hello-gl sample: bouncing balls with
  the sample's own structs, drawn interpolated.
- `PIXIE-TASK-005` — **Status:** done — hello-gl sample: "hello GL" centered,
  drawn with `stb_easy_font`.
- `PIXIE-TASK-006` — **Status:** done — Verification against the completion
  criteria.
- `PIXIE-TASK-007` — **Status:** done — hello-gl sample: the measured frame and
  update rates drawn in the top-left corner.
- `PIXIE-TASK-008` — **Status:** done — Executables in `bin/<config>/` and the
  build tree in `build/`.
- `PIXIE-TASK-009` — **Status:** done — Generated root `PixieEngine.slnx`
  without `ALL_BUILD` and utility targets, and the `PIXIE_BUILD_SAMPLES`
  option.
- `PIXIE-TASK-010` — **Status:** done — Static MSVC runtime for every target.

## Verification

Machine: Windows 11, AMD Radeon RX 7600 (driver 32.0.31041.1004), monitor at
100 Hz, Visual Studio 2026 (MSVC 14.51), CMake 4.3.1 bundled with it.

- Configure and build from an empty `build/`, Debug and Release, with `/W4 /WX`:
  zero warnings in the project's own targets.
- The log reports `GL_VERSION: 4.5.0 Core Profile Context 26.8.1.260810`; GL
  debug output is on in Debug and off in Release, where the context is not a
  debug context.
- Measured rates, from the engine's log, in 5- to 6-second runs:

  | Configuration | Frames per second | Updates per second |
  |---|---|---|
  | vsync on | about 100 | 60 |
  | vsync off, no cap | 3,300 to 3,800 | 60 |
  | cap 144 | 144.0 | 60 |
  | cap 30 | 30.0 | 60 |
  | vsync on, 120 updates per second | about 100 | 120 |
  | cap 50, 30 updates per second | 50.0 | 30 |

  Updates per second vary by about one around the configured rate between
  one-second windows.
- Gaps: frame rates dropped to 78 to 86 during seconds in which a window
  capture brought the window to the foreground while PowerShell compiled the
  capture script. One run with vsync on and 120 updates per second dropped to
  about 85 frames per second without a capture; it did not reproduce in four
  later runs.
- Visual check from window captures: the balls bounce off the edges, "hello GL"
  is centered, and the rates are drawn in the top-left corner.
- The root solution builds with MSBuild from Visual Studio 2026; configuring
  with `PIXIE_BUILD_SAMPLES=OFF` in a separate build directory leaves the
  samples out and does not change the root solution.
- With the static runtime, the Release executable depends only on
  `OPENGL32.dll`, `KERNEL32.dll`, `USER32.dll`, `GDI32.dll`, and
  `SHELL32.dll`; it no longer needs `MSVCP140.dll` or `VCRUNTIME140.dll`.
- Not run: anything outside this machine, including another GPU, another
  operating system, or a machine without Visual Studio installed.
