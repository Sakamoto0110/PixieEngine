# Documentation Aligned with Void Agent Workplace and the Family

**Document ID:** ARCHIVE-VAW-ALIGNMENT-20261012

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** the documentation rules brought in from Void Agent Workplace and from PixieLib and PixieInspector

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-011

**Type:** decision

**Origin:** owner decision

**Author:** claude

**Reference date:** 2026-10-12

**Commits:** with this record (parent `46ee6aa4d893fc14c6ca26d8b13fce051474e179`)

**Current state:** [`docs/governance/documentation-policy.md`](../governance/documentation-policy.md), [`docs/development.md`](../development.md)

The owner asked to look at Void Agent Workplace, whose contracts are still in
validation but whose documentation contracts are solid, and to apply it here
with the necessary adjustments, since PixieLib and the editor already follow
it.

What was found: Void Agent Workplace had changed only its agent procedure
(mandatory reads before documenting) since 2026-10-09. PixieLib and
PixieInspector had derived their documentation from this repository at
`46ee6aa`, as documentation-only derivations not bound to a profile, and added
practices of their own.

Applied here:

- From Void Agent Workplace's documentation references: an authority traversal
  order, stop rules (replacing the shorter conflict-handling section), and a
  self-review checklist, as sections of the policy.
- From the family: `docs/development.md` (working agreements, build, checks,
  dependencies, commits), which takes the build instructions out of the
  README; the commit identity named; work on `main`, with the rule for a
  session limited to its own branch; `.local/` in disposable environments; the
  evidence rules for measurements and out-of-repository consoles; code and
  comments in English; imported records keep their language.
- Facts that had changed: the editor's inspector was renamed from
  InteractiveEditor to PixieInspector, and its repository from
  `InteractiveEditor_Rework` to `PixieInspector`; the README, roadmap, and
  proposals use the new name, while earlier archive records keep the names of
  their baselines. PixieLib 0.2.0 defines identical layouts in both languages,
  which resolves the mismatch proposal PIXIE-PROPOSAL-002 recorded.
  `DEPENDENCIES.md` lists PixieLib 0.2.0 and the stale local checkout of
  PixieInspector, and GLM left the candidates because PixieLib brings it.

Not applied: Void Agent Workplace's records, campaigns, and premises, frozen in
its validation, and a binding of this repository to a profile, which does not
exist for the Pixie family.
