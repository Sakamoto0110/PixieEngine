# Extract PixieEditor into an independent SDK consumer

**Document ID:** PIXIE-PROPOSAL-022

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** exact Editor extraction scope, path ownership, relocation qualification, build and distribution boundaries

**Surface:** proposal

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## 1. State, purpose and owner decisions

**State:** promoted by the owner on 2026-11-05 under
[PIXIE-TODO-060](../../TODO.md#pixie-todo-060--separate-engine-sdk-game-runtime-and-editor-delivery).
This document preserves the extraction design and its pre-move inventory.
The [execution record](../evidence/sdk-editor-separation-2026-11-06.md#proposal-022--standalone-editor-extraction)
retains scoped clearances; the [milestone record](../evidence/sdk-editor-separation-2026-11-06.md#milestones-and-closure)
records the applied local cutover and its evidence limits. Current task state
belongs to [TASKLIST](../../TASKLIST.md#pixie-todo-060--engine-sdk-game-runtime-and-editor-separation).
The original execution gates below describe the reviewed plan, not an instruction
to repeat extraction. Publication requires separate authorization.

The objective is to place PixieEditor in its own project/repository, make it build and operate against installed Engine SDK artifacts, and leave PixieEngine independently buildable and distributable. A sibling folder is the owner's preferred workplace arrangement, not a dependency-resolution contract.

From the Engine checkout, the confirmed initial destination is:

`../PixieEditor`

The Engine checkout is the reference root:

`.`

The following owner decisions govern this proposal:

1. The Engine SDK already exists. Preserve its delivered capabilities; do not restart SDK design as a prerequisite to extraction.
2. Extract the existing Editor with its behavior, history, authoring, recovery, code integration and export functionality intact.
3. Engine SDK/runtime distributions should become publishable through GitHub Releases. The independent Editor should eventually obtain the correct distribution from those releases.
4. Inspector and Lib already have their own release arrangements. Moving Editor is not authorization to modify or republish them.
5. Engine and Editor are to use the same native GLFW version. Detailed investigation of loaded GLFW instances and shared native state is postponed. Matching package labels alone is not proof of matching native versions.
6. The future virtualization compatibility target is **OpenGL 4.3**, correcting the earlier 4.1 discussion. The owner selected 4.3 for the intended VMware environment. This records a future target, not a claim that every VMware product, guest, driver or setting supports it.
7. OpenGL 4.3 adaptation and VM qualification occur after extraction. This extraction preserves the current OpenGL 4.5 baseline and does not modify the renderer to meet the future target.
8. TASKLIST/VAW records are known to be stale. They are historical leads, not evidence of current readiness, destination or completion. Updating or installing VAW is not a prerequisite.
9. Correct path resolution is a release condition. The Editor must not accidentally work only because it remains next to Engine, launches from one directory, or reuses an old cache.
10. This proposal retains the detailed reviewed plan. Future implementation requires a reviewed execution baseline and explicit authorization for source removal; drafting does not grant either.

This proposal details the extraction phase of
[Engine SDK and Editor separation](engine-sdk-editor-separation.md).
The confirmed destination and future OpenGL target supersede earlier planning
assumptions for that phase.

## 2. Evidence, baseline and limits

The first sweep examined Editor architecture. Subsequent read-only sweeps covered the three local repositories, followed by an Engine/Editor-focused check of paths, CMake, build inputs, packaging, tests and publication guards.

The observed application tree contains 64 source/project/asset files under `editor/PixieEditor/`. Of these, 63 accompany Editor; `Native/PixieCapi.cs` remains Engine-owned at a new producer path. The observed `tests/editor/` tree contains 22 files: 21 accompany Editor and one native fixture remains Engine-owned. IDE state and generated build output are excluded from these counts.

The exact 94 source entries are recorded in [the inventory](standalone-editor-extraction-inventory.json) and reproduced in Appendix A: 92 transfer/content-transfer entries and two relocations inside Engine. The inventory contains source byte counts and SHA-256 values from proposal preparation. Those hashes are observations, not a future execution seal. Before implementation, refresh the inventory against the actual working trees and reconcile every difference without overwriting another writer's work.

Earlier successful status observations found substantial modified and untracked work in Engine, a clean Inspector checkout on `codex/edit-chains-locks`, and a clean Lib checkout on `main`. A later attempt to refresh Engine status returned `this operation must be run in a work tree`, including an explicit work-tree invocation. Files remained readable. This proposal does not diagnose that Git failure or claim that Git status was revalidated. Execution must first establish the actual repository/work-tree context, HEAD, index and dirty/untracked state.

No product configure, build, restore, test, runtime launch or publication was performed for this proposal. Documentation checks and inventory checks demonstrate the proposal's consistency only. Cached `obj` results and old qualification records are not proof of a fresh independent build or current runtime acceptance.

Important inspected owners:

| Finding | Owning source | Consequence |
|---|---|---|
| Editor imports Engine-local metadata, a source header, local adoption settings and Engine version | [Editor project](../../editor/README.md#source-provenance) (`editor/PixieEditor/PixieEditor.csproj`) | Replace producer-relative build inputs and establish Editor version ownership |
| SDK inventory and installation read a binding from inside Editor | [SDK producer](../../cmake/sdk/PixieSdk.cmake) | Relocate the canonical source before deleting Editor's source tree |
| Public packaging strips development-only binding declarations | [SDK assembly](../../scripts/sdk/assemble_sdk.py) | Select matching public/development binding inputs for each managed build |
| Native fixture is registered in Engine CMake despite its Editor directory | [Native tests](../../tests/CMakeLists.txt) | Relocate it within Engine and preserve its target and artifact role |
| Installed project build plans already bind exact SDK/cache/toolchain inputs | [ProjectBuild](../../editor/README.md#source-provenance) (`editor/PixieEditor/Native/ProjectBuild.cs`) | Preserve identity checks and remove implicit source-location assumptions |
| Source helpers and some tests still search Engine ancestors | [ProjectCode](../../editor/README.md#source-provenance) (`editor/PixieEditor/Native/ProjectCode.cs`), [workspace tests](../../editor/README.md#source-provenance) (`tests/editor/code-components/CodeWorkspaceTests.cs`) | Replace discovery with admitted SDK inputs or explicitly supplied fixture inputs |
| Relaunch already carries explicit SDK identities | [EditorLaunch](../../editor/README.md#source-provenance) (`editor/PixieEditor/Native/EditorLaunch.cs`) | Preserve continuity while making launch paths independent of working directory |
| Editor publication removes automation while keeping export UI | [Publish qualification](../../editor/README.md#source-provenance) (`tests/export/qualify_editor_publish.py`) | Preserve capability separation and audit the destination artifact |
| Capture script assumes a game name rather than an explicit installed selection | [Capture qualification](../../editor/README.md#source-provenance) (`tests/capture/qualify_editor_capture.py`) | Adapt the test invocation rather than relying on ancestor layout |

## 3. Product ownership after extraction

| Responsibility | Owner | Boundary |
|---|---|---|
| Window, panels, selection, Inspector integration, authoring, history and binding recovery | PixieEditor | Managed application policy remains together |
| EngineSession, SDK selection, code-build coordination, VS Code integration and export UI | PixieEditor | These are consumer/application responsibilities, not a new engine layer |
| Runtime, C API, public ABI declarations and canonical C# interop source | PixieEngine | Delivered as SDK/runtime artifacts |
| Project generators, template versions, native build helpers and exporter implementation | PixieEngine | Editor invokes installed SDK tools; it does not copy their implementation |
| Inspector core/view and Lib implementations | Their existing repositories | Consumed through explicitly selected versions; local source is optional development input |
| Native test probes and native contract tests | PixieEngine | Built by Engine and passed to consumers as explicit artifacts |
| Editor-only tests and publication checks | PixieEditor | Own their managed build and application behavior coverage |
| Existing player/Editor MCP cross-product qualification | Engine integration tooling for this increment | Receives exact external artifacts; no source dependency on the new Editor tree |
| Future GitHub download/install/cache of Engine versions | PixieEditor distribution acquisition layer | Produces an installed SDK selection consumed by the existing loader |

The extraction must not make Engine depend on an Editor checkout, installed Editor, Inspector checkout or adjacent Lib checkout for ordinary native builds or SDK production. Cross-product integration tests can require Editor artifacts explicitly; that is different from a producer build dependency.

Preserve application assembly names, namespaces, internal member names used by tests, resource logical names and public/native schemas unless a separately reviewed technical necessity is demonstrated. In particular, do not rename `PixieEditor.Native` merely because the Engine-owned binding source moves. Do not redesign the internal UI/Editing/Native dependency graph during this operation.

## 4. Exact transfer and retention rules

### 4.1 Files transferred into PixieEditor

Appendix A lists every source and destination. The grouping rules are:

| Source in Engine | Destination in Editor | Entries |
|---|---|---:|
| `editor/PixieEditor/`, excluding `Native/PixieCapi.cs` | `PixieEditor/`, preserving each suffix | 63 |
| `editor/PixieEditor.slnx` | `PixieEditor.slnx` | 1 |
| `editor/README.md` | `README.md`, adapted to installed consumption | 1 |
| `tests/editor/`, excluding `channel/NativeWriteProbe.cpp` | `tests/`, preserving each suffix | 21 |
| `tests/export/qualify_editor_publish.py` | `tests/publish/qualify_editor_publish.py` | 1 |
| `tests/export/InspectEditorPublishIl.cs` | `tests/publish/InspectEditorPublishIl.cs` | 1 |
| `tests/export/packages.lock.json` | `tests/publish/packages.lock.json` | 1 |
| `tests/capture/qualify_editor_capture.py` | `tests/capture/qualify_editor_capture.py` | 1 |
| `docs/editor-field-interactions.md` | `docs/editor-field-interactions.md` | 1 |
| `docs/editor-movement.md` | `docs/editor-movement.md` | 1 |

Guide transfers move current explanatory ownership. They may leave pointer-only migration notes at the old paths; they do not create two mutable normative copies. Native APIs and sample content referenced by those guides remain Engine-owned. Every migrated guide must have its native links and examples reconciled.

The export panel and job remain in the transferred `Development/` directory initially, including their current namespaces. That directory name is not a packaging rule: those two files are included in the normal Editor while automation files are excluded. Preserve the explicit project conditions.

### 4.2 Files relocated within Engine

| Current source | New Engine path | Installed/public path |
|---|---|---|
| `editor/PixieEditor/Native/PixieCapi.cs` | `capi/bindings/dotnet/PixieCapi.cs` | Keep `bindings/dotnet/PixieCapi.cs` in SDK packages |
| `tests/editor/channel/NativeWriteProbe.cpp` | `tests/devchannel/NativeWriteProbe.cpp` | Preserve the existing native target/artifact identity unless explicitly revised |

`tools/NativeApiVersion.cs` and `tools/NativeApiVersion.props` remain at their current Engine producer paths in this increment. They are also used by MCP. Their installed SDK locations remain `bindings/dotnet/NativeApiVersion.cs` and `bindings/dotnet/NativeApiVersion.props`. Moving those producer files as cleanup is unnecessary.

### 4.3 Areas retained in Engine

Retain `engine/`, `runtime/`, the remaining `capi/`, `devchannel/`, `cmake/`, `templates/project-code/`, `external/`, `samples/`, `tools/export/`, `scripts/sdk/`, `scripts/release/`, `tools/pixie-mcp/`, `tools/pixie-mcp.tests/`, `tests/mcp/`, `tests/separation/`, `tests/sdk-consumers/`, and other native/generation/export tests. Only the exact listed Editor-owned files leave their current test directories.

Retain Engine product version ownership, native dependencies, notices, historical records, existing proposals and prior validation evidence. `docs/entity-input-and-code-components.md` retains native input and generation contracts; extract the Editor operating workflow into a new Editor guide and replace overlapping instructions with references. Do not erase historical contracts or imply that historical test commands ran at the destination.

### 4.4 New destination-owned files

| Proposed Editor path | Purpose and constraint |
|---|---|
| `version.json` | Independent Editor product version; initial value is recorded before the baseline build, not inferred from the selected Engine at runtime |
| `.gitignore` | Ignore IDE state, managed outputs, local overrides, caches and layout; do not ignore source fixtures |
| `.gitattributes` | Preserve agreed line-ending and binary rules without normalizing unrelated content during transfer |
| `LICENSE` | Preserve applicable original copyright and license terms; not a new licensing decision |
| `global.json` | Pin an available, accepted .NET build SDK and explicit roll-forward policy |
| `NuGet.Config` | Explicit package feeds; public restore must not depend silently on a machine-local feed |
| `DEPENDENCIES.md` | Inspector/core/view, Lib, Hexa and native asset selections with provenance |
| `eng/EngineSdk.props` | Installed SDK build import and validation; no ancestor search or sibling fallback |
| `docs/development.md` | Independent build, test, local-source and publication workflows |
| `docs/code-authoring.md` | Editor-specific component/build/reopen/IDE workflow |
| `docs/extraction-origin.md` | Import anchor, source identity, dirty-content provenance and transfer correspondence |
| `.github/workflows/ci.yml` | Independent Editor build and scoped verification over explicit artifacts |
| `.github/workflows/publish.yml` | Independently prepared Editor publication workflow; execution remains separately authorized |

Generate/review dependency lock files for the selected projects during implementation. The transferred `tests/publish/packages.lock.json` belongs to its existing file-based audit; it is not a substitute for the application restore lock. Optional `.local/PixieEditor.props` is recreated as machine-local configuration, with an explicit Inspector root, never copied with the old relative path untouched.

### 4.5 Files and state that do not transfer as product source

Do not import `.git` wholesale, `.vs`, `bin`, `obj`, CMake caches, generated root solutions, `.local` wholesale, user settings, temporary SDK bundles, qualification outputs, or `imgui.ini` as a versioned default. Do not import private credentials or machine-specific package-source authentication. Preserve existing user data where it lives; exclusion from the new repository is not authorization to delete it.

The copied icon/chain/lock assets keep their current names and provenance. No asset replacement, conversion or shared resource-store modification is part of extraction.

## 5. Location independence is an acceptance contract

The requirement is stronger than fixing relative paths for the first destination. Every path-bearing operation must have a declared owner, resolution base and relocation behavior. Successful execution in the confirmed sibling layout is insufficient evidence.

For supported locations, moving a repository or installed artifact and selecting its new location must lead to the same intended inputs and behavior. If persistent state points to an old location, the product must request a new selection or regenerate disposable state explicitly. It must not silently choose a different Engine, reuse an incompatible build, overwrite user configuration, or reconstruct the old source-tree layout.

This does not promise that absolute filesystem references remain valid after arbitrary external moves, that CMake caches are relocatable, or that a loaded native library can move while in use. The guarantee is correct rebasing, re-selection or a clear refusal with preserved data. Test supported Windows paths with spaces and Unicode. Unsupported locations such as disallowed reparse paths must fail deliberately under the existing containment rules; do not relax those rules to make relocation tests pass.

### 5.1 Resolution rules by owner

| Path-bearing input | Authoritative base | Required behavior |
|---|---|---|
| Editor project imports, version and assets at build time | Their owning project/props file via `MSBuildThisFileDirectory` | Work from any shell directory; never use the Engine repository as an implicit base |
| Editor build SDK | Explicit absolute `PixieBuildSdkRoot` (proposed property) | Validate installed manifest, binding and metadata before compilation; reject missing/mixed inputs |
| Runtime SDK | Explicit admitted selection from CLI/UI/settings | Normalize once; preserve identity checks; build SDK selection must not overwrite runtime selection |
| SDK manifest payloads | Validated SDK root | Resolve declared relative paths inside that root and verify inventory, hashes and profile compatibility |
| Native runtime and game module | Admitted SDK runtime path and explicit module path | Retain loader identity checks; no module-adjacent or ancestor Engine substitution |
| Editor relaunch | Current explicit executable or `dotnet` plus explicit assembly | Preserve SDK identity, profile, configuration, project and handoff arguments |
| Game project | Explicit normalized project directory | Project-relative files remain within their declared owner; a project does not store an Engine checkout path |
| Generated project CMake | Selected installed SDK and generator manifest | Remain in PACKAGE mode with the exact SDK package configuration |
| Build/export cache | Declared temporary/cache base plus ownership key | Validate ownership; create a fresh cache when identity/path inputs change; never rebase cached absolute paths by string replacement |
| CMake executable | Explicit validated discovery result | Preserve the absolute tool path and diagnostics; never rely on a misleading executable in the current directory |
| VS Code workspace integration | User project, owned build plan and validated CMake cache | Generate/check exact paths; preserve user settings and backups on conflicts |
| Inspector local source | Explicit development-only root | No mandatory sibling layout; excluded from public publication |
| NuGet packages | Declared feeds and dependency locks | Clean restore without an ambient local project reference or accidental package version change |
| Inspector images | Editor application base directory | `assets/inspector/chain.png` and `lock.png` copied to build/publish outputs when the feature is enabled |
| Window icon | Preserved embedded resource identity | Repository or working-directory movement does not affect loading |
| Editor user settings | Existing application-data location | Preserve recent projects and SDK preferences; stale locations are diagnosed and reselected |
| Dock layout (`imgui.ini`) | Explicit application-owned layout path | Stop accidentally selecting another layout because the launch working directory changed; see migration below |
| Rebuild/reopen handoff | Project-owned `.local/editor-reopen/` | Preserve validation and state semantics; never redirect to another project or old repository |
| Capture/log/qualification output | Explicit writable output root | Independent of source and SDK roots; no accidental writes into installed packages |
| Native test fixtures and samples | Explicit input artifact/project roots | No construction of fake Engine ancestors and no implicit `samples/` discovery |
| Documentation links | Owning repository or established product documentation URL | No required `../PixieEngine` or `../PixieEditor` links between repositories |

### 5.2 Build SDK and execution SDK are separate selections

`eng/EngineSdk.props` is the proposed single Editor build integration point. It imports the selected SDK's `bindings/dotnet/NativeApiVersion.props` and compiles its canonical `bindings/dotnet/PixieCapi.cs` as an external source input without editing the SDK. There must be exactly one binding compilation, not a retained local copy plus an imported duplicate.

The build validates existence and compatibility before a missing file becomes an opaque compiler error. It records which SDK/binding contract and dependencies produced the application. No fallback to `capi/include/pixie_capi.h`, `tools/NativeApiVersion.props`, `../PixieEngine`, a current directory, or an ancestor scan is permitted in the independent application build.

The execution SDK remains chosen through the current admission mechanism. Compiling against one SDK contract is not permission to load arbitrary newer or older binaries. Retain ABI, capability, identity and C++ module compatibility checks. Multiple SDK installations may coexist, but a process must not switch an already-bound native runtime contrary to the existing lifecycle rules.

### 5.3 Public and development builds

The public SDK strips `#if PIXIE_DEVELOPMENT` binding declarations. Therefore changing only the import path while retaining the current development default can produce an invalid combination. Define and document explicit build profiles:

| Editor build | SDK binding inputs | Publication and execution boundary |
|---|---|---|
| Normal/public application | Public installed binding and compatible authoring capability | Export UI remains available; no automation endpoint; current public SDK admission restrictions remain |
| Development/qualification application | Installed binding carrying the needed private automation declarations | Explicit development selection; separate outputs; not published as the normal application |
| Save-disabled qualification variant | Matching development inputs with `PixieChannelSave=false` | Verify the existing negative save behavior without changing normal export functionality |

Do not infer the capability profile from Debug versus Release. Keep separate intermediate/output directories for materially different SDK/binding/profile/Inspector selections. A default `dotnet build` must either use a documented explicit selection or fail with a useful missing-selection diagnostic; it must not discover a source checkout to make the build succeed.

Preserve publication guards against local Inspector integration, development operations and publishing an arbitrary existing binary with `NoBuild`. Adapt the publication qualification script to pass the explicit build SDK and selected package configuration for every positive and negative case.

### 5.4 Project CMake, code generation and legacy tests

Preserve the installed build plan's explicit `PIXIE_ENGINE_MODE=PACKAGE`, SDK root/profile, output root, target, configuration, Windows SDK selection and `PixieEngine_DIR` resolved from the selected distribution. Do not introduce `add_subdirectory` of Engine into the Editor repository or generated game project. Do not copy the Engine build system into Editor.

`ProjectBuild`, `ProjectCode`, `CodeWorkspace` and `CodeEditor` currently contain explicit source-support paths, and some tests call `EngineFolder.Find()`. Classify every caller before changing it:

1. Application paths use installed support supplied by the admitted SDK.
2. Tests of installed behavior use the same installed path as the application.
3. Tests that deliberately verify legacy source/generation behavior receive explicit fixture/template inputs or stay in an Engine-owned source qualification harness. They must not depend on finding Engine above the test executable.
4. Preserve refusal and user-file-preservation cases. A test may be adapted or have its source-specific subcase reassigned, but no existing case disappears merely because its old location no longer works.

The source-discovery helper can be retired only after each caller and coverage case has an explicit disposition. A separate broad legacy-support redesign is not required. If preserving a case needs a new fixture asset beyond Appendix A, add that exact asset and owner to the reviewed execution inventory before implementation.

Do not reuse an old CMake cache after moving the project or SDK. The current key includes project path, SDK root, manifest, build ID, role and configuration; preserve that distinction. Verify the cache's ownership file, compiler/toolchain and declared output instead of assuming that a file in `bin/` is current.

### 5.5 Relaunch, user projects and editor state

Preserve `EditorLaunch` selection flags, the wait-for-parent protocol and `CodeReopen` state transfer: unsaved authored snapshot, selection/anchor, World/Scene target, camera, Environment and pending component attachment. Do not add a promise of history transfer that the existing reopen protocol does not provide; history behavior must match the observed baseline.

The child must launch from the relocated executable/assembly and receive the same admitted SDK identity. The handoff must still name the intended project and pass containment checks. Moving an application installation while its process or child still uses native libraries is unsupported; close it, move it, then launch the new location.

Moving a user project preserves its authored files, relative asset references and portable SDK requirements. Local caches and generated IDE presets may require regeneration. Existing presets or user configuration that point to an old location must cause an actionable conflict or a deliberate owner-approved regeneration, not silent overwriting. A moved project with custom CMake remains subject to the existing supported-template/refusal rules.

Settings retain the existing application-data identity. Do not erase recent projects or SDK preferences during extraction. If a selected SDK or project no longer exists at a remembered path, show that condition and permit explicit relocation/reselection; do not search for a similarly named substitute.

### 5.6 Dock-layout path migration

Current layout persistence uses `imgui.ini` relative to the working directory. This is a concrete relocation issue, not a reason to migrate all user state. The proposed default is an explicit `imgui.ini` path under the same application-data `PixieEditor` directory used by settings, assigned to ImGui with the required managed/native lifetime.

Resolve that location once and use the same absolute path for first-layout detection, legacy-import decisions and ImGui layout reads/writes. The existing `FirstLayout = !File.Exists("imgui.ini")` check must not remain relative after ImGui's persistence path changes. Keep the path storage valid for the full native context lifetime and release it through the matching lifecycle if the binding requires explicit allocation.

For normal use, check a legacy layout at an explicitly identified previous launch location and offer/import it only when the new location has no existing layout. Preserve the original file and do not choose among unrelated `imgui.ini` files by searching ancestors or all disks. If no unambiguous legacy location is known, start with the normal default layout and explain how to import the old one. Never overwrite a destination layout as migration housekeeping.

Qualification must use an explicit isolated layout input/output path, rather than changing the working directory to redirect user state. A development-only layout override or equivalent injected test setting must be included in the execution design and must not bypass the public/private artifact checks. This is a bounded path fix; it does not redesign docking or the Editor UI.

## 6. SDK identity, dependencies and packaging

### 6.1 Canonical binding and package identity

Update both the producer input inventory and the install source in `cmake/sdk/PixieSdk.cmake` when relocating the binding. Preserve the installed binding/metadata paths and existing public/private processing. Verify that SDK production no longer reads anything under the removed `editor/` tree.

Producer identity currently includes source paths and hashes. A source relocation can therefore change SDK identity even when the binding bytes are identical. Generate new manifests and receipts for the resulting artifacts. Do not overwrite a previously qualified package under an existing identity or change project SDK requirements automatically to suppress a mismatch.

Previously produced immutable SDKs remain valid artifacts for their own identity and supported Editor contract. Tests may use them explicitly. A newly produced SDK and its receipts must be labeled as new inputs. Package provenance, source inventory, headers, bindings, native libraries, profile and paired exporter must agree.

The native write-probe fixture keeps its existing target role when its source moves to `tests/devchannel/`. Update only the source registration and any fixture manifests that actually name it. Never put the private test probe into the public runtime or SDK release to simplify testing.

### 6.2 Inspector, Lib and graphical assets

The inspected Editor supports public Inspector 0.3.0, selected core/view pair 0.3.0-pixie021.3, and explicit local source. The latter two enable field-interaction behavior that the default public package path does not. The execution baseline must select the intended feature set and compatible immutable packages before comparing source and destination.

Do not claim preserved behavior by building the destination against a different feature baseline. Keep core and view paired exactly; retain the compatible Hexa.NET.ImGui contract. The current inspected references are Hexa.NET.GLFW 1.0.5, Hexa.NET.ImGui 2.2.9, Hexa.NET.ImGui.Backends 1.0.18 and Hexa.NET.ImGui.Backends.GLFW 1.0.18. These are observed package selections, not a remote latest-version audit or native GLFW version proof.

If the desired feature package is only available locally, record its feed and hashes for development qualification. Public release readiness separately requires an available permitted dependency source. Do not silently fall back to 0.3.0, publish upstream packages, or modify Inspector to conceal an unavailable dependency.

Inspector currently brings managed PixieLib 0.2.0. Engine uses its vendored native Lib. The neighboring standalone Lib checkout does not automatically replace either. Preserve both selections unless an independent compatibility decision changes them. No coordinated Lib/Inspector release or source refactor is introduced by this proposal.

Record the native GLFW version supplied to each side and implement the owner's version alignment through an explicit dependency/staging choice. Do not infer it from the Hexa package version or replace only a managed wrapper. A basic context create/render/interact/dispose check remains required; exhaustive loaded-instance/context ownership qualification is deferred. Record that residual limitation rather than declaring the deferred investigation passed.

### 6.3 Normal Editor payload

The normal Editor package contains its managed application, declared managed/native UI dependencies, permitted assets, license/notices and version/provenance information. It does not silently bundle a source checkout, private automation SDK, test clients, MCP package, arbitrary local files, or an exporter copied from a build directory. Export remains available through the exporter declared by the selected SDK.

Keep `ExportPanel` and `ProjectExportJob` in the normal application. Keep automation exclusion explicit in project compilation and qualification. Review the entire output inventory, not just whether `PixieEditor.exe` exists. The existing IL audit depends on specific type/method names; retain them or update the audit deliberately with equivalent checks.

The SDK and runtime package contents stay owned by Engine. A shipping runtime is not an authoring host. A public Editor must continue to reject an inadmissible private automation distribution under the current rules. Capability names, profile selection and compiler/CRT/architecture/CPU constraints remain contractual inputs.

## 7. Ordered implementation plan

Each stage has an output and a concrete exit condition. The task plan maps
the stages to existing owners and records the gates for execution.

### A. Establish a reliable source baseline

1. Confirm Engine's actual work-tree context, HEAD, index and modified/untracked files. Resolve the status-observation limitation without changing repository history or discarding local work.
2. Reconcile current files against the 94 proposal inventory entries. Include new `EditorLaunch`, `EngineSdk`, `SdkRequirements` and `SdkSelectionTests`; a HEAD-only export would omit delivered local work.
3. Identify active writers of the transfer files, SDK packaging files, shared tests and docs. Coordinate a bounded freeze for the actual snapshot/cutover. Worktrees alone do not capture uncommitted work.
4. Record source hashes, destination mapping, preserved modifications, intended package versions/features, SDK identities and toolchain selections. Record the initial Editor version and import anchor.
5. Capture or identify applicable baseline behavior evidence. Label reused evidence with its original inputs; do not rerun unrelated native suites merely to enlarge the report.

**Output:** a reviewed execution inventory and baseline. **Exit condition:** no unowned source change or ambiguous file disposition; all new files in the transfer scope accounted for. The proposal inventory must be refreshed if anything changed.

### B. Remove producer dependence on the Editor tree

1. Relocate the canonical C# binding to `capi/bindings/dotnet/PixieCapi.cs` inside Engine, preserving its bytes initially.
2. Update `cmake/sdk/PixieSdk.cmake` producer-input and install-source paths. Keep package paths unchanged and keep metadata producers in `tools/`.
3. Relocate `NativeWriteProbe.cpp` to `tests/devchannel/` and update `tests/CMakeLists.txt` while preserving its target role.
4. Ensure transitional source Editor builds have one binding input at the new producer location until the installed-input build is active. Do not leave a second independently editable binding copy.
5. Produce/verify the affected SDK artifacts with new identity receipts where required. Preserve the public/private stripping and fixture exclusions.

**Output:** Engine-owned contract sources independent of `editor/`. **Exit condition:** native configuration/build and SDK packaging no longer require those old paths; payload and profile identity remain coherent. No Editor checkout becomes an Engine build dependency.

### C. Stage the independent Editor

1. Create the confirmed destination only during authorized implementation. Check that it does not contain unrelated user work; do not overwrite an existing project.
2. Transfer the 92 listed entries with their directory suffixes, using a staging/copy phase before source removal. Verify byte equality before purposeful edits and keep a source-to-destination change ledger.
3. Establish the Editor root files from section 4.4. Preserve license/asset provenance and record the import origin, including dirty source contents that are not represented by a commit alone.
4. Retain assembly/namespace/resource identities. Keep the application directory as `PixieEditor/` so the transferred solution's basic project location remains straightforward.
5. Do not copy generated output or local state. Source and staged destination cannot receive divergent untracked development during this interval; reconcile any necessary change through the ledger.

**Output:** reviewable destination tree. **Exit condition:** every transfer is accounted for, every difference is deliberate, and excluded files remain excluded. Source deletion has not yet occurred.

### D. Make managed builds and restores independent

1. Replace Engine-relative imports with `eng/EngineSdk.props` and an explicit installed build SDK.
2. Read Editor's own `version.json`; stop deriving Editor identity from the Engine checkout. Keep the selected Engine version visible separately in the UI where applicable.
3. Import/compile the canonical installed binding exactly once. Select public/development inputs and separate intermediate/output directories.
4. Set explicit Inspector feature/package selections, matching core/view and UI bindings. Configure optional local source independently of the workplace's directory arrangement.
5. Update all four test project references from the old `../../../editor/PixieEditor/` location to the new owning project. Preserve the start-screen test's linked `FolderBrowser.cs` arrangement unless deliberately replaced with equivalent coverage.
6. Register the tests in the solution with an explicit development qualification configuration. A normal application build must not unexpectedly compile private tests with public binding inputs.
7. Prepare clean restore and publication parameters, lock files and feeds. Make profile mismatches actionable errors.

**Output:** independently buildable application and test projects. **Exit condition:** clean output builds from an unrelated working directory without accessing Engine source, optional Inspector sources or inherited sibling paths.

### E. Reconcile paths, fixture inputs and state

1. Apply the resolution rules in section 5 to every path-bearing caller; retain a checklist of changed and verified callers.
2. Replace ancestor discovery in application paths. Supply installed template/generation support or explicit fixture roots to tests of legacy behavior.
3. Adapt capture invocation to an explicit SDK and module/project artifact. `--game boids` by itself is not an installed-artifact test.
4. Ensure process-reopen and module tests receive their required modules explicitly instead of deriving a producer `bin` layout. Source tests reassigned to Engine remain linked in the coverage ledger.
5. Preserve cache identity/ownership, CMake exact package selection, project-file protection and VS Code conflict handling. Reject stale caches rather than rewriting them in place.
6. Implement the bounded explicit layout path and migration behavior. Keep qualification layouts isolated from the user's normal settings.
7. Keep AppData settings and project-local handoff formats stable; test relocated references and refused mismatches.

**Output:** complete path-owner/caller disposition and adapted harnesses. **Exit condition:** no application path depends on Engine ancestry; all retained tests have runnable explicit inputs or a documented owning source harness. No case is silently dropped.

### F. Qualify the staged destination

Execute the applicable matrix in section 8 with recorded artifact identities. Begin with static/build checks, then supported relocation cases and normal application flows. Use disposable projects, outputs and caches; do not mutate real user projects as acceptance fixtures.

Cover normal/public and development profiles separately. Include code and no-code projects, field interactions for the selected baseline, full authored Scene/World behavior, committed recovery, export and restart continuity. Compare source and destination behavior at matched feature/input selections.

**Output:** receipts and a list of passes, failures, omissions and residual limitations. **Exit condition:** required relocation and preservation cases pass; no omitted case is presented as complete. A compiler-only result does not satisfy this stage.

### G. Switch integration and documentation ownership

1. Prepare the independent Editor CI using explicit SDK/dependency artifacts. Remove the Engine CI job that builds `editor/PixieEditor.slnx` only when the replacement job is ready and the transition is documented.
2. Keep Engine-native CI and generated native solution ownership intact. `cmake/PixieRootSolution.cmake` must not add the external Editor into Engine's native target graph. Never hand-edit its generated root solution.
3. Keep cross-product MCP qualification in Engine tooling for this increment, using explicit SDK, Editor, client and fixture artifacts. Do not copy its whole implementation into Editor to make paths convenient.
4. Move current Editor guides and update Engine README/dependency/development routing. Preserve native contracts and historical attribution. Keep old guide paths as clearly identified migration pointers where needed, including anchors required by preserved references.
5. Create destination development and code-authoring documentation. Validate links without requiring repositories to be siblings. Establish actual remote URLs before linking to them; until then, use product/path descriptions rather than invented URLs.
   Source-evidence links in this proposal also require a cutover disposition: preserve their observed-baseline meaning using a durable source/import anchor or an explicit provenance record when the old files are removed. Do not leave broken links or relabel old evidence as if it ran against the destination.
6. Update the workspace launcher at `../abrir-todas-as-solucoes.bat` to the new solution. This is a separately enumerated workspace integration edit, not permission to reorganize the workspace.

**Output:** consumers, CI and docs route to their actual owners. **Exit condition:** active build/use instructions no longer direct users to the nested Editor; integration tests receive artifact inputs; historical records are not rewritten as new execution evidence.

### H. Complete the source-removal transaction

1. Refresh source hashes and verify that both source and staged destination still match the reviewed transaction. Stop and reconcile if either changed.
2. Obtain execution authorization for the exact removals under the accepted proposal. Approval of the proposal's direction is not a successful cutover or a completed qualification result.
3. Remove only the allowlisted original Editor/test/script files after the destination passes. Preserve Engine-owned relocations, guide migration pointers and unrelated work.
4. Do not recursively delete `editor/` just because its source entries were moved. Untracked `.vs` or other state is not on the deletion list; any optional local cleanup is separate.
5. Search active Engine build/package/configuration files for the removed paths. Re-run the affected native/SDK producer checks and independent Editor checks with the old source paths absent.
6. Record the final source/destination mapping, import anchor, source-removal change and validation identities. Keep Engine history intact; do not rewrite repository history or create remote repositories implicitly.

**Output:** the application is no longer source-owned by Engine. **Exit condition:** both products operate under their declared boundaries after removal, not merely while the old tree still exists.

### I. Prepare independent releases

Prepare Engine SDK/runtime and Editor release workflows as separate deliverables under section 9. Reuse the Engine's existing local package assembly/verification work; do not replace it with an unchecked directory ZIP. Assemble reviewable local candidates and manifests before any publication decision.

**Output:** explicit package/release contracts and candidate artifacts. **Exit condition for preparation:** content, identity, dependency and exclusion checks are recorded. Uploads, tags, pushes, public releases and upstream package changes require their own authorization. Automatic GitHub acquisition in Editor is a subsequent feature, not a hidden prerequisite for H.

## 8. Qualification and relocation matrix

### 8.1 Evidence requirements

Each executed case records the source/build identifiers where available, Editor artifact hash/version, build binding selection, Inspector/core/view selection, SDK catalog/profile/build identity, commands, working directory, input paths, output paths, expected result and actual result. Failed/refused cases record whether files changed. Only disposable fixture/output locations may be mutated by tests.

A fresh build or run must distinguish the relocated inputs from old cached outputs. Preserve receipts for the exact artifacts used; do not borrow a pass from a prior SDK whose identity changed during producer relocation. Reuse prior evidence only for unchanged, explicitly matching scope.

Use a controlled test arrangement in which producer source is unavailable to the independent consumer for the independence claim. Do not rename/delete active repositories, change user accounts or require the future VM work merely to manufacture isolation. Select a non-destructive isolated staging/runner method during implementation. If source remains readable, label that run as relocation evidence only, not source-unavailability proof.

### 8.2 Required path cases

| Case | Setup/action | Required result |
|---|---|---|
| R01 | Build Editor at the confirmed workspace destination using explicit SDK/dependencies | No Engine-relative imports or duplicate binding; expected application/features produced |
| R02 | Copy Editor source to a different directory depth, away from Engine/Inspector/Lib | Clean build succeeds from explicit inputs; no sibling assumption |
| R03 | Run build/test entry points from a shell directory unrelated to the repository | Resolution follows owning files/arguments, not current directory |
| R04 | Move the built/published Editor package intact and launch its absolute executable path | Native UI assets, embedded icon and application startup resolve correctly |
| R05 | Launch that same package from a second unrelated working directory | Same selected SDK and settings; layout behavior follows the explicit owner |
| R06 | Relocate an intact SDK package to a different path containing spaces and Unicode, then select it | Catalog/profile/hash checks pass for the same payload; runtime loads from the new selection |
| R07 | Keep old SDK path in settings after moving it | Clear stale-selection behavior; no silent alternate SDK or source fallback |
| R08 | Build project code against relocated SDK | Fresh owned cache and exact PACKAGE inputs; no reuse of old absolute CMake state |
| R09 | Relocate a no-code user project with scenes/assets and reopen | Authored relative references remain valid; no repository path needed |
| R10 | Relocate a code project with its portable requirements | Exact SDK matching remains; stale cache/preset handled through explicit regeneration or conflict, preserving user files |
| R11 | Move Editor and SDK independently while project remains fixed | Project opens with explicit new selection; no coupling between installation locations |
| R12 | Move project, Editor and SDK to unrelated roots | Same supported authoring/build/export flows with reselected locations and new caches |
| R13 | Rebuild and reopen from relocated Editor with unsaved authored state | Correct child executable/assembly, SDK identity, project, camera/Environment/selection and existing history semantics |
| R14 | Configure/open VS Code after a project/SDK move with old generated settings present | New owned configuration or clear conflict; custom files preserved; no unexplained old path reuse |
| R15 | Relocate optional Inspector checkout and supply its explicit new development root | Development build works; public publication still rejects source integration |
| R16 | Restore/build from locked package feeds without optional local sources | Intended feature baseline reproduced; no fallback to an older/different Inspector pair |
| R17 | Run capture and channel qualification with external fixture/output/layout paths | No ancestor `samples`/`bin` lookup; normal user layout and project files untouched |
| R18 | Export no-code and code projects from unrelated source/output directories | SDK-declared exporter and shipping identity used; resulting product runs from its own relocated output |
| R19 | Make producer source unavailable to the tested consumer without altering active worktrees | Editor build and installed workflows succeed; no disguised source dependency |
| R20 | Supply missing/mixed/corrupt SDK payload, wrong profile, unsupported path or mismatched cache ownership | Precise refusal before unintended mutation; no arbitrary fallback or cache overwrite |
| R21 | Alternate normal/development/save-disabled builds in one source checkout | Separated intermediates prevent stale bindings/capabilities from leaking between artifacts |
| R22 | Remove the original allowlisted Editor sources after staged verification | Engine native/SDK builds and new Editor workflows remain valid |
| R23 | Launch with an existing destination layout and a different legacy layout | Destination wins; neither file is overwritten by migration; first-layout detection uses the destination path |
| R24 | Import a known legacy layout into an absent destination, then restart from another directory | Layout is preserved, original remains untouched, and ImGui read/write plus first-layout detection use the same resolved path |
| R25 | Launch without an existing destination layout or an unambiguous legacy source | Normal default layout and actionable import guidance; no ancestor/disk search or selection of unrelated layouts |
| R26 | Run layout/capture qualification using its explicit isolated layout path | All test layout reads/writes stay in that fixture; normal user layout/settings remain unchanged |

Not every invalid location must succeed. R20 qualifies intentional refusal under the existing path/containment policy. Path correctness includes refusing an ambiguous, unsafe or incompatible input while preserving data.

### 8.3 Application preservation cases

| Area | Required coverage |
|---|---|
| Startup | Choose admitted SDK, create/open project, recent-project handling and missing-SDK diagnostics |
| Basic editing | Select World/Scene/entities, inspect fields, multi-selection edits, create/duplicate/delete and undo/redo |
| Selected Inspector features | Chains, locks, grouped history and reset/rebind semantics match the chosen baseline |
| World/Scene authoring | Membership, presentation, camera/Environment selection, full-document history and candidate publication |
| Recovery | Stale edits rejected; committed changes remain committed through Inspector binding recovery; no false rollback claim |
| Session | Play/Stop, pause/time controls and normal close preserve existing semantics |
| Persistence | Save/reopen preserves authored state; runtime changes are not accidentally persisted |
| Code | Add code, create component, exact target build, failed build diagnostics, source editing, dirty rebuild/reopen and module lifecycle |
| Export | No-code and code products use the SDK's paired exporter and preserve declared scenes/assets |
| Automation | Existing allowed operations, save-enabled/save-disabled cases, generation checks and committed recovery |
| Public artifact | Export UI retained; private automation, MCP/test clients and undeclared native payload absent; publication guards hold |
| Native graphics | Basic window/context create, render, interaction, Play/Stop and normal dispose at current OpenGL 4.5; matched native GLFW version recorded |

The existing start-screen test installs a custom resolver directly. Retain its bounded API coverage, but add/retain normal application-loader cases; that test alone is not proof of SDK discovery/admission. Similarly, IL marker checks complement rather than replace a normal export interaction and payload inventory check.

## 9. GitHub distribution direction and later virtualization

### 9.1 Distinct products and versions

Engine publishes public authoring SDK artifacts and shipping runtime distributions. Private automation/qualification SDKs remain outside public releases. Editor publishes its own application with an independent product version. SDK and runtime may initially share the Engine version and release tag while remaining distinct artifacts/profiles. The exact asset naming and whether runtime is a separate archive must be finalized before workflow implementation; this proposal does not invent a published asset contract.

The already existing Engine SDK remains the base. The missing release orchestration is not evidence that the SDK must be redesigned. Likewise, local workflow inspection is not a new audit of Inspector/Lib remote publication status; the owner supplied their existing-release context.

### 9.2 Preparation requirements

Engine release preparation must bind the archive to product version, SDK/build identities, platform, architecture, configuration/profile, public/private distribution class, dependencies, compiler/CRT/CPU constraints, payload inventory, checksums and applicable licenses. Public assets must exclude private automation and test fixtures. Authoring and shipping components and the paired exporter must match their declared contracts.

Use `scripts/release/rehearse_sdk.py`, SDK assembly/verification and existing artifact checks as inputs where applicable. Preserve their scope and update paths/identities deliberately. Introduce a reviewed Engine `.github/workflows/publish.yml` in the release-preparation stage; do not copy a managed-library workflow unchanged and assume native dependency closure is covered.

The Editor publish workflow restores its own selected packages and consumes explicit build SDK artifacts. It does not compile Engine from a sibling checkout or bundle the Engine source. Local candidate assembly comes before remote release execution.

### 9.3 Future automatic acquisition

The planned downloader maps project requirements and supported Editor compatibility to a specific release asset, verifies it, installs it into a versioned cache and hands its root to the existing SDK admission code. It is an acquisition layer, not a second runtime identity resolver.

The current portable requirement includes `sdkId` and compatibility constraints; it is not already a complete GitHub release locator. Define an explicit catalog/metadata mapping from supported requirements to immutable release artifacts before implementing acquisition. Never translate an unknown identity to `latest` or silently rewrite the project requirement.

Requirements for that subsequent feature include incomplete-download handling, verification before activation, parallel installations, offline reuse, explicit upgrades, absence/mismatch diagnostics and retention of working installations. Moving the cache must follow the same re-selection/cache-ownership rules as manual SDK relocation. Do not introduce a second incompatible package layout just for download.

### 9.4 OpenGL and VMware correction

The post-extraction graphics target is OpenGL **4.3**, as selected by the owner for VMware virtualization. Prior 4.1 and 3.1 references in this conversation are superseded by that target. This proposal neither changes context creation/shaders nor asserts that the renderer already works at 4.3.

After extraction, separately inventory resources above 4.3, adapt Editor and runtime together, and qualify the chosen VMware product/version, guest, virtual graphics driver and settings. That work must not be folded into extraction, used to conceal current 4.5 failures, or required before independent builds can be established. The deferred detailed GLFW investigation remains explicit; matching version and a smoke pass do not prove single-instance state ownership.

## 10. Cutover, rollback and concurrent work protection

The extraction is a two-product transition. Keep an allowlist of additions, edits, source removals and pointer replacements, with preimages and hashes. Verify resolved paths belong to the intended Engine or new Editor roots before any move/removal. Any workspace launcher change has its own explicit path.

Before source removal, rollback means preserving the original runnable source baseline and retaining the staged candidate/evidence for diagnosis. Do not delete the staged tree if it acquired user changes. Do not use a hard reset, broad clean, recursive source-folder removal or history rewrite as rollback.

After source removal, restore only the recorded files/preimages if their current destinations have not been independently changed. Reconcile concurrent edits instead of overwriting them. Restore the relevant CI/build registrations coherently with the source state; do not leave both old and new jobs pointing to missing paths.

No commit/push/tag/publication is implied by local transfer. When authorized later, preserve Engine history and document the new repository's import anchor rather than destructively filtering the old repository. The import record must distinguish committed source from locally modified/new files.

Do not update Inspector, Lib, user projects, VAW or unrelated player/export work as extraction cleanup. If a real boundary defect requires such a change, identify its precise owner, evidence, files and approval scope before extending implementation.

## 11. Completion criteria and unresolved selections

Extraction is complete only when:

1. The independent Editor source tree and required tests/docs have the reviewed ownership recorded in Appendix A.
2. Engine source/build/SDK production no longer requires the removed Editor tree; the canonical binding and native fixture have their Engine-owned locations.
3. Editor builds from installed SDK/package inputs with explicit public/development profiles, own versioning and no mandatory sibling source roots.
4. Required relocation cases demonstrate correct resolution, explicit re-selection or safe refusal; producer-source independence is evidenced separately from simple relocation.
5. Matched-baseline authoring, session, code, recovery and export behaviors are preserved. Omissions are not labeled passes.
6. Publication guards and payload exclusions remain effective; normal export UI remains available.
7. Active CI/docs/launcher references point to the correct owners and no generated solution is hand-maintained.
8. Source removal is recorded and post-removal checks pass with unrelated work preserved.
9. Deferred work is named accurately: full graphics investigation, OpenGL 4.3/VM qualification, remote releases and automatic acquisition are not claimed complete by extraction.

Before execution, resolve and record the actual Git baseline, initial Editor version, selected .NET SDK, Inspector feature/package baseline and feed availability, concrete matching native GLFW artifact selection, exact test artifact set, and handling of any newly discovered source-specific fixture. Before release implementation, finalize repository URLs, release asset naming, SDK/runtime archive boundaries and retrieval metadata. These are bounded selections; they do not reopen the confirmed destination or require a VAW update.

The path requirements are mandatory acceptance targets. Closure requires
the caller inventory and relocation evidence, including clear,
data-preserving refusals for stale caches and unsupported paths.

## Appendix A. Exact observed source-to-destination inventory

All source paths below are relative to the current Engine root. `Editor` destinations are relative to the confirmed independent Editor root; `Engine` destinations remain inside the existing Engine root. `guide-content-transfer` permits a pointer-only record at the old path. Other `transfer` entries remove their original source only at stage H. The two `engine-relocation` entries are handled at stage B with their transitional consumers reconciled.

The companion JSON records source bytes and SHA-256 per entry. It is a proposal snapshot, not execution authorization or proof that sources will remain unchanged. New destination-owned files from section 4.4 and in-place integration edits from stages B/G/I are additional planned writes, not hidden transfer entries.

| Entry | Source in Engine | Destination owner | Destination path | Operation |
|---|---|---|---|---|
| EXTRACT-001 | `editor/PixieEditor/assets/inspector/chain.png` | Editor | `PixieEditor/assets/inspector/chain.png` | transfer |
| EXTRACT-002 | `editor/PixieEditor/assets/inspector/lock.png` | Editor | `PixieEditor/assets/inspector/lock.png` | transfer |
| EXTRACT-003 | `editor/PixieEditor/assets/neko-purple-light.ico` | Editor | `PixieEditor/assets/neko-purple-light.ico` | transfer |
| EXTRACT-004 | `editor/PixieEditor/Development/CaptureDirectory.cs` | Editor | `PixieEditor/Development/CaptureDirectory.cs` | transfer |
| EXTRACT-005 | `editor/PixieEditor/Development/ChannelTestAccess.cs` | Editor | `PixieEditor/Development/ChannelTestAccess.cs` | transfer |
| EXTRACT-006 | `editor/PixieEditor/Development/DeterministicCapture.cs` | Editor | `PixieEditor/Development/DeterministicCapture.cs` | transfer |
| EXTRACT-007 | `editor/PixieEditor/Development/EditorChannel.cs` | Editor | `PixieEditor/Development/EditorChannel.cs` | transfer |
| EXTRACT-008 | `editor/PixieEditor/Development/EditorPipe.cs` | Editor | `PixieEditor/Development/EditorPipe.cs` | transfer |
| EXTRACT-009 | `editor/PixieEditor/Development/ExportPanel.cs` | Editor | `PixieEditor/Development/ExportPanel.cs` | transfer |
| EXTRACT-010 | `editor/PixieEditor/Development/InspectorChannelCustomRows.cs` | Editor | `PixieEditor/Development/InspectorChannelCustomRows.cs` | transfer |
| EXTRACT-011 | `editor/PixieEditor/Development/InspectorChannelSurface.cs` | Editor | `PixieEditor/Development/InspectorChannelSurface.cs` | transfer |
| EXTRACT-012 | `editor/PixieEditor/Development/ProjectExportJob.cs` | Editor | `PixieEditor/Development/ProjectExportJob.cs` | transfer |
| EXTRACT-013 | `editor/PixieEditor/Editing/GroupStep.cs` | Editor | `PixieEditor/Editing/GroupStep.cs` | transfer |
| EXTRACT-014 | `editor/PixieEditor/Editing/History.cs` | Editor | `PixieEditor/Editing/History.cs` | transfer |
| EXTRACT-015 | `editor/PixieEditor/Editing/ProjectAssets.cs` | Editor | `PixieEditor/Editing/ProjectAssets.cs` | transfer |
| EXTRACT-016 | `editor/PixieEditor/Editing/SceneAuthoring.cs` | Editor | `PixieEditor/Editing/SceneAuthoring.cs` | transfer |
| EXTRACT-017 | `editor/PixieEditor/Editing/SceneDocumentEditor.cs` | Editor | `PixieEditor/Editing/SceneDocumentEditor.cs` | transfer |
| EXTRACT-018 | `editor/PixieEditor/Editing/SessionEditGate.cs` | Editor | `PixieEditor/Editing/SessionEditGate.cs` | transfer |
| EXTRACT-019 | `editor/PixieEditor/EditorSettings.cs` | Editor | `PixieEditor/EditorSettings.cs` | transfer |
| EXTRACT-020 | `editor/PixieEditor/Inspection/ComponentHandle.cs` | Editor | `PixieEditor/Inspection/ComponentHandle.cs` | transfer |
| EXTRACT-021 | `editor/PixieEditor/Inspection/InspectorFactory.cs` | Editor | `PixieEditor/Inspection/InspectorFactory.cs` | transfer |
| EXTRACT-022 | `editor/PixieEditor/Native/CodeEditor.cs` | Editor | `PixieEditor/Native/CodeEditor.cs` | transfer |
| EXTRACT-023 | `editor/PixieEditor/Native/CodeReopen.cs` | Editor | `PixieEditor/Native/CodeReopen.cs` | transfer |
| EXTRACT-024 | `editor/PixieEditor/Native/CodeWorkspace.cs` | Editor | `PixieEditor/Native/CodeWorkspace.cs` | transfer |
| EXTRACT-025 | `editor/PixieEditor/Native/EditorLaunch.cs` | Editor | `PixieEditor/Native/EditorLaunch.cs` | transfer |
| EXTRACT-026 | `editor/PixieEditor/Native/EngineSdk.cs` | Editor | `PixieEditor/Native/EngineSdk.cs` | transfer |
| EXTRACT-027 | `editor/PixieEditor/Native/EngineSession.cs` | Editor | `PixieEditor/Native/EngineSession.cs` | transfer |
| EXTRACT-028 | `editor/PixieEditor/Native/GameLibrary.cs` | Editor | `PixieEditor/Native/GameLibrary.cs` | transfer |
| EXTRACT-029 | `editor/PixieEditor/Native/ProjectBuild.cs` | Editor | `PixieEditor/Native/ProjectBuild.cs` | transfer |
| EXTRACT-030 | `editor/PixieEditor/Native/ProjectCode.cs` | Editor | `PixieEditor/Native/ProjectCode.cs` | transfer |
| EXTRACT-031 | `editor/PixieEditor/Native/SdkRequirements.cs` | Editor | `PixieEditor/Native/SdkRequirements.cs` | transfer |
| EXTRACT-032 | `editor/PixieEditor/PixieEditor.csproj` | Editor | `PixieEditor/PixieEditor.csproj` | transfer |
| EXTRACT-033 | `editor/PixieEditor/Program.cs` | Editor | `PixieEditor/Program.cs` | transfer |
| EXTRACT-034 | `editor/PixieEditor/Ui/AssetChooser.cs` | Editor | `PixieEditor/Ui/AssetChooser.cs` | transfer |
| EXTRACT-035 | `editor/PixieEditor/Ui/Authoring.Colliders.cs` | Editor | `PixieEditor/Ui/Authoring.Colliders.cs` | transfer |
| EXTRACT-036 | `editor/PixieEditor/Ui/Authoring.cs` | Editor | `PixieEditor/Ui/Authoring.cs` | transfer |
| EXTRACT-037 | `editor/PixieEditor/Ui/BuildScreen.cs` | Editor | `PixieEditor/Ui/BuildScreen.cs` | transfer |
| EXTRACT-038 | `editor/PixieEditor/Ui/CandidateRecoveryPanel.cs` | Editor | `PixieEditor/Ui/CandidateRecoveryPanel.cs` | transfer |
| EXTRACT-039 | `editor/PixieEditor/Ui/DropEvents.cs` | Editor | `PixieEditor/Ui/DropEvents.cs` | transfer |
| EXTRACT-040 | `editor/PixieEditor/Ui/EditorWindow.cs` | Editor | `PixieEditor/Ui/EditorWindow.cs` | transfer |
| EXTRACT-041 | `editor/PixieEditor/Ui/EntitiesPanel.cs` | Editor | `PixieEditor/Ui/EntitiesPanel.cs` | transfer |
| EXTRACT-042 | `editor/PixieEditor/Ui/FolderBrowser.cs` | Editor | `PixieEditor/Ui/FolderBrowser.cs` | transfer |
| EXTRACT-043 | `editor/PixieEditor/Ui/Gl.cs` | Editor | `PixieEditor/Ui/Gl.cs` | transfer |
| EXTRACT-044 | `editor/PixieEditor/Ui/InspectorPanel.cs` | Editor | `PixieEditor/Ui/InspectorPanel.cs` | transfer |
| EXTRACT-045 | `editor/PixieEditor/Ui/InspectorPanel.Interactions.cs` | Editor | `PixieEditor/Ui/InspectorPanel.Interactions.cs` | transfer |
| EXTRACT-046 | `editor/PixieEditor/Ui/InspectorPanel.Layers.cs` | Editor | `PixieEditor/Ui/InspectorPanel.Layers.cs` | transfer |
| EXTRACT-047 | `editor/PixieEditor/Ui/InspectorPanel.Physics.cs` | Editor | `PixieEditor/Ui/InspectorPanel.Physics.cs` | transfer |
| EXTRACT-048 | `editor/PixieEditor/Ui/InspectorPanel.Presentation.cs` | Editor | `PixieEditor/Ui/InspectorPanel.Presentation.cs` | transfer |
| EXTRACT-049 | `editor/PixieEditor/Ui/InspectorPanel.Records.cs` | Editor | `PixieEditor/Ui/InspectorPanel.Records.cs` | transfer |
| EXTRACT-050 | `editor/PixieEditor/Ui/InspectorPanel.Scene.cs` | Editor | `PixieEditor/Ui/InspectorPanel.Scene.cs` | transfer |
| EXTRACT-051 | `editor/PixieEditor/Ui/InspectorPanel.World.cs` | Editor | `PixieEditor/Ui/InspectorPanel.World.cs` | transfer |
| EXTRACT-052 | `editor/PixieEditor/Ui/KeyEvents.cs` | Editor | `PixieEditor/Ui/KeyEvents.cs` | transfer |
| EXTRACT-053 | `editor/PixieEditor/Ui/LayerScheme.cs` | Editor | `PixieEditor/Ui/LayerScheme.cs` | transfer |
| EXTRACT-054 | `editor/PixieEditor/Ui/Screenshot.cs` | Editor | `PixieEditor/Ui/Screenshot.cs` | transfer |
| EXTRACT-055 | `editor/PixieEditor/Ui/Selection.cs` | Editor | `PixieEditor/Ui/Selection.cs` | transfer |
| EXTRACT-056 | `editor/PixieEditor/Ui/SessionPanel.cs` | Editor | `PixieEditor/Ui/SessionPanel.cs` | transfer |
| EXTRACT-057 | `editor/PixieEditor/Ui/StartScreen.cs` | Editor | `PixieEditor/Ui/StartScreen.cs` | transfer |
| EXTRACT-058 | `editor/PixieEditor/Ui/Status.cs` | Editor | `PixieEditor/Ui/Status.cs` | transfer |
| EXTRACT-059 | `editor/PixieEditor/Ui/Toolbar.cs` | Editor | `PixieEditor/Ui/Toolbar.cs` | transfer |
| EXTRACT-060 | `editor/PixieEditor/Ui/ViewportPanel.Colliders.cs` | Editor | `PixieEditor/Ui/ViewportPanel.Colliders.cs` | transfer |
| EXTRACT-061 | `editor/PixieEditor/Ui/ViewportPanel.cs` | Editor | `PixieEditor/Ui/ViewportPanel.cs` | transfer |
| EXTRACT-062 | `editor/PixieEditor/Ui/ViewportPanel.Lights.cs` | Editor | `PixieEditor/Ui/ViewportPanel.Lights.cs` | transfer |
| EXTRACT-063 | `editor/PixieEditor/Ui/WindowIcon.cs` | Editor | `PixieEditor/Ui/WindowIcon.cs` | transfer |
| EXTRACT-064 | `editor/PixieEditor.slnx` | Editor | `PixieEditor.slnx` | transfer |
| EXTRACT-065 | `editor/README.md` | Editor | `README.md` | transfer |
| EXTRACT-066 | `tests/editor/channel/PipeQualification.cs` | Editor | `tests/channel/PipeQualification.cs` | transfer |
| EXTRACT-067 | `tests/editor/channel/PixieEditor.ChannelTests.csproj` | Editor | `tests/channel/PixieEditor.ChannelTests.csproj` | transfer |
| EXTRACT-068 | `tests/editor/channel/Program.cs` | Editor | `tests/channel/Program.cs` | transfer |
| EXTRACT-069 | `tests/editor/code-components/CodeWorkspaceTests.cs` | Editor | `tests/code-components/CodeWorkspaceTests.cs` | transfer |
| EXTRACT-070 | `tests/editor/code-components/EditorUi.cs` | Editor | `tests/code-components/EditorUi.cs` | transfer |
| EXTRACT-071 | `tests/editor/code-components/EnvironmentViewportTests.cs` | Editor | `tests/code-components/EnvironmentViewportTests.cs` | transfer |
| EXTRACT-072 | `tests/editor/code-components/ModuleLoadRetryTests.cs` | Editor | `tests/code-components/ModuleLoadRetryTests.cs` | transfer |
| EXTRACT-073 | `tests/editor/code-components/ModuleSceneTests.cs` | Editor | `tests/code-components/ModuleSceneTests.cs` | transfer |
| EXTRACT-074 | `tests/editor/code-components/PixieEditor.CodeTests.csproj` | Editor | `tests/code-components/PixieEditor.CodeTests.csproj` | transfer |
| EXTRACT-075 | `tests/editor/code-components/PresentationTests.cs` | Editor | `tests/code-components/PresentationTests.cs` | transfer |
| EXTRACT-076 | `tests/editor/code-components/PresentationVisualTests.cs` | Editor | `tests/code-components/PresentationVisualTests.cs` | transfer |
| EXTRACT-077 | `tests/editor/code-components/ProcessReopenTests.cs` | Editor | `tests/code-components/ProcessReopenTests.cs` | transfer |
| EXTRACT-078 | `tests/editor/code-components/Program.cs` | Editor | `tests/code-components/Program.cs` | transfer |
| EXTRACT-079 | `tests/editor/code-components/SceneAuthoringTests.cs` | Editor | `tests/code-components/SceneAuthoringTests.cs` | transfer |
| EXTRACT-080 | `tests/editor/code-components/SceneCandidateTests.cs` | Editor | `tests/code-components/SceneCandidateTests.cs` | transfer |
| EXTRACT-081 | `tests/editor/code-components/SdkSelectionTests.cs` | Editor | `tests/code-components/SdkSelectionTests.cs` | transfer |
| EXTRACT-082 | `tests/editor/code-components/VisualLayoutTests.cs` | Editor | `tests/code-components/VisualLayoutTests.cs` | transfer |
| EXTRACT-083 | `tests/editor/field-interactions/PixieEditor.FieldInteractionTests.csproj` | Editor | `tests/field-interactions/PixieEditor.FieldInteractionTests.csproj` | transfer |
| EXTRACT-084 | `tests/editor/field-interactions/Program.cs` | Editor | `tests/field-interactions/Program.cs` | transfer |
| EXTRACT-085 | `tests/editor/start-screen/PixieEditor.StartScreenTests.csproj` | Editor | `tests/start-screen/PixieEditor.StartScreenTests.csproj` | transfer |
| EXTRACT-086 | `tests/editor/start-screen/Program.cs` | Editor | `tests/start-screen/Program.cs` | transfer |
| EXTRACT-087 | `tests/export/qualify_editor_publish.py` | Editor | `tests/publish/qualify_editor_publish.py` | transfer |
| EXTRACT-088 | `tests/export/InspectEditorPublishIl.cs` | Editor | `tests/publish/InspectEditorPublishIl.cs` | transfer |
| EXTRACT-089 | `tests/export/packages.lock.json` | Editor | `tests/publish/packages.lock.json` | transfer |
| EXTRACT-090 | `tests/capture/qualify_editor_capture.py` | Editor | `tests/capture/qualify_editor_capture.py` | transfer |
| EXTRACT-091 | `docs/editor-field-interactions.md` | Editor | `docs/editor-field-interactions.md` | guide-content-transfer |
| EXTRACT-092 | `docs/editor-movement.md` | Editor | `docs/editor-movement.md` | guide-content-transfer |
| EXTRACT-093 | `editor/PixieEditor/Native/PixieCapi.cs` | Engine | `capi/bindings/dotnet/PixieCapi.cs` | engine-relocation |
| EXTRACT-094 | `tests/editor/channel/NativeWriteProbe.cpp` | Engine | `tests/devchannel/NativeWriteProbe.cpp` | engine-relocation |

## Appendix B. Expected in-place edits and preservation boundaries

| Owner/path | Planned change | Boundary |
|---|---|---|
| Engine `cmake/sdk/PixieSdk.cmake` | Binding producer inventory/install source | Installed payload paths preserved; new producer identity recorded |
| Engine `tests/CMakeLists.txt` | Native write-probe source location | Existing target role and private fixture status preserved |
| Transitional Engine `editor/PixieEditor/PixieEditor.csproj` | Binding input until destination builds are active | No independently editable duplicate binding |
| Destination `PixieEditor/PixieEditor.csproj` and new build props | Version, installed binding, metadata, dependencies and profiles | No Engine checkout fallback |
| Destination `PixieEditor.slnx` and four test projects | References and explicit qualification configuration | Preserve assembly identities and equivalent coverage |
| Destination path owners in `Native/` and `Ui/EditorWindow.cs` | SDK/fixture/cache/relaunch/layout resolution | Changes bounded by section 5; no broad architecture redesign |
| Destination capture/publish/test entry points | Explicit SDK/artifacts/layout/output and matching profiles | Tests may not recreate old ancestors |
| Engine `.github/workflows/ci.yml` | Remove nested Editor build job at handover | Keep native CI and explicit cross-product artifact qualification |
| Engine README, DEPENDENCIES and development docs | Route Editor ownership and commands | No stale sibling requirements; historical claims remain historical |
| Engine `docs/entity-input-and-code-components.md` | Separate current Editor workflow from native/generation contract | Native contract stays Engine-owned |
| Engine old Editor guide paths | Migration pointers and necessary preserved anchors | No duplicate mutable guide |
| Engine MCP/harness inputs that actually name moved artifacts | Update declared paths or invocation interfaces | No broad MCP extraction or duplicated implementation |
| Workspace launcher | Point to independent solution | No workspace-wide reorganization |
| Engine/Editor release workflows and package metadata | Later stage I preparation | No publication without separate authorization |
| Engine generated root solution, old outputs/caches | No manual edits or transfer as source | Regenerated by their existing owners when applicable |
| Inspector, Lib and VAW repositories | No implementation edits planned | Any discovered extension is separately reviewed |

The exact lines/files for a conditional integration edit are added to the stage-A execution ledger before writing them. This preserves an exact source-transfer inventory without pretending that every future implementation detail or remote release choice has already been decided.
