# C API Findings

**Document ID:** CAPI-FINDINGS

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** unconfirmed observations about the C API contract and implementation

**Surface:** findings

**Boundary:** capi

**Authority role:** non-normative

**Mutation:** append-only

**Indexing:** include

These observations do not define behavior, establish a confirmed defect, or
schedule implementation. Dispositions and later evidence are appended. The
[reference](REFERENCE.md) and public header retain their authority.

## CAPI-FINDING-001 — View geometry and the last render

**Status:** unconfirmed

**Reference date:** 2026-10-18

**Reference commit:** `2365251e7ea50fe730e937c03bdd404f46a86f40`

**Origin:** source review during PIXIE-TODO-040

**Claim under review:** `pixie_entity_view_bounds`, `pixie_entity_view_quad`,
and `pixie_entity_view_path` describe geometry as of the last render.

The [public header](../../../capi/include/pixie_capi.h) and
[reference](REFERENCE.md#picking-destruction-and-spawning) use that wording.
The source review found:

- [Capi.cpp](../../../capi/src/Capi.cpp), `DrawnTransform`, reads the
  current transform, `PreviousTransform2D`, and `session.Alpha()` when the
  query runs. The view functions also use the current camera.
- [ProjectSession.cpp](../../../runtime/src/ProjectSession.cpp),
  `RenderToTarget`, renders with `Alpha()`; `Advance`, `StepOnce`,
  `SetPaused`, field writes, and camera changes can alter query inputs
  without a subsequent render. The queried implementation does not use a
  retained frame-geometry snapshot.
- The shape branch of `pixie_entity_view_bounds` uses the interpolated
  position but calls `PickRadius` with the current transform. Changing
  scale between steps is another case for the same temporal-consistency review.
- The editor's [ViewportPanel.cs](../../../editor/PixieEditor/Ui/ViewportPanel.cs)
  calls advance, render, and then draws the selection. That order may keep
  ordinary outlines aligned; it does not establish the API's behavior for
  every host call order.

**Possible impact:** a host that changes state between rendering and querying
may receive geometry that differs from the displayed texture. This is a
source-level hypothesis, not an observed runtime failure.

**Verification needed:** in a host with an OpenGL context, render a known
entity, change its transform or advance without rendering again, and compare
the three queries with the retained frame. Include an interpolated scale and
a camera change. Check the ordinary render-then-query sequence separately.

**Disposition:** pending owner decision after verification: whether the
contract requires retained frame geometry or requires a specific host call
order. No implementation change, contract rewrite, or issue promotion was
performed by this documentation phase. The conflict is retained under the
[documentation stop rules](../../governance/documentation-policy.md#stop-rules).

### Owner disposition on 2026-10-18

The owner accepted the recommendation to retain geometry corresponding to
the last rendered frame for these three queries. The public header and
reference are preserved. The earlier pending disposition above describes
the observation's original state.

PIXIE-TODO-041 investigates the mismatch using disposable projects and
records possible corrections and their costs. It does not authorize a
product behavior change: the owner's earlier restriction to suggestions
remains in force. Runtime reproduction and its limits will be appended;
the contract decision alone does not confirm the finding.

### Author reproduction on 2026-10-18

The unchanged Release implementation was exercised on Windows with an
OpenGL 4.5 core context on an AMD Radeon RX 7600. After transform or camera
changes without another render, all three queries changed while a byte
comparison of the retained texture remained equal. Advancing only the
interpolation fraction changed a sprite's quad without another fixed step
or render. A separate same-frame circle case exposed the current-scale
radius alongside an interpolated drawing.

The probe reported eight contract assertion failures and zero control
failures; the existing `pixie-scene-capi` CTest passed. These are author-run
results, not independent confirmation or acceptance. The finding remains
unconfirmed pending independent review; no issue or implementation work was
promoted. The execution details, reproduction sequence, correction costs,
and evidence limits belong to
[PIXIE-ARCHIVE-076](../../archive/view-geometry-contract-investigation-2026-10-18.md).

The owner disposition above is settled: retain the last-render contract.
Possible corrections remain suggestions. Saving only the last camera and
interpolation factor cannot preserve geometry after its component inputs
change; a complete correction must account for those inputs and the API's
existing entity-lifetime rules.
