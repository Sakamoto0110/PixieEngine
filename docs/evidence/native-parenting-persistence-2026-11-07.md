# Native entity parenting persistence and simple export

**Document ID:** GLOBAL-NATIVE-PARENTING-PERSISTENCE-EVIDENCE-2026-11-07
**Schema version:** 1
**Kind:** audit
**Lifecycle:** historical
**Subject:** version 3 parenting scene, installed SDK, Editor round trip and paired player export
**Surface:** validation-evidence
**Boundary:** global
**Authority role:** evidence
**Mutation:** snapshot
**Indexing:** include

This is the `PIXIE-TASK-258` follow-up to the
[in-memory result](native-parenting-runtime-2026-11-07.md). It covers the
[simple fixture](../../tests/runtime/fixtures/native-parenting/ParentingFixture.pixie)
from [Proposal 023](../proposals/native-entity-parenting.md), with a working
copy in Engine `.local/parenting-023/fixture`. The project uses the built-in
scene runtime. It has five authored entities and three parent edges.

## Scene and SDK checks

The version 3 scene stores `parent2D` as a stable scene ID and uses local
`Transform2D` fields on children. The existing Windows x64 Release tree ran
`runtime-tests` (316 tests) and `lighting-tests` (42 tests). Both passed.
The new tests cover reverse-order graph load, pose evaluation, a scene round
trip, fragment copy with internal and external parent IDs, and rejection of
missing parents, cycles, incomplete poses and invalid parent scale. A rejected
load preserved the destination World in the tested cases.

A prior player from SDK catalog `99c9f6789a1fd79f018f15e53f53c897619fde289454e938629d83bc8b85089a`
refused the unmodified version 3 scene with exit code 1 and an explicit newer
version error. It reported support through version 2.

A local public-shaped SDK was assembled with authoring Release, shipping
Release and a paired exporter. It was verified with the SDK verifier's runtime
queries. This package is Release-only and does not represent a full public
release distribution. The first exporter configure lacked `rc` and `mt` in the
developer shell. Retrying with Windows SDK `10.0.26100.0` paths paired the
already-built profiles. The build used MSVC `19.51.36260.0`.

The [installed consumer](../../tests/sdk-consumers/parenting/CMakeLists.txt)
used `find_package` and `Pixie::Runtime` from the assembled SDK. It read the
unmodified fixture, checked the two parent edges up to the root, checked the
marker pose before and after a quarter turn, and checked the exact serialized
round trip. The staged authoring DLL matched the SDK DLL. Source checkout
availability was unchanged, so this is an installed-package test on the same
machine.

## Editor and exported player

The existing public Editor 0.3.0 was launched with the SDK catalog hash and
an explicit fixture path. The scripted inspector route set marker A's world X
from `120` to `125`. Undo restored `(120, 110)` and Redo restored `(125, 110)`.
Play and Stop completed, and the Editor saved the working scene. The saved
version stayed 3, all three links retained their stable IDs, and A stored
local `(5, 10)`.

The Editor was opened again from disk. Computer Use showed A at world
`(125, 110)` in Editing mode. Play showed it at approximately `(77.9973,
115.52)` with a world angle of `123` degrees. Stop restored `(125, 110)` and
the saved editing state. **File > Export game** wrote the local
`.local/parenting-023/fixture-export` folder. The exporter verified shipping
dependencies and exclusions. The exported scene bytes matched the saved scene
and its runtime DLL matched the SDK shipping profile.

The exported `pixie-player.exe` launched from the export folder without an
SDK or project argument. `--quit-after 1.35 --screenshot <path>` exited zero
and produced a 1280 by 720 screenshot after movement. The measured distances
from the root to the pivot, A and B in that frame were `20.04`, `26.33` and
`29.96` pixels. Expected distances were `20.00`, `26.93` and `30.00` pixels.
All passed the two-pixel tolerance, including the offset caused by the red
triangle's centroid. The pivot was visibly rotated away from its saved pose.

## Artifact identities and limits

| Artifact | SHA-256 or identity |
| --- | --- |
| SDK ID | `904a5d1b492765729f66d3c5e54cce2cbaedd187e70461d2a65401e71b2f0ce7` |
| SDK catalog | `2f31c870d1deccc781d773ed51f5ed169f66e65ac2be8a0b79be72f2b79c45aa` |
| SDK authoring DLL and staged probe DLL | `463a2a63b2b17635ae442cb33c71d1c5afc73a856fd69f42952765a5ea50fd47` |
| Editor executable | `d8a68d84aa87bee10b0d7f63c5e4769926d25970f98dfda084ccf7ed42fa7f97` |
| Original fixture scene | `8f20aa198148a93aa65ef3a12aad78199e72558a325d3ee81743f2b1f97ff90a` |
| Saved and exported scene | `18709ad761592b2479ce7b214343dcefca83a912bb26732e60568d1485366700` |
| Exported shipping DLL | `e20b87123fbfcd8f994a337209405feaa5bbbc1d80e34d4f89323e5dc4f14707` |
| Exported player executable | `af08ace9ee6ce700feb4999ba488d3674b58bfd8875a7b33a48d510c38b94900` |
| Player screenshot | `dd1fb9ed9f57580210dbb8d293fd86fd9a390572877b203ac847618714f9c7f5` |

The SDK receipt, build logs, Editor logs and player screenshot are under
`.local/parenting-023/`. Computer Use observations are in the conversation.
This pass covers persisted hierarchy through the installed SDK, current
Editor and no-code export. The Editor still presents a flat entity list.
Hierarchy authoring, structural Undo/Redo and an unsaved code rebuild need
the later Editor slice. Weapons-Lab remains reserved until that qualification.
No publication or D09 qualification is part of this record.
