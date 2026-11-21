# Internal SDK identity and integration

Status: Proposed. The current SDK wire format and exact module admission remain in force.

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
| `sdkId` | Exact package compatibility identity. The short form is for display and lookup hints; compare the full value. |
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

1. An explicit launch or environment override wins, and must satisfy the project pin. Display the override source. A restart carries the same path, manifest hash, SDK identity and build identity.
2. Otherwise prefer the last successfully opened, validated machine binding for the project. If it is missing or stale, resolve the project's exact identity against registered SDKs. The global default may satisfy that identity; it cannot override it. If more than one registered artifact can satisfy the pin but provenance or the selected tuple is ambiguous, ask for a selection.
3. Use the global default for a new project or as an explicit migration candidate. For legacy projects, allow the existing explicit adoption flow. A project with an existing pin requires a reviewed migration; a fallback installation never rewrites the pin silently.
4. Before configuring, verify the catalog, payload, host platform, CPU requirements, required capabilities and requested tuple. Offer only configurations in that artifact. Check Editor C API compatibility and required entry points separately from scene/resource schema compatibility. Validate the module identity and the C++ ABI tuple (compiler, STL, CRT, iterator debug level, architecture, profile and configuration) against the actual runtime. Never pair a Debug module with Release or accept a same-version artifact as a substitute.
5. Record a successful binding by project and chosen tuple. Failed selections and failed builds must leave that binding and the portable pin intact. Show required and selected identities, the available tuples, failing stage and recovery action. A native process fault needs an isolated child and a supervising launcher to report its exit code; managed exception handling alone cannot recover from an access violation.

## CMake ownership and updates

The portable project owns source, CMake entry points, generated component registry and the exact SDK requirement. The local resolver owns installation paths and build directories. Neither owns the other team's edits.

- Generate machine-local configure/build presets from the admitted SDK selection. Pass the same root, profile, toolset, Windows SDK, role and configuration to manual CMake and the Editor. Do not commit `CMakeUserPresets.json` or copy a literal developer path into `CMakeLists.txt`. Verify the exact resolved root and chosen tuple during configure.
- Keep different build directories for different project paths, catalog/build identities, roles and configurations. The Editor loads a separate immutable module snapshot so a second build can link while an earlier Editor holds its loaded DLL. Keep the build output available for manual debugging and export. Clear disposable caches only after the owning Editor processes close.
- The installed builder compares the root CMake file to the supported template outside the managed component block. The current Editor also accepts the restricted literal `ASSETS` declaration used by Weapons-Lab for its font license. Keep project tests in their own CMake entry point for current SDKs. Add a versioned, explicitly supported target hook in a future template for custom sources/tests; migrate generated blocks by declared template version and preimage. Never overwrite custom wiring. Keep source mode explicit, separate from installed package mode.
- For an SDK update, install and verify the candidate side by side. Stage a candidate pin and template/registry migration **in a disposable copy at a different absolute project path**, then configure it with a fresh cache and use the explicit launch with recents disabled. The candidate pin in the copy is required; an override cannot bypass the existing pin. Review the pin, template/registry changes and scene/resource mutations. Apply the reviewed changes only to the intended project and regenerate its local presets and binding. Reopen from Recent and the disk browser without a special shortcut. Rollback restores the previous pin, template, registry, scene/resource changes, user presets and per-project binding, then resolves the retained SDK with its own cache. The binding key is the normalized absolute project directory; the global Editor default and other projects' bindings are independent.

## Delivery and acceptance

1. **Inventory and install:** introduce the producer receipt and installer/registrar; inventory the two legacy packages as distinct identities and preserve their current pins. Test install, relocation, side-by-side use and an absent tuple.
2. **Resolve and integrate:** expose validated inventory with channel, label, exact identity, profile/configuration and location. Make the Editor and CMake use one resolver contract while keeping the portable pin path-free. Verify the global default can differ from the project binding without breaking manual open or reopening after a restart.
3. **Migrate CMake:** introduce the versioned customization hook and a previewable migration for the managed project template and user presets. Exercise clean configure, module build, project tests, load, relink while another instance is running, and export with matched shipping Release.
4. **Qualify failure paths:** run both Weapons-Lab variants as independent fixtures. Test cross-SDK rejection, Atlas Debug absence, edited payload, invalid MSVC/CRT, scene schema mismatch, a locked or corrupt cache and unexpected native failure. Verify stage-specific diagnostics remain visible for the interactive workflow and come out with a nonzero result for CLI automation. Keep the published SDK validation matrix separate from development smoke tests.
