# Weapons-Lab consumer parenting acceptance

**Document ID:** GLOBAL-NATIVE-PARENTING-CONSUMER-EVIDENCE-2026-11-07
**Schema version:** 1
**Kind:** audit
**Lifecycle:** historical
**Subject:** native hierarchy in a copied Weapons-Lab r5 project and its shipping player
**Surface:** validation-evidence
**Boundary:** global
**Authority role:** evidence
**Mutation:** snapshot
**Indexing:** include

The owner authorized `PIXIE-TASK-260` after the
[simple-scene qualification](native-parenting-editor-2026-11-07.md).
Acceptance used the complete Weapons-Lab r5 source in a separate copy. The
r4 export was retained as a binary reference. The original r5 input files
and r4 reference artifacts matched their baseline hashes after the run.

The version 3 copy persisted the base, its turntable and three independent
visual slots, with two active in the saved scene. The public Editor opened
the module, duplicated the five-entity branch with remapped IDs, restored
the original branch and an unsaved per-child field across a code rebuild,
then saved and ran Play/Stop. The two active visuals followed the platform
and Stop restored the authored pose. The paired shipping export loaded the
same saved scene, and the standalone player displayed the moving assembly
with the base fixed.

The detailed `CONSUMER-REPORT.md` is in the local workspace copy
`PixieEngine_ConsumerTests/Weapons-Lab-parenting-023/`. The export is in the
sibling `Weapons-Lab-parenting-023-export/` folder. Logs, hash ledger, the
temporary duplicate scene and the export manifest are under Engine
`.local/parenting-023/weapons-lab-acceptance/`. The tested surface was
Windows x64 Release with OpenGL 4.5. This records consumer acceptance for
the two-active-slot scene.
