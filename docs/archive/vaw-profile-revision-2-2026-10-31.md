# VAW Profile Revision 2 and Product Versioning

**Document ID:** ARCHIVE-VAW-PROFILE-REVISION-2-20261031

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** the owner's adoption of VAW workflow, premises, validation and versioning capabilities for PixieEngine, and the engine's first product version

**Surface:** archive-record

**Boundary:** global

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-089

**Type:** decision

**Origin:** owner decision

**Author:** claude

**Reference date:** 2026-10-31

**Commits:** code `c5454c6f9fac480397f0239d6ec18437af94b612`; VAW `52694e7`, `53c7067`, `05b04c2` on its local branch `codex/pixie-workplace`

**Current state:** the [development guide](../development.md#void-agent-workplace), its [product version](../development.md#product-version) section, and the [documentation policy](../governance/documentation-policy.md#authority)

## Decision

On 2026-10-31 the three Pixie repositories moved into the VAW checkout's managed
Pixie family (`projects/Pixie/`). The owner then asked to "bring the heavy
cavalry": the `pixieengine` profile, until then binding and skill discovery only
([PIXIE-ARCHIVE-070](vaw-binding-2026-10-16.md)), gains VAW's working
capabilities. Asked whether to version everything, the owner answered "n quer
começar a versionar tudo?" and approved the design ("Yes, write it").

- Revision 2 adds workflow state, continuity checkpoints, premises, repository
  state, validation records, package validation and versioning, with routes for
  every source area and the full matrix as its validation, and two skills of
  this profile, `pixieengine-documentation` and `pixieengine-qualification`.
- Campaigns stay frozen: the owner explained they ran together with premises,
  as long rounds of operations that blocked other work, and that nested
  campaigns made them a poor fit beside `TODO.md` and `TASKLIST.md`.
  Execution control stays out until VAW qualifies it.
- VAW records live in `.local/vaw/` and are never authority.
- The engine's product version is owned by `version.json`, starting at
  `0.1.0.0` (it was never released or tagged). Below 1.0 a break raises `MINOR`;
  this rule is VAW's `void.versioning/2`, added for it (VAW 0.3.0.0; its
  `nekolib` profile moved to revision 8 to select it, changing nothing else).
  PixieInspector starts at `0.3.0.0` and PixieLib at `0.2.1.0`, their published
  versions; their package `Version` is derived as the public projection, so tags
  and NuGet versions keep their form.

## Verification and evidence limits

- VAW: `verify-platform` valid; `verify-profile-adapters` valid for
  `pixieengine`/2; 14 of its 16 test files pass. `ControllerTransport` and the
  `Adapter` file that reruns it fail on a Unicode stream case, before and after
  these changes alike, in execution control, which is not adopted. VAW's record
  validation had been failing on PowerShell 7.6 (sibling schema references);
  `52694e7` fixed it.
- Binding: `pixieengine`/2 resolves `bound` with all 13 routes; PixieInspector
  and PixieLib resolve at revision 1. All three are in the explicit registry
  `.local\bindings\pixie.json` of the VAW checkout (SHA-256
  `E94BE5F7FC80F704200D95050EF07C689BC00573E36ABF6C0E6FD31FF904630D`). Claude and
  Codex list the same seven skills.
- One real workflow cycle under `.local/vaw/records/`: created, started, paused
  with a checkpoint, the checkpoint verified compatible, resumed and completed;
  a stale expected revision was refused; a byte added to an owned path made the
  checkpoint incompatible, and its byte-identical restoration compatible again.
- At code `c5454c6`, on the owner's machine (Ryzen 7 5700X, RX 7600, Visual
  Studio 2026): vs2026 and ci built and passed 22 of 22 CTest tests in Debug and
  Release, including the export qualification, which now requires
  `engine_version` in the manifest and `PixieEngine <version>` in the player's
  log; the editor built with no warning in Debug and Release, with assembly
  version `0.1.0.0`; the development-channel qualification passed 170 cases in
  Debug and in Release (`--skip-cold`); `boids.exe` logged `PixieEngine
  0.1.0.0`. The vs2026 builds reported MSB8028 (an intermediate directory shared
  with another project) for `coupling-lab` and `physics-benchmark`; they were
  not seen before the repository moved and are left for a clean rebuild.
- `PIXIE-ARCHIVE-088` is taken by the 2D lighting completion on its unmerged
  branch; this record takes the next number.
- PixieInspector and PixieLib build with no warning; MSBuild reports `0.3.0` /
  `0.3.0.0` and `0.2.1` / `0.2.1.0`; PixieLib's checks pass. Their commits are
  local; nothing was pushed.
