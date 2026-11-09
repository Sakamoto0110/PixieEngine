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

- The configure step writes `build/PixieEngine.slnx` with the engine, samples,
  and source dependencies. CMake owns its project list. Edit the CMake
  targets and configure again to add a file or project. The Pixie workspace
  generator reads the curated project list in `build/PixieEngine.root.slnx`.
  Run `py -3 update_pixie_workspace.py` from the Pixie workspace root to
  refresh the shared solution after configuring.
- Executables land in `bin/Debug/` and `bin/Release/`.
- `-DPIXIE_BUILD_SAMPLES=OFF` builds the engine alone, and
  `-DPIXIE_ENGINE_BUILD_TESTS=OFF` leaves out the tests (PixieLib owns the name
  `PIXIE_BUILD_TESTS`, for its own tests, which stay off).
- Native targets use the shared C++ runtime: `/MDd` in Debug and `/MD` in
  Release. The engine is built once as `pixie.dll`; game players link their
  code directly and development builds also produce `<game>-module.dll`.
  The shared native dependency `glfw3.dll` belongs beside the corresponding
  binaries. App-local Visual C++ runtime export deployment was qualified for Windows
  under [PIXIE-TODO-051](archive/todo-051-shared-engine-export-mcp-2026-11-01.md#pixie-todo-051--shared-engine-game-export-and-local-mcp-development-channel);
  use the manifest exporter for a qualified standalone game folder; copying the
  development bin directory is not a shipping procedure.
- The project's own targets compile with `/W4 /WX`: a warning is an error.
  Third-party code under `external/` compiles with its own settings.
- On x86-64 they also compile for AVX2 (`/arch:AVX2`; `-mavx2 -mfma -mf16c`
  elsewhere), with FMA and F16C, so the binaries need a CPU with AVX2 (Intel
  since 2013, AMD since 2015). `-DPIXIE_AVX2=OFF` builds for an older one. The
  setting is in `cmake/PixieTargetDefaults.cmake`, which games built with
  `pixie_game` share.

`build/` and `bin/` can be deleted at any time; configure again.

PixieEditor builds in its own repository with an installed Engine SDK.
The [migration guide](../editor/README.md) points to its build, publish and qualification entry points.

### Build and rehearsal storage

For an iteration on the Engine DLL, build the affected target and configuration
in the existing CMake tree. The command
`cmake --build --preset vs2026-release --target pixie_shared` builds the Release
DLL and its dependencies. Build the
specific sample or test target when the change needs it. Use the full presets
when the commit or qualification checks require them. A game consuming an
installed SDK uses the Editor's project build and needs no Engine checkout build.

Reuse an incremental tree while the generator, toolchain and SDK inputs are
compatible. A multi-config tree can hold both Debug and Release. Give a run its
own tree when isolation, a different SDK or a clean qualification check requires
one. Keep rerun instructions, identities, relevant logs and receipts as
the durable evidence. Retain the exact verified archive when the result depends
on those binary bytes. Treat extraction trees, staged copies and object files as
working outputs once their checks finish and any required evidence is recorded.
Before retaining another full copy, record what it is needed for and when that
need ends. Reuse the same verified package across local runs where the test
permits it. Keep installed SDKs, active caches and evidence needed by open work
separate from scratch outputs.

## Installed SDK and local rehearsal

Start with the supplied SDK root containing `sdk.json`. Select a declared
profile and configuration, and record the expected SDK ID, build ID and
catalog/profile hashes from the producer receipt. Match the native toolchain,
architecture, CPU features and CRT declared by the selected profile. Run the
verifier shipped with the SDK before configuring a consumer.

```powershell
$sdk = 'C:\qualification\SDK'
python -B "$sdk\tools\sdk\verify_sdk.py" "$sdk"
```

Use `--complete` when the supplied artifact is a complete distribution. The
verifier checks the package inventory and profile metadata. The
[installed consumer fixture](../tests/sdk-consumers/CMakeLists.txt) separately
configures against the explicit SDK root, profile, build configuration and
identity pins. Give it a fresh build directory and the matching toolchain.
Package mode resolves the SDK's CMake package without an Engine source build.

The [focused consumer runner](../tests/separation/g1.py) takes a reviewed
execution input with the pinned SDK, fixture and tools. From the Engine
repository root, run it with a fresh output directory.

```powershell
python -B tests/separation/g1.py --inputs "C:\qualification\g1-inputs.json" --out "C:\qualification\g1-run"
```

Read `results.json` and the configure, build and test receipts. Exit code 3
can accompany a focused pass or a blocked result. G1 acceptance separately
requires unavailable producer sources or an accepted access-verification
method. A separate work folder under the same account leaves that requirement
open when producer files are still readable.

### Manual local SDK rehearsal

Use the [SDK rehearsal command](../scripts/release/rehearse_sdk.py) on Windows
with the supplied `pixie-manual-sdk-rehearsal/1` input. It names a public SDK
with its paired exporter, the SDK catalog hash, and SHA-256 pins for the
producer composition, exporter seal and exporter entry. Keep the SDK, input
and receipts in their declared locations. Choose a new output directory on
a local fixed drive with an existing parent, separate from those inputs.

```powershell
python scripts/release/rehearse_sdk.py --inputs "C:\qualification\sdk-rehearsal-inputs.json" --out "C:\qualification\sdk-rehearsal"
```

The command writes a local SDK ZIP, a relocated SDK, logs and
`artifact-receipt.json`. It verifies the archived payloads and queries the
paired exporter from the relocated tree. Keep the archive and receipt
together. This is a manual Engine SDK rehearsal. Editor release rehearsal,
source-unavailable qualification, G6 acceptance and publication have
separate evidence or authorization.

### Prepare the Engine SDK and Runtime release

From a reviewed clean Windows x64 checkout, open an x64 Visual Studio developer
shell with CMake, Ninja and Python 3.12 available. Choose a fresh absolute output
directory outside the checkout.

```powershell
python -B scripts/release/prepare_release.py --out "C:\qualification\pixie-engine-candidate"
```

The producer builds the public `authoring/Debug`, `authoring/Release` and
`shipping/Release` SDK units in separate profile build and binary directories.
The SDK ZIP retains the paired exporter and shipping inputs required by export.
The Runtime ZIP contains the matching `shipping/Release` player, DLLs, notices,
provenance and runtime manifest. Its generic player runs an existing no-code
project folder. A code export supplies its own game executable.
Both ZIPs are re-extracted and verified before `assets/` receives the checksums
and release receipt. Debug authoring requires the matching developer CRT.
The standalone PixieEditor checkout's `docs/development.md` describes the
consumer rehearsal. It takes the Engine workflow artifact and the trusted SDK
archive and catalog hashes, with the locked Inspector pair as a separate input.

After the [manual release workflow](../.github/workflows/publish.yml) is present
on the default branch, dispatch it to run Engine CI and upload the verified
candidates as a workflow artifact. The default dispatch stops there. To prepare
a GitHub Release later, dispatch with `draft_release` enabled from an existing
`v<version>` tag whose commit contains the workflow. The workflow checks that
the tag points to the packaged commit and attaches those exact archives to an
unpublished draft. Review the draft and its checksums before publishing it in
GitHub. Tag creation, dispatch, and publication are separate owner actions.
Archive verification and identity checks are distinct from G6 acceptance.

### Separation contract ownership

| Contract | Authority and stable identity | Consumer reference and check |
| --- | --- | --- |
| SDK compatibility, native bindings, runtime and export | PixieEngine owns [ABI28](modules/capi/REFERENCE.md), `sdk.json`, profile manifests and the release receipt. The catalog SHA-256 and profile build IDs bind the installed inputs. | The Editor takes the installed SDK and catalog hash. The SDK verifier and both candidate rehearsals check the identities and inventory. |
| Editor application, SDK selection and release | PixieEditor owns its README, `docs/development.md`, `version.json`, selected-package lock and native provenance manifest. Its candidate binds these inputs through `editor.json` and the ZIP digest. | The Engine guide names the standalone guide by project and path. Editor links, locked restore and publish checks cover its own checkout. |
| Inspector packages and contained Lib | PixieInspector and PixieLib each own their repository README, dependency record and product version. The Editor records its selected package identities and content hashes in its lock. Engine owns the inventory and notices for the native Lib snapshot it ships. | The rehearsal verifies the Inspector pair against the Editor lock. The installed SDK verifier checks the contained native inventory. |

Keep relative links within their owning repository. Use project and path for
the local Editor guide while it has no remote, and use package or catalog
identities at product boundaries. The Engine documentation validator checks
its own links and scheduler. The standalone guide links are checked against
the Editor root.

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
  `tools/pixie-mcp*`, the native probe in `tests/devchannel/`, or the Editor's channel also passes
  the [explicit-input MCP qualification](../editor/README.md#local-development-mcp-channel)
  for each affected SDK configuration. The driver verifies and runs declared
  artifacts and fixtures outside CTest.
- **Documentation:** `pwsh -File ./checkdocs.ps1` reports no problem, and
  the policy's [self-review](governance/documentation-policy.md#self-review)
  passes for what the script does not check. The script checks relative links
  and anchors, metadata fields and their vocabulary, the cross-links and task
  IDs of `TODO.md` and `TASKLIST.md`, and the archive router; it needs Python 3.
- **Whitespace:** `git diff --check` is clean, outside `external/`.

## Continuous integration

GitHub Actions runs `.github/workflows/ci.yml` on every push to `main`, on pull
requests, and by hand:

- **Documentation** (Linux): `pwsh -File ./checkdocs.ps1`.
- **Engine** (Windows): the `ci` preset, Ninja Multi-Config with the compiler of
  the Visual Studio the runner has (the workflow finds it with `vswhere`, enters
  its developer environment, and logs its version), built in Debug and Release
  and tested with `ctest --preset ci-debug` and `ci-release`. The runners have
  no GPU, so Mesa's software renderer, llvmpipe (a pinned release of
  mesa-dist-win, checked by SHA-256), is placed next to the tests for the
  OpenGL 4.5 core context they open; the log shows the renderer they get.

PixieEditor runs its Windows build and publish workflow in its own repository.
It downloads an explicitly selected public SDK artifact and checks its catalog hash.

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

The workspace owns project profiles outside the VAW source repository. The
active Pixie family contains Engine, Editor, Inspector and Lib at profile
revision 4; product code and local project contracts remain independently owned.
The Editor's `vaw-local` remote is a local binding identity, not publication.

Use the activated workspace launcher from the project root in PowerShell 7:

```powershell
& ../../.governance/vaw.ps1 -Action resolve-project-binding -Arguments @{ RepositoryPath = (Get-Location).Path }
& ../../.governance/vaw.ps1 -Action resolve-workplace-binding -Arguments @{ WorkplacePath = (Resolve-Path ..).Path }
```

The launcher selects external profiles, the explicit registry, and the installed
package set. Require `bound` for the project and `workplace_bound` for the family.
Its activation receipt records the platform location, exact pins, packages and
shared-policy hash. The shared policy is materialized from the workspace's
versioned configuration; runtime copies and coordination records are local.

Do not use an old VAW checkout's default profile directory after rebind. A
binding grants no publication authority. Standalone project build and validation
still work without VAW. Never run `git clean -x` on the enclosing VAW checkout:
its ignored `projects/` contains independent repositories and unpushed work.

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
