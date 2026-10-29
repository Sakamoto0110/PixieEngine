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

Export is being implemented under
[PIXIE-TODO-051](../TODO.md#pixie-todo-051--shared-engine-game-export-and-local-mcp-development-channel).
Windows boids and no-code sprites exports passed the 23-case native qualification,
including isolated launches, observed app-local DLL loading and four-barrier
sensitivity/restoration cases. Three local editor publish checks also passed:
development publication rejected, fresh shipping publication accepted with
development types absent, and reuse of an existing binary rejected. The editor's
real `--do export` route exported sprites through the native CLI successfully;
the final regression matrix remains pending. This guide
does not claim a finished distribution guarantee or publication.

`tools/export/` owns the native command and verifier; CMake owns shipping
configuration and product metadata. Qualification currently covers Windows
PE64 only. ELF/native export on another platform is not qualified.

## Command and build separation

```text
pixie-export <project> <out>
pixie-export --verify <manifest.json> <out>
pixie-export --from-manifest <manifest.json> <out>
```

The normal command prepares Release products beneath the project's
`build/pixie-export`, with a separate native output root. It requires
`PIXIE_DEVELOPMENT=OFF`, `PIXIE_DEV_CHANNEL=OFF` and tools/tests/sample builds
disabled for that product. It does not collect whatever happens to be in the
development `bin/Release`. Old ignored per-game C API libraries are not inputs.

The player uses its local project before any development source-folder
fallback; a shipping player must not rely on an absolute project path on the
build machine. The code project is linked directly into its player, while a
no-code project uses the generic player. No game module belongs in an export.

## Concrete file plan

`cmake/PixieExportProduct.cmake` records product paths/configuration. The native
tool freezes a versioned `pixie-export` manifest: each entry has a source,
relative destination, role and SHA-256, with one player and the shared engine.
That working manifest remains in build storage; its absolute build paths are
not copied into the portable game folder as a runtime dependency.

Inputs are the player, shared engine, native dependency closure, release
Visual C++ runtime bundle, project document, scenes and assets. Native imports,
delay imports, exports and forwarders are inspected without loading the images.
An explicit OS-library policy is separate from app-local CRT requirements;
finding a third-party DLL in the development machine's System32 is not enough
to classify it as a Windows dependency.

Scene asset references are discovered through native built-in descriptors.
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

The editor Export command invokes the same native implementation. Its real
`--do export` route completed a sprites export and waited for the job before
timed exit. Static inspection connects the File menu and command route to the
same ExportPanel/ProjectExportJob; no interactive menu gesture is claimed by
that command test. MCP and deterministic capture are separate pending
increments; their later code must be included in exclusion tests before any
shipping/publish/remote-release guarantee is stated. Save-over-channel remains
disabled, and no golden reference image has been approved.

See the [qualification matrix](shared-engine-export-validation.md) for actual
results, working-tree/code revision, failures, sensitivity/restoration and gaps.
