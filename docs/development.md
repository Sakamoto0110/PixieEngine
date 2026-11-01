# Development Guide

**Document ID:** GLOBAL-DEVELOPMENT-GUIDE

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** how to build, check, and commit a change to PixieEngine

**Surface:** guide

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

This guide collects the practices needed to change PixieEngine safely. It does
not replace the sources that own technical truth: source code owns behavior,
the CMake files own targets and options, module `REFERENCE.md` files own the
documented contracts, and the
[documentation policy](governance/documentation-policy.md) owns the commit
rules.

## Working agreements

- Change the design only after the owner confirms it. When the owner asks for
  a change, apply it and verify it; commit and push when the owner says so.
- Simplify, but keep one concept per file; do not merge files to lower the
  count.
- Redesign one thing at a time, and fix the cause rather than a symptom.
- Names in a request are examples, not specifications.
- Ask numbered questions, so the owner can answer by number ("3: yes"). When
  the owner asks for an opinion, give a recommendation, not a list of options,
  and its cost. Questions live in `.local/decisions/open-questions.md`, or in
  the conversation when `.local/` does not survive the session; numbers are
  never reused.
- The engine never depends on the editor or on the samples; a sample consumes
  only the engine.
- Before calling a change done, check that the verification catches the error:
  undo the change, or break a copy on purpose, and watch the check fail.

## Build

Requirements: Visual Studio 2026 with the C++ workload. It ships CMake and
Ninja, which are on `PATH` in its developer prompt. From the repository root:

```bash
cmake --preset vs2026
cmake --build --preset vs2026-debug
cmake --build --preset vs2026-release
```

- The configure step writes `build/` and the root `PixieEngine.slnx`, with the
  engine, the samples, and the dependencies built from source. Open the
  solution, or open the folder in Visual Studio. Both are generated: to add a
  file or a project, change CMake and configure again.
- Executables land in `bin/Debug/` and `bin/Release/`.
- `-DPIXIE_BUILD_SAMPLES=OFF` builds the engine alone, and
  `-DPIXIE_ENGINE_BUILD_TESTS=OFF` leaves out the tests (PixieLib owns the name
  `PIXIE_BUILD_TESTS`, for its own tests, which stay off).
- Native targets use the shared C++ runtime: `/MDd` in Debug and `/MD` in
  Release. The engine is built once as `pixie.dll`; game players link their
  code directly and development builds also produce `<game>-module.dll`.
  The shared native dependency `glfw3.dll` belongs beside the corresponding
  binaries. App-local Visual C++ runtime export deployment is being qualified
  under [PIXIE-TODO-051](../TODO.md#pixie-todo-051--shared-engine-game-export-and-local-mcp-development-channel);
  a development build directory is not yet a qualified standalone export.
- The project's own targets compile with `/W4 /WX`: a warning is an error.
  Third-party code under `external/` compiles with its own settings.
- On x86-64 they also compile for AVX2 (`/arch:AVX2`; `-mavx2 -mfma -mf16c`
  elsewhere), with FMA and F16C, so the binaries need a CPU with AVX2 (Intel
  since 2013, AMD since 2015). `-DPIXIE_AVX2=OFF` builds for an older one. The
  setting is in `cmake/PixieTargetDefaults.cmake`, which games built with
  `pixie_game` share.

`build/` and `bin/` can be deleted at any time; configure again.

The C# editor builds on its own, with the .NET 10 SDK, after the engine (it
loads the shared engine and selected development game module from `bin/`):

```bash
dotnet build editor/PixieEditor.slnx -c Release
```

It compiles with warnings as errors. Its output stays under `editor/`; see
[`editor/README.md`](../editor/README.md).

## Checks before a commit

- **Build:** Debug and Release build with 0 warnings and 0 errors.
- **Tests:** `ctest --preset vs2026-debug` and `ctest --preset vs2026-release`
  pass. Tests live under `tests/`, with a minimal harness (`tests/Check.hpp`).
  The C API tests (`boids-capi`, `flocks-capi`, `particles-capi`,
  `pixie-scene-capi`, the samples' `<sample>-project`, and `project-code`)
  open a hidden window, so they need a
  desktop session with an OpenGL 4.5 driver. `project-code` builds a project
  with code, and the engine with it, in `build/tests/project-code/`, so it
  takes a minute or two.
- **Run:** the samples that exercise the change start, show what their README
  describes, and exit 0. `hello-gl --quit-after 5` runs for five seconds and
  closes; its log shows `GL_VERSION` and the measured rates once per second.
  Every sample takes `--screenshot <file.png>`, which saves the last frame
  before `--quit-after` closes the window.
- **Rates:** a change to the loop or the frame limiter is measured with vsync on,
  vsync off, and a frame cap, and the numbers are recorded with the machine and
  commit (see the [policy](governance/documentation-policy.md#tests-and-evidence)).
- **Development channel:** a change to `devchannel/`, `engine/platform/src/LocalCommandChannel.cpp`,
  `editor/PixieEditor/Development/`, `tools/pixie-mcp*` or `tests/editor/channel/` also passes
  `py tests/mcp/qualify_mcp.py --config Debug` and `--config Release` (see the
  [editor's README](../editor/README.md#local-development-mcp-channel)). It is not a CTest test:
  it builds the managed bridge and editor, and its cold code project builds the engine again.
- **Documentation:** `python scripts/check_docs.py` reports no problem, and
  the policy's [self-review](governance/documentation-policy.md#self-review)
  passes for what the script does not check. The script checks relative links
  and anchors, metadata fields and their vocabulary, the cross-links and task
  IDs of `TODO.md` and `TASKLIST.md`, and the archive router; it needs Python 3.
- **Whitespace:** `git diff --check` is clean, outside `external/`.

## Continuous integration

GitHub Actions runs `.github/workflows/ci.yml` on every push to `main`, on pull
requests, and by hand:

- **Documentation** (Linux): `python scripts/check_docs.py`.
- **Engine** (Windows): the `ci` preset, Ninja Multi-Config with the compiler of
  the Visual Studio the runner has (the workflow finds it with `vswhere`, enters
  its developer environment, and logs its version), built in Debug and Release
  and tested with `ctest --preset ci-debug` and `ci-release`. The runners have
  no GPU, so Mesa's software renderer, llvmpipe (a pinned release of
  mesa-dist-win, checked by SHA-256), is placed next to the tests for the
  OpenGL 4.5 core context they open; the log shows the renderer they get.
- **Editor** (Windows): `dotnet build editor/PixieEditor.slnx -c Release`.

Warnings are errors there as here: what the runner's compiler reports is fixed
in the code. After every push, its run is checked to the end, and a red `main`
comes before anything else. The `ci` preset also runs locally from a developer
prompt, into `build-ci/`.

## Dependencies

Third-party code lives unmodified under `external/<name>/`, built by
`external/CMakeLists.txt`, and is stored byte for byte (`-text` in
`.gitattributes`). Adding or updating one:

1. Download the release archive, verify its published SHA-256, and inspect it
   in an empty directory outside the repository before copying it in.
2. Record the version, source, license, and hash in `DEPENDENCIES.md`.
3. Commit the vendored code alone, before the code that uses it.

## Void Agent Workplace

On the owner's machine, this repository lives inside a Void Agent Workplace
checkout, in its Pixie family of managed projects (`projects/Pixie/`, beside
PixieInspector and PixieLib), and is bound to the VAW profile `pixieengine`,
revision 3. The checkout is on the local branch `codex/pixie-workplace`
(commit `bb1c8b3`, VAW 0.3.0.0), whose `profiles/pixieengine/` holds the
profile. The binding is in an explicit registry for the Pixie family,
`.local\bindings\pixie.json` in the VAW checkout, passed with `-BindingsPath`
on every command: agents have seen different copies of the default AppData
registry ([PIXIE-ARCHIVE-071](archive/vaw-explicit-registry-2026-10-16.md)). VAW ignores
`projects/`, so the repository keeps its own history and remote. The profile
maps VAW's authority roles to this repository's documents, routes each source
area to the documents that own it, and lists the full matrix as its
validation. Nothing in this repository depends on it: a machine without it
builds, checks, and commits as before.

The three Pixie profiles (`pixieengine`, `pixieinspector`, `pixielib`) share one
revision number and are revised together, even when only one of them changes.

The profile adopts, besides binding and skill discovery: workflow records with
planned-pause checkpoints that another agent can verify and resume, scoped
premises (to skip re-reviewing validated code that has not changed), repository
state in those records, multi-agent validation and documentation records,
package validation, and product versioning (below). Campaigns stay frozen in
VAW. Two skills of this profile, `pixieengine-documentation` and
`pixieengine-qualification`, route an agent to this guide and the
[documentation policy](governance/documentation-policy.md); the repository's
documents remain the authority. VAW records live in `.local/vaw/` (unversioned)
and never replace `TODO.md`, `TASKLIST.md`, the archive, or the code: they point
to them.

Before a task, an agent resolves the binding for the task's routes (`core`,
`platform`, `render`, `app`, `engine`, `runtime`, `capi`, `devchannel`, `tools`,
`editor`, `samples`, `build`, `documentation`), from the repository root, in
PowerShell 7 (the VAW checkout is three directories up):

```powershell
$bindings = (Resolve-Path ..\..\..\.local\bindings\pixie.json).Path  # VAW needs it absolute
..\..\..\scripts\resolve-project-binding.ps1 -RepositoryPath . -BindingsPath $bindings `
    -CompatibilityScope Routes -RouteId runtime,documentation
..\..\..\vaw.ps1 skills list -Agent claude -RepositoryPath . -BindingsPath $bindings
```

Select all routes affected by the actual task; `runtime,documentation` is an
example. `skills show <name>` takes the same arguments as `skills list`, and
Codex passes `-Agent codex`.

Never delete ignored files in the VAW checkout (`git clean -x` and the like):
`projects/` is ignored there, so that would delete this repository and its
unpushed work.

It requires `bound_scoped` with every selected route checked (or `bound` without
`-CompatibilityScope`), and the task's archive record states the VAW commit, the
profile and revision, and the routes checked. A binding grants no authority:
commits, pushes, and releases follow the rules below.

## Product version

The root `version.json` owns the engine's product version, in four components:
`MAJOR.MINOR.PATCH.REVISION` (VAW's `void.versioning/2`). CMake reads it as the
project version; the player logs `PixieEngine <version>` when it starts, the
editor shows the public projection (the first three components) in its title,
and an export's manifest records it as `engine_version`.

The version rises once per change set that reaches `main` (a merged pull
request, or a closed `TODO.md` entry), by its highest-impact change: `MINOR`
for a compatible capability, `PATCH` for a correction, `REVISION` when external
behavior is unchanged (documentation, infrastructure, refactors). Below 1.0 the
engine is in initial development and a break raises `MINOR`; 1.0 is an owner
decision. Raise it with VAW's `scripts/update-version.ps1 -VersionPath
version.json -Component <component> -ExpectedVersion <current>`, and cite the
new version in the change's archive record.

Other version axes stay independent and change on their own rules: the C API
(`PIXIE_CAPI_VERSION`), the scene and documentation schemas, and the VAW profile
revision. A product version grants no authority to tag, push, or publish.

## Commits and pushes

The [commit rules](governance/documentation-policy.md#commits) apply: prefixes,
code and documentation in separate commits, the owner's identity only, work on
`main`, and nothing pushed without an owner decision. A documentation commit
that closes work also adds its archive record and router row, citing the code
commits by full hash.
