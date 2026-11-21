# Technical Documentation: Sanitation Phase 2

**Document ID:** ARCHIVE-DOCUMENTATION-SANITATION-PHASE2-20261018

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** bounded reconciliation of runtime, C API, and editor documentation with their implementation sources

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-075

**Type:** completion

**Origin:** PIXIE-TODO-040

**Author:** codex

**Reference date:** 2026-10-18

**Commits:** with this record (parent `2365251e7ea50fe730e937c03bdd404f46a86f40`); uncommitted at verification

**Reference commit:** `2365251e7ea50fe730e937c03bdd404f46a86f40`

**Current state:** [runtime reference](../modules/runtime/REFERENCE.md), [C API reference](../modules/capi/REFERENCE.md), [editor guide](../../editor/README.md), and [C API findings](../modules/capi/FINDINGS.md)

## Scope and authority

The owner authorized continuing the proposed technical documentation audit
on 2026-10-18. Work remained on `codex/pixie-docs-phase1`, in the isolated
worktree, on top of the uncommitted [phase 1 changes](documentation-sanitation-phase1-2026-10-18.md).
The repository's [documentation policy](../governance/documentation-policy.md)
and schema retained their authority. No module boundary, planning model,
public declaration, implementation, or sample save was changed.

VAW `c9d81ba1fe538ddaaaa44f2ca48e1dc9d3dcfcb1`, version 0.2.0.0,
resolved `pixieengine` profile revision 1 with `bound` for all routes:
build, capi, documentation, editor, engine, runtime, and samples. No route
was left unchecked. This is local binding evidence, not runtime acceptance
or permission to publish.

## Entry as completed

### `PIXIE-TODO-040` — Reconcile technical documentation with its sources, phase 2

**Status:** completed

**Source:** owner authorization on 2026-10-18 to continue the proposed runtime, C API, editor, and root README documentation audit.

**Accepted scope:** compare module introductions, manifests, technical references, and editor/root READMEs with public headers, implementation, build definitions, tests, and existing accepted records; correct descriptive drift, consolidate repeated explanations through links, and record any remaining contract conflicts without changing behavior. Preserve phase 1 history and stable IDs. ROADMAP-PROMOTIONS-PENDING.md, code, dependencies, and sample saves remain outside the mutation scope.

**Dependencies and gates:** retain the separate branch and primary-checkout exclusions; VAW binding must cover every inspected source route; do not reinterpret an unresolved normative requirement as an implementation change.

**Completion criteria:** each correction has a source or accepted-record basis; duplicated explanations route to their owner; verification and evidence limits are explicit; the documentation verifier passes; changes remain documentation-only and excluded-file hashes stay unchanged.

**Breakdown:** none

## Reconciliation and source basis

| Documentation | Source basis | Correction or consolidation |
|---|---|---|
| Runtime introduction and manifest | [runtime CMake](../../runtime/CMakeLists.txt), [ProjectSession](../../runtime/include/pixie/runtime/ProjectSession.hpp), [ProjectApp](../../runtime/include/pixie/runtime/ProjectApp.hpp) | Hosted sessions already exist; drawing adapters belong to runtime while renderer implementations remain in the provisional engine; JSON is private to scene and project-file work. |
| Runtime field metadata | [ComponentTypes.hpp](../../runtime/include/pixie/runtime/ComponentTypes.hpp), [RuntimeComponents.cpp](../../runtime/src/RuntimeComponents.cpp) | Supported members include point lists, and built-in descriptors include scene text and its TTF asset field. |
| Runtime and C API timing, editor Session panel | [ProjectSession.cpp](../../runtime/src/ProjectSession.cpp), [FixedStepper.hpp](../../engine/include/pixie/FixedStepper.hpp), [Capi.cpp](../../capi/src/Capi.cpp) | The measured duration covers `World::Step`, excluding `BeforeStep`, rendering, and UI. Advance scales before the accumulator clamp; every `SetPaused` call drops the remainder. Shared timing details route to the runtime reference. |
| Runtime drawing | [ShapeDrawing.cpp](../../runtime/src/ShapeDrawing.cpp), [PathDrawing.cpp](../../runtime/src/PathDrawing.cpp), [SpriteDrawing.cpp](../../runtime/src/SpriteDrawing.cpp), [World.cpp](../../runtime/src/World.cpp), [culling completion](todo-037-view-culling-2026-10-16.md) | Culling filters submission and retains component traversal and sprite/text gathering and ordering. Visibility does not stop simulation. No whole-frame performance result is inferred. |
| Runtime image refresh and undo scope | [ProjectSession.cpp](../../runtime/src/ProjectSession.cpp), [SpriteImages.hpp](../../runtime/include/pixie/runtime/SpriteImages.hpp), [History.cs](../../editor/PixieEditor/Editing/History.cs) | Image changes refresh at the session check interval; editor undo uses entity snapshots and does not imply a runtime-owned undo stack. |
| C API surface and non-goals | [pixie_capi.h](../../capi/include/pixie_capi.h), [Capi.cpp](../../capi/src/Capi.cpp), [snapshot/name completion](todo-024-snapshots-2026-10-14.md) | Name editing is implemented, entity liveness is described, current ABI declarations route to the header, and version chronology routes to archives. |
| Editor rendering, picking, inspector, and loading | [ViewportPanel.cs](../../editor/PixieEditor/Ui/ViewportPanel.cs), [Picking.cpp](../../runtime/src/Picking.cpp), [PixieCapi.cs](../../editor/PixieEditor/Native/PixieCapi.cs), [GameLibrary.cs](../../editor/PixieEditor/Native/GameLibrary.cs), [Program.cs](../../editor/PixieEditor/Program.cs) | Rendering and hit-test semantics link to their native owners. Reads and edits use the native state; native-library discovery distinguishes projects with code, projects without code, and named games. |
| Editor dependencies and icon claims | [project file](../../editor/PixieEditor/PixieEditor.csproj), [DEPENDENCIES.md](../../DEPENDENCIES.md), [WindowIcon.cs](../../editor/PixieEditor/Ui/WindowIcon.cs) | Package versions route to the dependency owner. Supplying icon pixels is distinguished from validation on every platform. An undated throughput anecdote was removed. |
| Root README and validation routes | [engine CMake](../../engine/CMakeLists.txt), [C API CMake](../../capi/CMakeLists.txt), [C API test source](../../tests/capi/CapiTest.c), [runtime test sources](../../tests/runtime/) | The root introduction includes paths and text. Test descriptions index source coverage, with C API session checks owned by that module's reference; historical executions remain in archives. |

## Unresolved contract observation

[CAPI-FINDING-001](../modules/capi/FINDINGS.md#capi-finding-001--view-geometry-and-the-last-render)
records a source-level question about the view-geometry functions. The
header and reference describe geometry as of the last render, while queries
recompute from current transforms, interpolation state, and camera state.
The finding identifies the call-order and scale cases that need a host with
an OpenGL context to verify them.

The last-render contract was preserved. No defect was confirmed, issue
promoted, implementation scheduled, or host call-order requirement invented.
The [stop rule for disagreement](../governance/documentation-policy.md#stop-rules)
leaves that contract's disposition with the owner. This record closes the
bounded documentation audit, not that observation or its possible remedy.

## Evidence limits

Source reads and symbol/option comparisons establish only the checked static
relationships. Existing test code and accepted records were read; no native
build, automated runtime suite, editor session, GPU profile, or performance
measurement was executed. This work provides no new runtime result or
independent acceptance. The documentation verifier was not modified.

ROADMAP-PROMOTIONS-PENDING.md was not copied into this worktree or adopted as
planning input. It and CLAUDE-SESSION-REPORT.md remain unmodified in the
primary checkout. Phase 1's record and the earlier archive bodies are
preserved at their own baselines.

## Verification on 2026-10-18

- Documentation: `python -B scripts/check_docs.py` checked 119 Markdown
  documents and reported 0 problems. An initial run found a mistaken history
  filename in this record's source links; it was corrected to `History.cs`
  before the passing run.
- Diff hygiene: `git -c core.safecrlf=false diff --check` reported no
  whitespace errors.
- Scope: the cumulative phases 1 and 2 contain exactly 18 Markdown paths.
  No source, build, dependency, sample, or save path is in the change set.
- History: all 73 archive bodies from the reference commit retain their
  Git-normalized content. The phase 1 record's raw-byte SHA-256 remains
  `55c1dabe382acf6c1cccbb312dae7296915ffa2de1f080bfbedc2fac69b5a31e`.
  The original archive router is a preserved prefix with rows 074 and 075
  appended; row 074 is unchanged from phase 1.
- Planning conservation: all 38 earlier completion targets and dates, plus
  phase 1's pointer, are retained. The 40 completed IDs are ordered. All 12
  proposal rows are unchanged. Existing Document IDs and the roadmap's
  original prefix, guardrails, freezes, and goals are unchanged.
- Link casing: 315 relative link occurrences in the changed documents were
  checked against actual directory-entry names, separately from the
  documentation verifier's link and anchor checks.
- Static surface cross-checks: all 64 exported C functions are named in
  the C API reference; the editor's 15 option-table entries match its parser;
  native and managed ABI declarations both report version 8; all 38 header
  paths in the runtime entry-point table exist. These are coverage and
  consistency checks, not execution of those functions or options.
- Exclusions: SHA-256 checks in the primary checkout still report
  `48a80859917788b027c5d5d6407e85c455b0061c50031de00c5235f075162259`
  for ROADMAP-PROMOTIONS-PENDING.md and
  `0ed31d37ac73d09d4cfb91e92533821d5cde28cd39941e7f8e4af8d019b8d1a8`
  for CLAUDE-SESSION-REPORT.md. The primary checkout's tracked files,
  branch, and HEAD are unchanged from the observed baseline.
- These are author checks. No native build, automated runtime suite,
  application session, independent acceptance, commit, push, or merge was
  performed for this documentation change. The geometry observation remains
  unconfirmed and its contract disposition remains pending with the owner.
