# Native entity parenting in the Editor

**Document ID:** GLOBAL-NATIVE-PARENTING-EDITOR-EVIDENCE-2026-11-07
**Schema version:** 1
**Kind:** audit
**Lifecycle:** historical
**Subject:** version 3 inline Inspector hierarchy, structural history and code-bearing export
**Surface:** validation-evidence
**Boundary:** global
**Authority role:** evidence
**Mutation:** snapshot
**Indexing:** include

This records the local Windows x64 Release qualification for `PIXIE-TASK-259`.
The test used the same visible entities and stable IDs as the
[persistence fixture](native-parenting-persistence-2026-11-07.md).
The Editor working copy omitted Alternate so only Root appeared in Entities.
The five-entity fixture remained available for the earlier runtime and SDK
checks.

## Editor history and code rebuild

- The public Editor built against the locally assembled SDK with no warnings
  or errors. `ctest --preset vs2026-release -R '^runtime-tests$'` passed
  317 tests with zero failed checks. The new batch test moved only selected
  branch roots and rejected a cyclic batch.
- In the actual Editor, Root appeared on the left. The Inspector placed Pivot
  below the root's components and exposed A and B under Pivot. Expanding
  each child exposed its regular component controls. The parent picker
  offered world-preserving edits. The native keep-local mode stayed
  available through the API.
- Attempting to parent Pivot to A was rejected with a cycle error. The scene
  stayed clean. Parenting A to B made the scene dirty and retained A's world
  position `(125, 110)`. Undo restored A under Pivot and Redo restored A
  under B. The corresponding local position under B was `(-5, 10)`.
- The disposable fixture used a portable SDK requirement copied from the
  selected catalog's authoring profile. An initial attempt in the earlier
  working copy had stopped before generation because the requirement was
  absent. This pass supplied the requirement file as fixture setup.
- **Code > Add code** compiled the first module. **Code > Build and reopen**
  rebuilt it after changing Spinner's default to `1.25` radians per second.
  The replacement process restored the unsaved link and selected A. Adding
  Spinner from B's inline component controls showed the new default of
  `71.6197` degrees per second. A second source edit made Spinner schedule
  its local-pose write through `World::Defer` and `SetLocalTransform2D`.
  A further rebuild retained both the unsaved link and B's Spinner.
- Before explicit Save, the disk scene still had its baseline SHA-256.
  Play moved the branch and Stop returned it to its authored pose. The final
  saved scene had four entities and three links, Root to Pivot to B to A.
- **Duplicate branch** on Pivot copied three entities with the copied Pivot
  still under Root. Undo and Save reproduced the exact exported scene bytes.
  A separate public Editor process opened the saved module and ran
  `--select B --do delete --do select=A --do undo --do redo --do undo --do select=A --do save`.
  A survived removal of B at `(125, 110)`. Undo restored B and A's link to it.
  The final save reproduced the same bytes.

The fixture module used the selected SDK's Windows x64 MSVC `19.51.36260.0`
authoring profile. Native consumer builds reported MSB8029 for temporary
output directories and the first configure reported two unused CMake inputs.
The build and export completed successfully.

## Paired shipping player

The Editor's **File > Export game...** built a code-bearing Release player
against the paired shipping profile. The exporter verified the manifest,
dependency closure and shipping exclusions. The exported scene and shipping
DLL matched the saved scene and selected shipping DLL byte for byte.

`ParentingFixture.exe --quit-after 0.35 --screenshot <early.png>` and the
same command at `1.35` seconds both exited successfully on AMD Radeon RX
7600 with OpenGL 4.5. Measured center distances in the two rendered frames
were `19.72` and `20.04` pixels from Root to Pivot, `10.25` and `9.98`
from Pivot to B, and `11.41` and `11.69` from B to A. Root stayed fixed.
The direction from B to A rotated approximately 73 degrees relative to the
root's rotation across the captures, consistent with the authored Spinner
speed over the one-second interval.

| Artifact | SHA-256 |
| --- | --- |
| SDK catalog | `c8ee2b17b617448bbdb4b32848de7b73529a8abc91afcb0b4c0760492afe1bea` |
| Editor managed assembly | `67a5e87bdb0ddc5f8e3b7fc13c35f4d358cfc2b1bbb83ebd76b01d0bb9cb1571` |
| Baseline saved scene | `2bfbdc950b03b25803667b7ef758790844051cab4f0483d9cdc6de0ffb96baeb` |
| First authoring module | `44b4079ff7381404501233566393d8056781dfaa60f75ec253f6f6769016539b` |
| Authoring module after local-pose code rebuild | `3214cb2173d3ecc67e04753292ba9d16026185d03decc13f91aa7dc71e78031a` |
| Final saved and exported scene | `b8fdf19a04dfca7046b4bf4251aa8e305a18684cae307a3a03087cd4b4569ecf` |
| Shipping DLL | `a5c291c36b1230dbbcbf1d2c4c39da97093fd075b8fa134d4c45562802bb417f` |
| Exported player | `eab2a91a3e2fc008d3451e5209c83b49dc512b06ad3b811209b5fc0aef3a013d` |
| Export manifest | `bc8bd855e29f7cd7a71b273546a35d81257d57ccf7f24aea0a6849d0689e323a` |

The project, single code-bearing export, manifest, build logs and captures
are under Engine `.local/parenting-023/`. The SDK identity is
`7d787d8a055ad5ecfc6da5342d116ffa31aca9fa2408fdeb510a716b8d40027b`.
This run used one Windows account and an installed-package layout on the
producer machine. It covers the simple scene only. Weapons-Lab acceptance
remains a separate task.

## Compact tree follow-up

The updated public Editor places a tree of names immediately above
**Add component**. On the same four-entity scene, the Pivot arrow hid and
restored its descendants while A's fields stayed visible. Clicking B showed
B's Transform2D, Shape and Spinner. Right-clicking the unselected A opened
its menu while B stayed selected. **Set parent > Pivot** moved A to Pivot
and preserved world position `(125, 110)`. Undo restored B as A's parent.
**Duplicate branch** on Pivot copied its three entities and kept one root
in Entities. Undo and Save reproduced the same scene hash as above.

The updated Editor assembly SHA-256 is
`810be88d1671e30ba2868079a1a5931e88b8847cd8d31b6a429ae928300e20a8`.
The follow-up exercised the Editor layout and commands with the same SDK.
The shipping evidence above describes the earlier export run.

## Opening the working copy

From the Engine root, use the current local Editor build and the SDK from this run.

```powershell
$work = (Resolve-Path -LiteralPath '.local/parenting-023').Path
$sdk = Join-Path $work 'sdk-fixture/editor-pass/sdk'
$catalog = (Get-FileHash -LiteralPath (Join-Path $sdk 'sdk.json')).Hash
$editor = Join-Path $work 'editor-artifacts/bin/PixieEditor/release/PixieEditor.exe'
$project = Join-Path $work 'single-root-fixture'
& $editor --sdk $sdk --sdk-manifest-sha256 $catalog --native-configuration Release --project $project --recent off --select A
```
