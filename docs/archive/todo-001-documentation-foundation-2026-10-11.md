# PIXIE-TODO-001: Documentation Foundation

**Document ID:** ARCHIVE-TODO-001-20261011

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completed PIXIE-TODO-001, the owner's review of the documentation policy, and the corrections it produced

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-008

**Type:** completion

**Origin:** PIXIE-TODO-001

**Author:** claude

**Reference date:** 2026-10-11

**Commits:** `f081b81ddd4ec3c9301a28aba581bb1a94681dbf`, and with this record (parent `ccf1db9e7537f20f3d9eb127c492984ee1eefa5e`)

**Original path:** `TODO.md`

**Current state:** [`docs/governance/documentation-policy.md`](../governance/documentation-policy.md)

## Entry as completed

**Status:** completed on 2026-10-11

**Source:** owner decision on 2026-10-09

**Accepted scope:** the root `README.md`, `ROADMAP.md`, `TODO.md`,
`TASKLIST.md`, and `DEPENDENCIES.md`; the documentation policy, schema,
templates, proposal index, and archive under `docs/`; and the unversioned
`.local/` convention. The model is derived from NekoLib's and adapted. No code,
build files, or modules.

**Dependencies and gates:**

1. Owner review of the structure and the policy: done on 2026-10-11.
2. The initial commit requires separate owner authorization: authorized and
   made on 2026-10-09 as `f081b81`.

**Breakdown:** none

## Review of the policy (2026-10-11)

Corrections applied without a decision, because they fixed stale text or wrote
down practice already followed:

- The build-configuration owner became the CMake files; the build system had
  been decided on 2026-10-10.
- The authority table gained third-party source (`external/<name>/`) and the
  rule that generated output (`build/`, `bin/`, the root `PixieEngine.slnx`) is
  never authority and never versioned.
- A Dependencies section gives the meaning of the `DEPENDENCIES.md` statuses,
  its `Last checked` date, candidates, and hashes of vendored code.
- Completing a `TODO.md` entry is described as it was done for PIXIE-TODO-002;
  a change to a promoted entry's scope needs a dated owner decision.
- Proposals gained an optional `Outcome` field, already used by
  PIXIE-PROPOSAL-001.

Owner decisions on 2026-10-11:

- `docs/archive/` is the history of the repository: what changed, what was
  completed, and why. As a consequence, modules have no `HISTORY.md`; a record
  about a module carries its key in `Boundary`. This reverses the earlier
  recommendation to keep a per-module history, because two owners of history
  would break the one-owner rule.
- `.local/` is for quick, unversioned use, and everything in it is safe to
  delete.
- Modules: left to the documentation's judgment. A module is created when its
  boundary is decided, not when code appears; the provisional `engine/` library
  has no module documentation until the module split is decided.
- Samples: one `README.md` in the sample's directory. A scenario (proves a
  claim) or an oracle (breaks something on purpose) also documents the claim
  or failure, the expected outcome, and how to read the result.
- Commit conventions are written into the policy.
- Every archive record identifies what produced it: `Archive ID`, `Type`,
  `Origin`, `Author` (`owner`, `claude`, or `codex`), `Reference date`, and
  `Commits`. Every event gets its own record, however small, and
  `docs/archive/README.md` became a router with one row per record, carrying
  enough to find the record without reading it. The events from 2026-10-09 to
  2026-10-11 were reconstructed as PIXIE-ARCHIVE-001 to PIXIE-ARCHIVE-008, in
  the same model as NekoLib's history: a timeline of identified events, each
  pointing to its evidence.

## Verification

- Every Markdown document outside `external/` has the metadata the schema
  requires, checked by a script on 2026-10-11; no verifier exists in the
  repository.
- Every relative link and anchor resolves, checked by the same script.
- `docs/schemas/documentation-schema.json` parses as JSON.
