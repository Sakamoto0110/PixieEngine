# Internal SDK identity and integration

Status: Partially implemented for the SDK 0.4 candidate. The
[source-project compatibility contract](../modules/capi/REFERENCE.md#source-project-compatibility)
owns current admission rules. Installer/registry work below remains proposed;
exact native module admission remains in force. Candidate publication and
compatibility qualification are not implied by implementation.

Owner decision, 2026-11-22: a newer SDK that preserves the project's required
contracts must be usable without a version-only rejection. The selection policy
below records that direction. CMake, Editor and public-export admission now have
a candidate implementation; final-package qualification remains required.

## Problem and local evidence

The Editor previously summarized both local packages as `PixieEngine 0.3.0.0 · authoring · Release`. It now displays the short SDK identity with the Engine product version and selected tuple. The short identity helps distinguish installed packages; it does not prove publication or replace project admission.

| Installed folder | Package `sdkId` prefix | Catalog SHA-256 prefix | Editor profiles present | Project use on this machine |
| --- | --- | --- | --- | --- |
| `0.3.0.0-dev-current-b5043d1c` | `b5043d1c` | `fd4bdea7` | authoring Debug and Release | Both Weapons-Lab projects |
| `0.3.0.0-99c9f678` | `ebbab809` | `99c9f678` | authoring Debug and Release | Previous pin for `Weapons-Lab` |
| `0.3.0.0-atlas-acee92e4` | `acee92e4` | `c4e76419` | authoring Release | Previous pin for `Weapons-Lab-components-024` |

All three carry shipping Release. The `99c9f678` folder name abbreviates a catalog digest and the other two abbreviate an SDK identity. The manifests declare `distributionClass: public`, which constrains contents and capabilities. Publication provenance comes from a release receipt. Preserve unknown provenance for the legacy installations.

The older project SDKs were originally found under the Codex package's `LocalCache`. The local installation uses `%LOCALAPPDATA%\Pixie\SDKs`. The Editor's machine settings and project bindings are under `%APPDATA%\PixieEditor`. Both Weapons-Lab pins, the global Editor selection, and the local Editor build use the complete development SDK `b5043d1c`, which includes the middle-button pan fix and the paired shipping exporter. The published `v0.3.0.0` SDK remains installed side by side. The verification history and configuration preimages are under `.local/sdk-history/`. Module builds are under `%TEMP%\PixieEditor\code` and can be recreated from the installed SDK and project source.

The matching old SDK opened the original project in Debug and Release. Atlas Release failed with linker error `LNK1104` while an existing Editor process held the build output DLL open; it built and opened after that process closed. After the Editor started loading module snapshots, two instances could open the Atlas project in sequence without closing the first. Atlas Debug was rejected because the tuple is absent. These checks describe the current project files; they do not reconstruct earlier crashes or CMake edits.

The Editor's own MSBuild input is separate from the SDK chosen when opening a project. The current Editor source compiled with Atlas bindings; the older SDK's bindings lacked parenting and field metadata used by that source. A managed Editor compiled against Atlas bindings can still open projects with the older runtime when it admits the C API and gates optional exports. Keep the Editor build input pinned and validate the runtime matrix independently.

An earlier manual reopen used two separate steps: align the root CMake file with the SDK project template while keeping the tests separately buildable, then launch with an explicit SDK override. The CMake edit persists. A launch override alone neither changes the portable project pin nor selects a new global Editor default.

## Identity and installation

Use separate fields for separate decisions:

| Field | Owner and purpose |
| --- | --- |
| Product version | Engine version, displayed for product context. |
| Channel and artifact label | Producer receipt. A tagged, verified publication is labeled `release/<tag>`; a development package is labeled `dev/<base>+g<commit>.<build>`, where `<build>` is a producer-assigned build run identifier. A dirty development build records its input digest and dirty status. |
| `sdkId` | Exact package identity, not by itself proof of incompatibility with a different package. The short form is for display and lookup hints; compare the full value for integrity and reproducibility. |
| `sdkBuildId` | Exact native identity for the selected profile and configuration, used for module admission and restart. |
| Catalog and profile SHA-256 | Integrity pins for the actual manifest bytes. Compare against the trusted producer receipt and validate the entire inventory. |
| Distribution class | Payload and capability policy. A development artifact can have public payload; a private capability still requires the separate private distribution and an eligible Editor build. |

Release channels require a published artifact, tag-to-commit mapping and trusted receipt. A local build bearing the same product number is a development build even if its payload policy is public. Record producer revision, build inputs, toolchain, profile matrix and manifest digests in a versioned receipt. Keep legacy schema 1 readers working: have release CI or the explicitly configured development producer issue a receipt outside the SDK inventory, bound to the finalized catalog hash. Register the receipt in the installer; the release label must come from the publication job, not an editable local package field. If inline provenance is needed later, version and migrate the manifest and its CMake, Editor and runtime readers together.

Install into an explicitly configured, stable per-user root, using `%LOCALAPPDATA%\Pixie\SDKs` as the proposed Windows default for ordinary desktop processes. Register absolute roots visible to the Editor, including existing packaged-host roots if intentionally retained. Use immutable directories named from the producer label plus an identity prefix, and install side by side. Moving an SDK means re-registering the same exact identity and catalog hash at the new root. Keep old registered installations until dependent project pins are migrated. Store no path or local cache in the portable pin.

## Inspect an existing installation

Today, check the portable project pin at `.pixie-code/sdk.json` inside the project, the catalog at `sdk.json` inside the installed root, and each selected profile manifest at the relative path in the catalog's `profiles` array. For example:

```powershell
$catalog = Get-Content -Raw (Join-Path $sdkRoot 'sdk.json') | ConvertFrom-Json
$pin = Get-Content -Raw (Join-Path $projectRoot '.pixie-code/sdk.json') | ConvertFrom-Json
$catalog.sdkId -eq $pin.sdkId
$catalog.profiles | Select-Object profile, configuration, sdkBuildId, manifest
Get-FileHash (Join-Path $sdkRoot 'sdk.json') -Algorithm SHA256
python -B (Join-Path $sdkRoot 'tools/sdk/verify_sdk.py') $sdkRoot
& $editorExe --project $projectRoot --sdk $sdkRoot --sdk-profile authoring --native-configuration Release --recent off
```

Use absolute paths. The command is a one-off diagnostic against the existing pin; `--recent off` skips machine project history and binding updates. It is independent of the proposed registry and migration command.

## Selection and compatibility

1. An explicit launch or environment override wins, and must satisfy the project's compatibility requirement. Display the override source. A restart carries the same resolved path, manifest hash, SDK identity and build identity.
2. Otherwise prefer the last successfully opened, validated machine binding for the project. If it is missing or stale, resolve the original SDK or a verified compatible successor against registered SDKs. The global default is eligible when it satisfies that requirement. If provenance or the selected tuple is ambiguous, ask for a selection.
3. A compatible successor does not require project migration merely because its version or identity differs. Retain the original requirement and separately record the exact resolved SDK for reproducibility. An explicitly requested exact lock remains available for reproduction and qualification; a legacy identity pin must not silently be reinterpreted as such an intentional lock. Define the legacy-pin interpretation in the resolver/schema transition. Unknown or incompatible successors use the explicit adoption/migration flow; never silently rewrite the requirement.
4. Before configuring, verify the catalog, payload, host platform, CPU requirements, required capabilities and requested tuple. Offer only configurations in that artifact. Check Editor C API compatibility and required entry points separately from scene/resource schema compatibility. Validate the module identity and the C++ ABI tuple (compiler, STL, CRT, iterator debug level, architecture, profile and configuration) against the actual runtime. Never pair a Debug module with Release or infer compatibility from an equal or higher version number.
5. Record a successful binding by project and chosen tuple. Failed selections and failed builds must leave that binding and the portable pin intact. Show required and selected identities, the available tuples, failing stage and recovery action. A native process fault needs an isolated child and a supervising launcher to report its exit code; managed exception handling alone cannot recover from an access violation.

### Compatible successor acceptance

Compatibility is directional: accepting an old project on a new SDK does not
imply that the old SDK can read a project using new features. A trusted producer
declaration must identify the supported predecessor contracts and the qualified
profiles/configurations, with evidence bound to the candidate artifact. Source
API, behavior, scene/resource formats, CMake/code-generation contracts and the
Editor/runtime boundary must preserve the project's existing requirements.
Adding symbols alone does not establish this claim; unknown compatibility must
be reported as unverified, not as a confirmed breaking change.

Project acceptance and binary admission are separate. Rebuild available project
source against the resolved SDK when existing modules have another build identity;
do not weaken the native loader's identity/ABI checks. A binary-only dependency
without demonstrated ABI compatibility requires a compatible binary or rebuild.
No source edits or destructive format conversion belong to the compatible path.

VAW baseline approval is also separate: compatibility makes a candidate eligible,
but does not silently promote it to the approved workspace baseline. Once approved,
propagation resolves and records the actual artifact without blocking compatible
projects merely because their original SDK identity differs.

Acceptance must cover an older unchanged project with a verified additive SDK,
rebuild and reload of its modules, reopen and export, an explicit exact lock,
missing capabilities/tuples, incompatible ABI or formats, unknown compatibility,
and failed-build recovery. Version inequality alone must never be the failure.

## CMake ownership and updates

The portable project owns source, CMake entry points, generated component registry and the SDK compatibility requirement (or an explicitly requested exact lock). The local resolver owns installation paths, the exact resolved identity and build directories. Neither owns the other team's edits.

- Generate machine-local configure/build presets from the admitted SDK selection. Pass the same root, profile, toolset, Windows SDK, role and configuration to manual CMake and the Editor. Do not commit `CMakeUserPresets.json` or copy a literal developer path into `CMakeLists.txt`. Verify the exact resolved root and chosen tuple during configure.
- Keep different build directories for different project paths, catalog/build identities, roles and configurations. The Editor loads a separate immutable module snapshot so a second build can link while an earlier Editor holds its loaded DLL. Keep the build output available for manual debugging and export. Clear disposable caches only after the owning Editor processes close.
- The installed builder compares the root CMake file to the supported template outside the managed component block. The current Editor also accepts the restricted literal `ASSETS` declaration used by Weapons-Lab for its font license. Keep project tests in their own CMake entry point for current SDKs. Add a versioned, explicitly supported target hook in a future template for custom sources/tests; migrate generated blocks by declared template version and preimage. Never overwrite custom wiring. Keep source mode explicit, separate from installed package mode.
- For a verified compatible SDK update, install and verify the candidate side by side, resolve it without rewriting the original requirement, and rebuild into its own cache. Record the binding only after successful admission. For an incompatible or unverified update requiring explicit adoption, stage a candidate pin and template/registry migration **in a disposable copy at a different absolute project path**, then configure it with a fresh cache and use the explicit launch with recents disabled. Until the proposed resolver is implemented, the current exact-pin flow still applies. Review the pin, template/registry changes and scene/resource mutations. Apply the reviewed changes only to the intended project and regenerate its local presets and binding. Reopen from Recent and the disk browser without a special shortcut. Rollback restores the previous pin, template, registry, scene/resource changes, user presets and per-project binding, then resolves the retained SDK with its own cache. The binding key is the normalized absolute project directory; the global Editor default and other projects' bindings are independent.

## Delivery and acceptance

1. **Inventory and install:** introduce the producer receipt and installer/registrar; inventory the two legacy packages as distinct identities and preserve their current pins. Test install, relocation, side-by-side use and an absent tuple.
2. **Resolve and integrate:** expose validated inventory with channel, label, exact identity, profile/configuration and location. Make the Editor and CMake use one resolver contract while keeping the portable pin path-free. Verify the global default can differ from the project binding without breaking manual open or reopening after a restart.
3. **Migrate CMake:** introduce the versioned customization hook and a previewable migration for the managed project template and user presets. Exercise clean configure, module build, project tests, load, relink while another instance is running, and export with matched shipping Release.
4. **Qualify acceptance and failure paths:** run both Weapons-Lab variants as independent fixtures. Test compatible-successor acceptance and rebuild, incompatible cross-SDK rejection, explicit exact locks, unknown compatibility, Atlas Debug absence, edited payload, invalid MSVC/CRT, scene schema mismatch, a locked or corrupt cache and unexpected native failure. Verify stage-specific diagnostics remain visible for the interactive workflow and come out with a nonzero result for CLI automation. Keep the published SDK validation matrix separate from development smoke tests.
