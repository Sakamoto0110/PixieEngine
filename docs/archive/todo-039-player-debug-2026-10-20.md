# Native Player Debug Controls Completed: PIXIE-TODO-039

**Document ID:** ARCHIVE-TODO-039-PLAYER-DEBUG-20261020

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** implementation and focused verification of native-player debug rates and display commands

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-075

**Type:** completion

**Origin:** PIXIE-TODO-039

**Author:** codex

**Reference date:** 2026-10-20

**Commits:** uncommitted working tree based on `2365251e7ea50fe730e937c03bdd404f46a86f40`

**Current state:** [player debug controls](../player-debug.md)

## Scope and implementation

The owner [promoted](player-debug-promotion-2026-10-20.md) F3 for read-only FPS/UPS and F3+C for
a top-docked command bar in the native `pixie::Run` host. There were no additional gates or
task breakdown. The commands are `-vsync true|false`, `-fps-cap true <integer >= 15>`, and
`-fps-cap false`. Invalid syntax leaves settings unchanged. Changes are per-run, applied on
the GL thread, and do not alter the fixed update rate or persist configuration.

`DebugConsole` owns parsing, shortcut state, and simple ASCII editing. `DebugOverlay` draws
after the game to the window framebuffer and restores the GL state it changes. It reuses the
existing `stb_easy_font` dependency and needs no game assets, World, or editor UI. `Run` captures
input while typing, cancels held controls on modal transitions/focus loss, and applies settings.
`FrameLimiter::SetRate` restarts the schedule and reuses its existing Windows timer. The additive
`Input::ReleaseAll` host operation is documented in the guide and public header.

Entity counts and further commands remain future work. C API hosts and the C# editor have
their own loops; these controls are inherited by games rebuilt against the native host.
No commit, push, package publication, or game-source change was performed.

## Verification

Machine: Windows 11 Pro `10.0.26200`, AMD Ryzen 7 5700X, Radeon RX 7600, Windows driver
`32.0.31041.1004`; GL `4.5.0 Core Profile Context 26.8.1.260810`. MSVC `19.51.36260.0` x64,
C++20, `/W4 /WX`, static CRT (`/MTd` Debug, `/MT` Release), Ninja Multi-Config. Source baseline
is the full engine HEAD above plus the working-tree changes; local source hashes identify them.
Binding/discovery used VAW `c9d81ba1fe538ddaaaa44f2ca48e1dc9d3dcfcb1`, profile `pixieengine`/1,
and the engine/build/documentation routes; this is provenance, not broader authorization.

An ignored consumer project under `.local/validation/player-debug/` adds the engine with
`PIXIE_BUILD_SAMPLES=OFF` and `PIXIE_ENGINE_BUILD_TESTS=ON`, keeping outputs away from root
`build/` and `bin/`. Both configurations built `engine-tests` and `runtime-tests` without compiler
warnings/errors. Direct execution passed **26 engine tests and 92 runtime tests per configuration**.
This is focused author verification; the C API, editor, and nested project-code suite were not run.

Coverage includes command grammar and bounds, unchanged settings after rejection, shortcut
repeat/chord behavior, caret/history/paste, modal cancellation, actual GLFW input callbacks,
pixel readback of window-overlay text, restoration of deliberately changed GL state, and live
frame-cap/VSync transitions in a real native window. The AMD context returned polygon-mode
values `0x1b01/0` both before and after the overlay; an initial test assuming two nonzero values
was corrected to compare the captured state. The product's restoration needed no repair.

The native test renders a minimal clear, not a game workload. Each phase lasts about 0.8 seconds
and includes a transition frame; rates are diagnostic samples, not refresh guarantees:

| Runtime mode | Debug FPS | Release FPS |
|---|---:|---:|
| VSync off, cap 30 | 28.6 | 30.0 |
| VSync off, cap 60 | 61.2 | 58.8 |
| VSync off, uncapped | 5670.4 | 6491.8 |
| VSync on, uncapped | 75.9 | 100.7 |
| VSync on, cap 15 | 16.1 | 16.1 |

The separate native preview, capped at 60 with VSync off and both panels visible, logged
`60.0 fps | 59.0 ups` then `60.0 fps | 60.0 ups`. Its screenshot was visually inspected:
command text, feedback, FPS, and UPS were legible at the top of the window.

For the required broken-copy oracle, the local consumer builds a copy of `DebugConsole.cpp`
with `rate >= 15` changed to `rate >= 1`, against the same command tests. It exited 1 with
11 failed checks, including rejection of 14 FPS and preservation of the previous cap.
The product source was never replaced with the broken copy.

Local evidence under `.local/validation/player-debug/`: `CMakeLists.txt`, `Preview.cpp`,
`build/BrokenDebugConsole.cpp`, `source-hashes.json`, `debug-engine.log`, `debug-runtime.log`,
`release-build.log`, `release-engine.log`, `release-runtime.log`, `mutation.log`, `preview.log`,
and `preview.png`. This ignored evidence is machine-local and is not shared proof after cloning.
Reproduction uses the tracked tests, headers, source, and settings recorded here. Documentation
and whitespace verification use `python scripts/check_docs.py` and `git diff --check`.
