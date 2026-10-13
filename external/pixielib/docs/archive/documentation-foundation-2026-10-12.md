# Documentation Foundation

**Document ID:** ARCHIVE-DOCUMENTATION-FOUNDATION-20261012

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** the adoption of the documentation policy derived from PixieEngine, PixieInspector, and Void Agent Workplace, completing PXL-TODO-001

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PXL-ARCHIVE-007

**Type:** completion

**Origin:** PXL-TODO-001

**Author:** claude

**Reference date:** 2026-10-12

**Commits:** `9601cea682f1fde07fc6e7c7914f9cce1c877d21`, `94a1a839c0368b7d82dc224851c820d27c7e99e2`, `78308961d5e81b0be144480bb0d9b04f85dba0f4`, `c61a0616a46867164df4027b7bdd55ed10e70b1e`, `6a7a482ede6b4477f7bb44f8798d4aac54a56c8e`, `e9f6d0fe47025fb5ccd4a59904cd4f4b3cb509ab`, `a858cdd1aaac11e8921df7577a590abd69871abf`, and with this record (parent `a858cdd1aaac11e8921df7577a590abd69871abf`)

**Current state:** [`docs/governance/documentation-policy.md`](../governance/documentation-policy.md), [`docs/schemas/README.md`](../schemas/README.md)

## What happened

On 2026-10-12, after PixieLib's point and vector were separated and the
segment added, the owner asked PixieLib to adopt the documentation model of
its family. PixieInspector had adopted it the same day, derived from
PixieEngine's, which follows a partial derivation of Void Agent Workplace
(VAW): the extraction of NekoLib's documentation and agent infrastructure,
frozen while it is validated. The owner granted read access to VAW,
PixieEngine, and PixieInspector the same day.

The work followed the four steps of VAW's documentation-authoring contract
(`nekolib.documentation-authoring/1`): `inventory/design` produced a migration
map, which the owner accepted by answering its nine numbered questions (10.1 to
10.9, all yes); then `migrate-structure`, `populate-module` for each of the
four boundaries, and `validate-module`. Like PixieEngine's and
PixieInspector's, it is a documentation-only derivation: the repository is not
bound to a VAW profile.

## Decisions taken with the map

1. The derivation follows PixieEngine's and PixieInspector's, the owner's
   latest decisions for the family. Current documents are in English, and the
   documents written before the policy are archived whole, in Portuguese,
   under an English header (10.1).
2. The modules are concepts, each covering both languages where both have it:
   `primitives`, `math`, `events`, and `generator` (10.2).
3. Stable IDs use the prefix `PXL-` (10.3).
4. `docs/layout.md` stays at its path, in English, as a global normative
   reference, so the 36 source comments that cite it keep working (10.4).
5. `docs/notas.md` becomes a portal to PXL-ARCHIVE-001, so the 18 source
   comments that cite it keep working, with no change to the source (10.5).
6. Open questions live in `.local/`, or in the conversation when `.local/`
   does not survive the session (10.6).
7. The schema's differences from PixieEngine's: both languages in a manifest,
   the `header-only` and `analyzer` artifacts, the `package` and `source`
   distributions, `Target frameworks`, and the public API as headers plus
   assembly (10.7).
8. Four ideas the notes left for later become proposals, none promoted; the 3D
   with the quaternion and the terminal module go to the roadmap (10.8).
9. The commit rules stay, with the session-branch flow written down (10.9).

## The entry, as it closed

### `PXL-TODO-001` — Adopt the documentation policy

**Status:** completed

**Source:** owner decision on 2026-10-12, accepting the migration map of the
`inventory/design` step (questions 10.1 to 10.9)

**Accepted scope:** the documentation policy, schema, and templates derived from
PixieEngine's, with PixieInspector's .NET additions; the six documents written
before the policy archived whole, in Portuguese, as imported records; the notes
portal; the four ideas the notes left for later as proposals; the root
documents, the development guide, and the layout contract in English; the
module documents (`MANIFEST.md`, `README.md`, `REFERENCE.md`) of `primitives`,
`math`, `events`, and `generator`, written from the current source. Out of
scope: any change to product code or public API, and the promotion of any
proposal.

**Dependencies and gates:** the owner accepts the migration map; met on
2026-10-12.

**Completion criteria:** every document named in the scope exists with valid
metadata and resolving links; every module reference is checked claim by claim
against the source; no product code changes; the completion is archived.

**Breakdown:**

- `PXL-TASK-001` — **Status:** done — Archive the six documents written before
  the policy, whole, as PXL-ARCHIVE-001 to 006 (`9601cea`).
- `PXL-TASK-002` — **Status:** done — The policy, schema, templates, archive
  router, the notes portal, and the four proposals (`94a1a83`).
- `PXL-TASK-003` — **Status:** done — The root documents, the development
  guide, the documentation index, and the layout contract in English
  (`7830896`).
- `PXL-TASK-004` — **Status:** done — The module documents of `primitives`
  (`c61a061`).
- `PXL-TASK-005` — **Status:** done — The module documents of `math`
  (`6a7a482`).
- `PXL-TASK-006` — **Status:** done — The module documents of `events`
  (`e9f6d0f`).
- `PXL-TASK-007` — **Status:** done — The module documents of `generator`
  (`a858cdd`).
- `PXL-TASK-008` — **Status:** done — The self-review of each module, then
  this record.

## Verification

- Each module reference was written from the source and checked claim by
  claim against it. Where a first draft went beyond the source, it was
  corrected before the commit: an example of a rejected scalar used the point,
  which has no `*` since 8.1; the integer vectors were said to have no `Lerp`
  on both sides, while GLM's `glm::mix` takes an `ivec2`; the platforms named
  macOS, which nothing has checked.
- The examples in the module introductions and the README's C# example were
  compiled and run, in C++ (g++, with `-Werror`) and in C#, and do what their
  comments state; the generator's is a template fragment and was not compiled.
- A review script checked every Markdown file against the schema: the required
  metadata and its vocabulary, the surface rules, unique document IDs, the
  manifest fields and their vocabulary, the paths in each manifest, and every
  relative link and anchor. After this record: 43 files, 35 with metadata, no
  problem found. A copy with a broken anchor, a bad `Surface`, and a bad
  `Distribution` was caught, so the script detects all three. It is a review
  aid, not the verifier the policy says does not exist yet.
- The bodies of PXL-ARCHIVE-001 to 006 were compared with the documents at
  their reference commits and are identical.
- No product code changed: `cpp/`, `dotnet/`, and `.github/` are the same as
  at `e0186c6`. The C++ tests still pass.
- Not checked: the rendered look of the documents on GitHub, and links into
  other repositories.

## Observations

- `DEPENDENCIES.md` records `Microsoft.CodeAnalysis.CSharp` at 4.8.0 while
  5.9.0 exists; no decision about that version is recorded. It is not a
  defect: the generator only runs while PixieLib builds.
- The `GIT_TAG v0.1.0` of the README's C++ example stays until 0.2.0 is
  published, as the development guide says.
- The remote branch of the session that made this migration could not be
  deleted from the session; it is an observation about the environment, not a
  repository fact.
