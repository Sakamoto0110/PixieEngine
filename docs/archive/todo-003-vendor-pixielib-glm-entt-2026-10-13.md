# PIXIE-TODO-003: PixieLib, GLM, and EnTT Vendored

**Document ID:** ARCHIVE-TODO-003-20261013

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completed PIXIE-TODO-003, its task breakdown, and its verification

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-015

**Type:** completion

**Origin:** PIXIE-TODO-003 (PIXIE-TASK-011 to PIXIE-TASK-015)

**Author:** claude

**Reference date:** 2026-10-13

**Commits:** `89b5e2fe1e8d0a76f255ca5da74fb117f4ebd5da`, `c38d25715463bf96d8f3c42ef1282e65aacd78e7`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** [`DEPENDENCIES.md`](../../DEPENDENCIES.md) and `external/CMakeLists.txt`

## Entry as completed

**Status:** completed on 2026-10-13

**Source:** owner decision on 2026-10-13: PixieLib vendored with its GLM; EnTT as the component storage

**Accepted scope:** PixieLib v0.2.0 under `external/pixielib/` and GLM 1.0.3 under `external/glm/`, from their release archives, unmodified. PixieLib's own CMake project builds `pixie::pixie` and `pixie::math`, with its GLM `FetchContent` pointed at the vendored copy, so that configuring never needs the network. EnTT v4.0.0 under `external/entt/`, unmodified, as a header-only target. `DEPENDENCIES.md` records each release, source, license, and SHA-256. The engine links them; nothing uses them yet.

**Dependencies and gates:** none

**Completion criteria:** (1) each tree is identical to its release archive, whose SHA-256 is recorded; (2) a configure from an empty `build/` with `FETCHCONTENT_FULLY_DISCONNECTED=ON` succeeds; (3) Debug and Release build with `/W4 /WX` and zero warnings in the project's own targets, with an engine translation unit that includes PixieLib, PixieLib math, and EnTT headers; (4) the vendored code is in its own commit.

**Commits:** `89b5e2fe1e8d0a76f255ca5da74fb117f4ebd5da` (the vendored trees) and `c38d25715463bf96d8f3c42ef1282e65aacd78e7` (the build wiring and the engine's library check).

## Breakdown as completed

- `PIXIE-TASK-011` — **Status:** done — Download the PixieLib v0.2.0 source archive, verify it, and copy it unmodified to `external/pixielib/`.
- `PIXIE-TASK-012` — **Status:** done — Download the GLM 1.0.3 release archive, verify it, and copy it unmodified to `external/glm/`.
- `PIXIE-TASK-013` — **Status:** done — Download the EnTT v4.0.0 source archive, verify it, and copy it unmodified to `external/entt/`.
- `PIXIE-TASK-014` — **Status:** done — Wire the targets in `external/CMakeLists.txt`: PixieLib through its own project with `FETCHCONTENT_SOURCE_DIR_PIXIE_GLM` set to `external/glm/`, EnTT as an interface target, both with system include directories; link them to the engine and include their headers from one engine source.
- `PIXIE-TASK-015` — **Status:** done — Record the three in `DEPENDENCIES.md` and `.gitattributes`; verify a disconnected configure and both configurations with zero warnings.

## Verification

Machine: the one recorded for PIXIE-TODO-002 (Windows 11, AMD Radeon RX 7600,
Visual Studio 2026 with MSVC 14.51 and CMake 4.3.1).

- Archives, downloaded on 2026-10-13 and inspected in an empty directory
  outside the repository: PixieLib `v0.2.0.tar.gz` (SHA-256
  `942d70a7…3389`), identical to `git archive v0.2.0` of the local checkout
  apart from line endings; GLM `glm-1.0.3.zip` (SHA-256 `1c0a0fce…7deb2`),
  equal to the digest GitHub publishes for the asset; EnTT `v4.0.0.tar.gz`
  (SHA-256 `32a2ff2c…5212e`). No binaries in any of them. The full hashes are
  in `DEPENDENCIES.md`.
- `diff -r` between each extracted archive and its `external/` tree: no
  difference. The trees were added with `git add -f`, because each carries its
  own `.gitignore`; the file counts in the index match the trees (PixieLib 122,
  GLM 1,672, EnTT 374). `.gitattributes` stores them with `-text`.
- Configure from an empty `build/` with `-DFETCHCONTENT_FULLY_DISCONNECTED=ON`:
  succeeds; `build/_deps` holds no download, and the cache shows
  `FETCHCONTENT_SOURCE_DIR_PIXIE_GLM` at `external/glm`.
- Debug and Release build: zero warnings in `pixie_engine` and `hello-gl`;
  `pixie_math` builds with PixieLib's own settings.
- `engine/src/LibraryCheck.cpp` includes EnTT's registry and PixieLib's
  primitives and math under `/W4 /WX`. Changing one of its `static_assert`s to a
  wrong size fails the build with `C2607`; restored afterwards.
- `hello-gl --quit-after 3` (Release) runs and exits 0 at about 100 frames and
  60 updates per second, as before.
- The root `PixieEngine.slnx` gains `pixie_math` under `external`.
- Not run: a configure on another machine, or with PixieLib's own tests.
