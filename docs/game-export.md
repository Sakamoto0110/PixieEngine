# Native Game Export

**Document ID:** GLOBAL-GAME-EXPORT

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** manifest-based native folder export and bounded Windows isolation qualification

**Surface:** guide

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## Status and source

Export was delivered under
[PIXIE-TODO-051](archive/todo-051-shared-engine-export-mcp-2026-11-01.md#pixie-todo-051--shared-engine-game-export-and-local-mcp-development-channel).
Windows boids and no-code sprites exports passed the 33-case native qualification,
including isolated launches, observed app-local DLL loading and four-barrier
sensitivity/restoration cases. Three local editor publish checks also passed:
development publication rejected, fresh shipping publication accepted with
development types absent, and reuse of an existing binary rejected. The editor's
real `--do export` route exported sprites through the native CLI successfully;
the current qualification and closure are recorded in the
[qualification guide](shared-engine-export-validation.md). The owner-selected
light-lab code project also exported, verified and ran in an isolated output folder.
These historical results cover the Windows distribution paths exercised by that
baseline. Publication and other platforms have separate acceptance criteria.

`tools/export/` owns the native command and verifier; CMake owns shipping
configuration and product metadata. Qualification currently covers Windows
PE64 only. ELF/native export on another platform is not qualified.

## Command and build separation

Use the exporter at `tools/export/Release/pixie-export.exe` in the selected
[public Engine SDK](development.md#installed-sdk-and-local-rehearsal). Keep it
beside its paired DLLs inside that SDK. Every mode requires `--sdk <root>`.
The root must contain `sdk.json`, its declared exporter and the matching
`shipping/Release` tuple. `--identity` reports the selected SDK and exporter
identities before a project is opened.

Check the composed SDK's `sdk.json` for `features.pairedExport: true` and a
`tools.exporter` descriptor for that shipping tuple. Profile manifests keep
`pairedExportAvailable: false` from individual unit assembly. The release
producer stages the paired tool after composition and records it in the
root catalog. The exporter checks that descriptor and the selected tool
and runtime files against the shipping tuple.

```text
pixie-export --sdk <root> --identity
pixie-export --sdk <root> [--work <directory>] <project> <out>
pixie-export --sdk <root> --verify <manifest.json> <out>
pixie-export --sdk <root> --from-manifest <manifest.json> <out>
```

Use a saved project folder containing its `.pixie` document. A scene-only
export uses the selected SDK's generic player and app-local runtime and
requires no native build. The exporter validates `.pixie-code/sdk.json` when
present. A project with a `CMakeLists.txt` requires that file to declare
a compatible portable public authoring requirement. It also
requires consumer CMake and the matching Windows x64 MSVC/Windows SDK
toolchain. CMake is found in absolute directories on `PATH` or at the
absolute `cmake.exe` path supplied by `PIXIE_EXPORT_CMAKE`.

Code export configures a separate installed-package `shipping/Release`
build with `PIXIE_DEVELOPMENT=OFF` and `PIXIE_DEV_CHANNEL=OFF`. The SDK owns the
prebuilt engine and dependency closure. A fresh configure must declare exactly
one shipping player target. The exporter builds that target and takes its
executable from the owned `bin/Release` under the work root. Development
build outputs and game-module DLLs are excluded from the file plan.

### Work directory and diagnostics

The default work directory is
`<project>/.local/pixie-export/shipping/<sdk.json SHA-256>`. Supply
`--work <directory>` only for a normal export, for example to shorten paths or
use a separate cache. Use a dedicated work directory outside the destination.
The exporter takes an exclusive work-directory lock. For code projects,
`build/pixie-export-owner.json` binds the build and binary directories to the
project path, selected SDK root/catalog/build ID, shipping role, Release
configuration and CMake path/hash. An unowned populated build, unowned binary
directory or mismatched owner record requires a fresh work directory.
The tool archives prior product declarations under
`build/previous-export-products/` before each code configure.

Read the command's stdout/stderr for configure/build failures and paths. The
fresh code product declaration is in `build/pixie-export/Release/` under the
work root. A scene-only export writes `pixie-export/Release/no-code.json`
under the work root. Both flows write `export-manifest.json` there and print
its path after copying and verification.

### Launch the exported player

Launch the executable from the exported folder or pass `--project <folder>`
explicitly. Without that option, the player checks beside the executable
and then its ancestors for a `.pixie` file. The compile-time source-folder
fallback is limited to applicable development builds. A code export links
game code directly into its player. Scene-only exports use `pixie-player.exe`.

## Concrete file plan

`cmake/PixieExportProduct.cmake` records product paths/configuration. The native
tool freezes a versioned `pixie-export` manifest with SDK and shipping
provenance. Each entry records its source, relative destination, role and
SHA-256. The plan includes one player and one shared engine. Keep the
manifest in the work directory. The portable folder contains the manifest's
payloads, while `--verify` and `--from-manifest` use the working manifest and
matching SDK. Replaying a manifest requires the original source files to
remain accessible with matching hashes.

Inputs are the player, shared engine, native dependency closure, release
Visual C++ runtime bundle, project document, scenes and assets. Native imports,
delay imports, exports and forwarders are inspected without loading the images.
An explicit OS-library policy is separate from app-local CRT requirements;
finding a third-party DLL in the development machine's System32 is not enough
to classify it as a Windows dependency.

Scene asset references are discovered through native built-in descriptors.
An atlas-backed `Sprite` includes its `.atlas.json` descriptor and the descriptor's PNG.
The exporter checks the referenced frame and decoded PNG dimensions before copying, and checks
the atlas dependencies again during manifest replay and verification. The direct image field
supplies the dependency for direct PNG sprites.
Game-specific component assets and files opened directly by code need explicit
`pixie_game(... ASSETS <project-relative-file>...)` declarations. There is no
recursive copy of all project/source/build files. Extra code-owned assets do
not authorize development executables or modules as assets.

The destination must be empty or absent. Project/ancestor replacement, path
escape, conflicting file destinations and reparse-point traversal are rejected.
Only manifest entries are copied, then output content/hashes, binary exclusions
and dependency closure are checked. A changed source after planning or an extra
output file invalidates verification. Executed negative cases cover development
configuration/binaries, module-loader code, unlisted files/directories, missing
GLFW/CRT/forwarder dependencies, path traversal and an original output junction.
Restored clean boids and sprites bundles passed verification afterward. The
explicit OS-library classification policy was reviewed statically; this run
did not install a third-party DLL into System32 to test that distinction.

## Delivery boundaries

The Editor's **Export game** dialog invokes the native exporter for a
saved project in Edit mode. Its **Build log** shows recent stdout and stderr. The historical `--do export` test
completed a sprites export and waited for the job before timed exit. Static
inspection linked the File menu and command route to the same
`ExportPanel`/`ProjectExportJob`. The test exercised the command route.
The current public shipping profile excludes the development channel. See
the [profile and host contract](modules/capi/REFERENCE.md#sdk-identity-and-profiles)
for development transport, capture and save boundaries. The historical
export run did not approve a golden reference image. Publication and other
host/transport qualification use their own receipts.

See the [qualification matrix](shared-engine-export-validation.md) for actual
results, working-tree/code revision, failures, sensitivity/restoration and gaps.
