# Export and troubleshooting

**Document ID:** GLOBAL-CONSUMER-EXPORT
**Schema version:** 1
**Kind:** guide
**Lifecycle:** current
**Subject:** exporting and diagnosing a consumer game
**Surface:** guide
**Boundary:** global
**Authority role:** non-normative
**Mutation:** authored
**Indexing:** include

[Handbook](README.md) · [Setup](setup.md)

## Export through the Editor

1. Stop Play. Save the authored scene and all external C++ edits.
2. Use **Code > Build and reopen** if game code changed. Inspect the result.
3. Open **File > Export game...**. Choose a **new or empty output folder outside
   the game project**, for example `C:\Games\Exports\MyGame-r1`.
4. Run Export and wait for the panel's completion/log. A code game is compiled
   for the selected SDK's paired shipping runtime. Export is a local operation;
   it does not create a GitHub release or upload the game.
5. Open the resulting folder, run its game executable and exercise the controls.
   Keep the executable, native dependencies and packaged assets together.
6. Check the assets and their accompanying license/notice files as described
   below, then copy the complete export folder when handing it to another player. Report
   that machine's prerequisites/graphics baseline; a successful local launch
   does not establish clean-machine or other-platform support.

The authoring DLL used by the Editor and the shipping player are different
outputs. Export links game code into its player; it does not enable the public
shipping runtime to load authoring modules or private automation channels.
The exporter/runtime/descriptors must come from the selected compatible SDK.
Never fix a missing DLL by copying a similarly named file from another build.

## Assets and export limits

Use project-relative asset paths and keep files within the project. The normal
exporter discovers built-in authored Sprite/Text and supported material asset
references. A path string only embedded in C++ or an arbitrary custom component
is not automatically an export dependency just because it works locally.

Advanced export supports an explicit ASSETS inventory in its CMake integration.
However, editing the template-owned CMake file can make the Editor-managed code
workflow refuse it as custom wiring. Do not recommend that change while claiming
the managed workflow remains unchanged. For the first prototype, prefer assets
referenced through supported authored components; otherwise establish a deliberate
custom export workflow and verify its manifest/output before relying on it.
An authored Text referencing the same font used by the HUD is one way to give
that font an existing supported inventory reference. Check the exported file,
not only the source string.

Asset discovery does not automatically collect adjacent license or notice files.
In the consumer follow-up, an authored Text reference staged the HUD font, but
its accompanying license file was absent until copied separately. For each
third-party asset, retain the supplied license/notice text in the project and
carry the appropriate accompanying files into **each new export** before
distribution. Check their presence in the final folder alongside the asset;
keep the runtime's staged notices too. Record any manual copies in the consumer
report. This is output preparation, not a reason to edit the SDK or its inventory.

Do not put test logs, handbooks or custom assets inside the installed SDK. Its
inventory is validated; extra files can invalidate the package just as missing
files do. Keep each export attempt in a new folder rather than merging outputs.

## Keep iteration output small

For an existing game, reopen the same project with its selected SDK. After
changing C++ code, use **Code > Build and reopen**, then test in **Play**. This
uses the Editor's existing authoring cache when the project path and SDK
identity stay the same. Use a separate CMake scratch tree only for a diagnostic
or standalone check that needs it. Reuse that tree for the same inputs and
keep the relevant log and command in the report.

Export once when the requested standalone check or handoff is ready. Use a
new or empty destination, then verify the final files and launch the player.
For routine work, keep the current verified export and the prior one used for
comparison. Record paths, hashes and observations in `CONSUMER-REPORT.md`.
Keep any other export that serves as a baseline, is needed for handoff or
preserves evidence that the report alone cannot replace.

## Symptoms and next actions

| Symptom | Check / action |
|---|---|
| SDK requested at startup | Choose the root containing sdk.json, not profiles, bin, Engine source or a runtime-only folder |
| SDK exists for file tools but the Editor cannot find it | Verify the physical path from the Editor process and match the supplied identity; see [cross-application paths](setup.md#when-a-path-works-in-one-application-but-not-another) |
| SDK identity mismatch | Read the requested and selected IDs/profile/toolchain. Obtain the intended package; use explicit adoption only when compatible and intended |
| Valid folder rejected after copying | Verify all files/hashes and no unexpected additions. Do not delete identity metadata or turn off verification |
| New project refuses a folder | The location is the parent; location/name must be absent or empty. Do not point it at an existing source project |
| Code compile fails | Read the first compiler error, file and line; check exact MSVC/Windows SDK/CMake availability and generated registry ownership |
| Copied source appears unchanged after build | Timestamp-based build tools may see copied old timestamps as up to date. Save the changed source in the code editor, then Build and reopen |
| C++ component absent | Generate it through Add component, register Describe via the generated registry, compile and reopen; source files alone cannot change a loaded DLL |
| Open code launches the wrong app | It uses the OS file association/fallback. Open the displayed project source in your allowed external code editor |
| Controls do nothing | Check that game code binds that control; Play, choose Game input, focus/hover the viewport as required; attach enabled generic KeyboardControl/MouseControl for Behaviour callbacks/polling helpers |
| Only synthetic keyboard input fails | Compare a bound key manually when possible. Record the automation result separately; do not infer an Editor keyboard defect from it |
| Object does not move | Check required Velocity2D for the sample, speed/enabled, and whether a motor/body owns motion instead |
| Objects pass through each other | Visuals are not colliders; inspect collider geometry, enabled state, layer masks and motion owner |
| No collision callback fires | There is no OnCollisionEnter. Read ScenePhysicsEvents2D after Physics; character proxies have different event behavior |
| No particles | Check target pool, positive capacity/rate/lifetime and ParticleStep; collision events do not automatically emit |
| Light has no effect | Configure lighting, lower ambient, check masks/range/receiver; Emission2D alone does not light neighbors |
| HUD text absent | Readable project-relative TTF, nonzero size, exported asset; missing fonts draw nothing |
| Editing lost after Stop | Stop restores the authored snapshot at Play entry. Make permanent scene changes in Editing |
| Undo absent after rebuild | Reopen replaces process/history; unsaved authored content may be restored without Undo history |
| Inspector text seems stale after Undo | A consumer reported this for a custom field. Finish the edit, reselect/recapture and compare scene/game state and history; do not repeat a mutation based only on the old label |
| v2 rejects custom Behavior scene | Keep the supported v1 route; conversion is not required for a game using custom Behaviours |
| Export refuses to start | Stop Play, save, select a complete SDK with paired exporter; choose a new/empty destination |
| Export log is clipped | Resize/reposition the panel when possible. Otherwise inspect output and run the player; report the log as unread, not as a verified success message |
| Exported player cannot start | Keep complete output together; inspect log, native architecture/runtime prerequisites and GL4.5 graphics support |

Do not solve a consumer issue by silently changing project format, the SDK,
Engine internals, frozen native dependency policy or another user's files.
Preserve the exact diagnostic before attempting recovery.

## Optional diagnostic command line

These commands describe the exporter for technical diagnosis. **They do not
replace the mandatory Computer Use export in the Claude handoff.** Use the
executable path declared by `sdk.json` (the inspected SDK declares
`tools/export/Release/pixie-export.exe`):

```text
pixie-export --sdk <sdk-root> --identity
pixie-export --sdk <sdk-root> [--work <work-dir>] <project> <output-dir>
pixie-export --sdk <sdk-root> --verify <manifest.json> <output-dir>
pixie-export --sdk <sdk-root> --from-manifest <manifest.json> <output-dir>
```

Do not omit `--sdk` based on an older example. Manifest/build diagnostics belong
in the chosen work/output folders, not the SDK. Preserve error text and the
identity of the package used when asking the maintainer for help.

## What to report at handoff

Save `CONSUMER-REPORT.md` in the game folder using the
[report template](consumer-report-template.md). Include:

- Project folder, selected SDK version/profile/identity, Editor executable.
- What was actually done through Computer Use, including Save/reopen and export.
- C++ build result and which source examples were used or changed.
- Observed input, motion, visual effects and Stop behavior, separately from compilation.
- Export folder and whether its executable was launched and exercised.
- Missing features, failures or flows not yet checked. Screenshots/logs where useful.

State the exact build for each observation. Separate direct UI observations,
source inspection, another person's report and unchecked items. List actual
terminal commands only when you ran them; a UI Build/Export action should be
reported by its menu route, not an invented equivalent command. Include SDK
selection changes, external source/asset edits and manual output copies.

A local walkthrough demonstrates those actions on that machine. It is not a
substitute for independent first-consumer feedback, release qualification or a
promise that every public API has been visually tested.
