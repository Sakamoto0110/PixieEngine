# Last-Render Geometry Contract: Decision and Investigation

**Document ID:** ARCHIVE-VIEW-GEOMETRY-CONTRACT-INVESTIGATION-20261018

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** owner disposition and bounded author reproduction of the C API view-geometry observation

**Surface:** archive-record

**Boundary:** capi

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-076

**Type:** completion

**Origin:** PIXIE-TODO-041

**Author:** codex

**Reference date:** 2026-10-18

**Commits:** with this record (parent `2365251e7ea50fe730e937c03bdd404f46a86f40`); uncommitted at verification

**Reference commit:** `2365251e7ea50fe730e937c03bdd404f46a86f40`

**Current state:** [C API reference](../modules/capi/REFERENCE.md#picking-destruction-and-spawning) and [CAPI-FINDING-001](../modules/capi/FINDINGS.md#capi-finding-001--view-geometry-and-the-last-render)

## Owner decision and scope

On 2026-10-18 the owner accepted the recommendation to preserve geometry
corresponding to the last rendered frame for `pixie_entity_view_bounds`,
`pixie_entity_view_quad`, and `pixie_entity_view_path`. Their public
declarations and reference wording were retained. The earlier restriction
to suggesting behavior changes remains in force; this investigation does
not implement a remedy or schedule one.

Work continued on `codex/pixie-docs-phase1` in the isolated worktree, on top
of the uncommitted documentation phases. The primary checkout and its
sample saves were not used as test projects. The pending promotions file
remained outside the scope.

VAW `c9d81ba1fe538ddaaaa44f2ca48e1dc9d3dcfcb1`, version 0.2.0.0,
resolved `pixieengine`/1 as `bound`, with build, capi, documentation, editor,
engine, runtime, and samples checked and no unchecked route. Binding is
local routing evidence and grants no publication or implementation authority.

## Entry as completed

### `PIXIE-TODO-041` — Preserve the last-render geometry contract and investigate its implementation

**Status:** completed

**Source:** owner acceptance on 2026-10-18 of the recommendation to retain the last-render contract and reproduce CAPI-FINDING-001 before selecting a correction.

**Accepted scope:** preserve the existing public contract; reproduce the view-bounds, view-quad, and view-path observations with a disposable project, including transform, camera, and interpolation changes; inspect the cost and scope of possible corrections; append the decision and bounded evidence to the C API findings and an archive record. Test instruments, generated projects, builds, and logs stay in ignored worktree-local paths. Product behavior changes remain suggestions under the owner's earlier restriction.

**Dependencies and gates:** use the separate branch and freshly resolved VAW binding; reuse the installed toolchain and existing dependencies; do not change implementation, API declarations, sample saves, historical records, or ROADMAP-PROMOTIONS-PENDING.md. Native reproduction requires a working local OpenGL context; its result is author evidence, not independent acceptance.

**Completion criteria:** the contract decision is recorded; reproduction outcomes and their controls, build provenance, and limits are explicit; any correction remains a proposal with its cost and open design questions; the documentation verifier and diff checks pass; excluded files remain unchanged.

**Breakdown:** none

## Executed build and controls

The installed Visual Studio 2026 toolchain was used: MSVC 19.51.36260.0,
toolset directory 14.51.36231, Windows SDK 10.0.26100.0, x64, Release.
Only existing vendored dependencies were built; none was adopted or updated.

From the worktree, using the Visual Studio installation's CMake tools:

```text
cmake --preset vs2026 -DPIXIE_BUILD_SAMPLES=OFF
cmake --build --preset vs2026-release --target pixie-scene-capi capi-test --parallel 4
ctest --preset vs2026-release -R ^pixie-scene-capi$ --output-on-failure
```

Configure and both build targets exited 0. The existing CTest passed 1/1
in 1.74 seconds, including its project, sprite, path, and text checks.
This is an existing-suite control, not an independent test of the finding.

The additional probe is a worktree-local copy of
[CapiTest.c](../../tests/capi/CapiTest.c), with appended cases and the
existing input-scroll export loaded. It uses the same dynamic-library
loading, GLFW context, public C API, and texture-read helpers. Its small
local CMake project links the already-built GLFW library. It built with
`/W4 /WX` and ran in a hidden window.

The driver reported OpenGL `4.5.0 Core Profile Context 26.8.1.260810`,
renderer `AMD Radeon RX 7600`, vendor `ATI Technologies Inc.`.
The disposable project and all probe artifacts remain under
`.local/view-geometry-20261018/`; they are machine-local evidence, not
versioned authority or a clone-portable test suite.

## Reproduction sequence and observations

Create a fresh project using `pixie_project_create`, then open it at
640 by 360 pixels in edit mode. Add three named scene entities through the
C API, each with `Transform2D`:

- A red circle at `(-120, -60)`, with `Shape.size = (10, 10)` and unit scale.
- A sprite at `(0, 40)`, using the existing C API test's 4 by 4 green PNG,
  with scale `(10, 10)` and the default center pivot.
- A path at `(60, -40)`, with local points `(0, 0)` and `(60, 0)` and width 4.

Render and query the circle bounds, sprite quad, and path line. Controls
check their expected view coordinates, the circle and sprite colors in the
texture, and repeated-query equality without mutations.

For each of the first three mutations below, retain the render target,
read all 640 by 360 RGBA pixels before and after, and compare them with
`memcmp`. Query geometry again without calling render. Render between
cases where specified so the next case has a fresh reference frame.

| Case | Texture comparison | Query observation |
|---|---|---|
| No mutation | Equal bytes | All three results unchanged; circle `(200, 120, 10)`, sprite first X `300`, path first X `380`. |
| Translate each entity by `(40, 10)`, without rendering | Equal bytes | Circle changes to `(240, 130, 10)`; sprite first X changes `300` to `340`; path first X changes `380` to `420`. |
| Render the translation, then scroll at view center by `+1` and advance by zero in edit mode, without rendering | Equal bytes | Circle changes `(240, 130, 10)` to `(228, 122.5, 11.5)`; sprite first X changes `340` to `343`; path first X changes `420` to `435`. |
| Give the sprite velocity `(120, 0)`, play, advance by `1.5/60`, render, then advance by `0.25/60` without rendering | Equal bytes | Fixed-step count remains 1; sprite first X changes `344.15` to `344.725`. Circle and path remain unchanged. |
| Drop the remainder by pausing and resuming, advance by `0.5/60`, set the circle scale to `(3, 3)`, then render and query | Same-frame pixel measurement | Red interior has measured horizontal radius `22` pixels; interpolated geometric radius is `23`, including antialiased edge allowance. The bounds query reports `34.5`, from current scale 3. |

The probe exited 1 with eight contract assertion failures: three after
translation, three after camera movement, one after interpolation-only
advance, and one for the same-frame radius comparison. Its setup, texture
equality, known-coordinate, pixel-color, and mutation controls reported
zero failures. Eight assertions do not mean eight independent defects.

The same-frame radius measurement counts pixels with red above 240 and
green/blue below 8; the one-pixel radius allowance covers the antialiased
edge. It is not an exact reconstruction of every rasterized boundary.

These results reproduce the observed mismatch in this author-controlled
fixture. They are not independent confirmation or acceptance. The finding
stays unconfirmed pending independent review; it was not promoted to an
issue or an implementation task.

## Correction scope and cost: suggestions only

[Capi.cpp](../../capi/src/Capi.cpp) recomputes geometry from the current
components, camera, and `Alpha()`. [ShapeDrawing.cpp](../../runtime/src/ShapeDrawing.cpp)
interpolates scale for drawing, while the bounds query passes the current
transform to `PickRadius`. [SpriteDrawing.cpp](../../runtime/src/SpriteDrawing.cpp)
and [PathDrawing.cpp](../../runtime/src/PathDrawing.cpp) show the existing
render traversals where a future capture could be considered.

- Retaining only camera and interpolation factor is insufficient: the
  transform and component inputs may have changed after rendering.
- Using the interpolated transform for the circle radius would address
  that same-frame symptom; it would not preserve geometry after later
  state changes and must not be presented as the complete fix.
- A complete candidate retains the minimal geometry or geometry inputs
  required by these C API queries for a frame, rather than cloning the
  World. For `N` queryable entities, `P` copied path points, and `T` copied
  text bytes, a direct snapshot of geometry inputs has order `N + P + T`
  time and storage cost. Retaining computed geometry instead changes the
  processing/storage tradeoff; image sizes and font metrics also need frame
  consistency. This is a source-based estimate, not measured overhead or FPS.
  Reusing existing traversals may avoid an extra scan, but still adds
  retention work. Keeping it specific to C API hosts would avoid imposing
  that cost on player-only sessions.
- Capturing only submitted visible entities could reduce retention, but
  the current query implementation also accepts hidden or offscreen
  entities. That narrower cache cannot silently redefine the contract.
  Before implementing, specify before-first-render behavior and preserve
  existing not-found rules, handle generations, and project/reset/resize
  lifetimes; avoid returning geometry for a reused handle from another World.

The recommended next implementation proposal is minimal C API frame
retention with those lifetime rules and measured CPU/frame-memory costs
on a large circuit. Independent reproduction should precede that proposal's
promotion. No cache design was selected, no performance gain claimed, and
no simulation or picking behavior changed in this investigation.

## Artifact identity and evidence limits

SHA-256 values at this execution:

| Artifact | SHA-256 |
|---|---|
| `bin/Release/pixie-scene-capi.dll` | `b3756d6b1992743959ed83b62594a75ffc25c69e8799e0f75faffebdb79b4c2e` |
| `bin/Release/capi-test.exe` | `99f55f0ef845e9e4cc02dcbdc8d534f94dd32614d0f4b5d68b50ac7767d79514` |
| Local `Probe.c` | `a921a4aece57f4fafaf04723b9acced23cc15e1e2a8dd3acd2f3235225eeed77` |
| Local `ProbeCases.c` | `a817a653e97a95b7f09c8033f563c7aed867854b1b7759666b6289c8819841a3` |
| Local `view-geometry-probe.exe` | `1650253b4f3a7ad03d03d8f9b11d902ccaa1b66072f3bf0b2295201b8d43d839` |

Logs are `configure.log`, `build-release.log`,
`control-ctest-release.log`, `probe-configure.log`,
`probe-build-release.log`, and `probe-release.log` under the local evidence
directory. `probe-exit-code.txt` records 1. Earlier documentation archive
records keep their original no-runtime-execution scope; this is a later,
separately authorized investigation.

No Debug build, full CTest suite, editor UI session, alternate platform/GPU,
independent review, performance benchmark, or product correction was run.
Sample saves, dependencies, API declarations, and implementation sources
were unchanged. No commit, push, or merge was performed.

## Final verification on 2026-10-18

- `python -B scripts/check_docs.py`: 120 documents checked, 0 problems.
- `git -c core.safecrlf=false diff --check`: no whitespace errors.
- The cumulative documentation change set contains exactly 19 Markdown
  paths; all native instruments, generated projects, build products, and
  logs are ignored. There are no implementation or public API changes.
- All 73 archive records from the reference commit retain their normalized
  bodies. Phase 1 and phase 2 records retain their raw-byte hashes,
  respectively `55c1dabe382acf6c1cccbb312dae7296915ffa2de1f080bfbedc2fac69b5a31e`
  and `3ecb3c295ce3246af3d24261e28ccdb1c2a5f8ec67966f7c2817b22cc787f7df`.
  The archive router preserves its earlier prefix and now ends at 076.
- The 41 completed TODO IDs are ordered; all 38 pre-sanitation completion
  targets and dates and both sanitation pointers are preserved. All 12
  proposal rows, existing Document IDs, and roadmap guardrails are unchanged.
- All 325 relative link occurrences in the changed documents match actual
  directory-entry casing, in addition to the verifier's link/anchor checks.
- The primary checkout's tracked files, branch, and HEAD are unchanged.
  Protected-file SHA-256 values still match the initial observations:
  `48a80859917788b027c5d5d6407e85c455b0061c50031de00c5235f075162259`
  for ROADMAP-PROMOTIONS-PENDING.md and
  `0ed31d37ac73d09d4cfb91e92533821d5cde28cd39941e7f8e4af8d019b8d1a8`
  for CLAUDE-SESSION-REPORT.md.
- The local verification receipt also cross-checks the recorded artifact
  hashes, probe cases and exit code, and the existing CTest result. It is
  author evidence; no independent acceptance is claimed.
