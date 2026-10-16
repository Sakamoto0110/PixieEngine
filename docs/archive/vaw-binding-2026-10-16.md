# Void Agent Workplace Binding Adopted

**Document ID:** ARCHIVE-VAW-BINDING-20261016

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** the repository bound to a machine-local Void Agent Workplace profile for project binding and skill discovery, as the smallest live adoption of VAW in development

**Surface:** archive-record

**Boundary:** global

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-070

**Type:** decision

**Origin:** owner decision

**Author:** claude

**Reference date:** 2026-10-16

**Commits:** with this record (parent `2ff0e5112f1f07197ac51302d12acf3e913bae14`)

**Current state:** the [development guide](../development.md#void-agent-workplace)

## How it arrived

The owner handed over a pilot plan for adopting Void Agent Workplace (VAW) in the Pixie family,
starting with this repository, and asked which VAW doctrine was already adopted and for the
smallest addition that would exercise VAW in real development, shown before it was applied. The
plan named PIXIE-TODO-037 as the first task; it had already been completed, pushed as `2ff0e51`,
and verified by the CI, so it could not serve as pilot evidence.

## What was already adopted

Doctrine only, recorded in PIXIE-ARCHIVE-001 and PIXIE-ARCHIVE-011: the documentation model
derived from NekoLib's schema version 1, with VAW's authority traversal, stop rules, and
self-review (observed at VAW `c9d81ba`); one scheduler (`TODO.md`, `TASKLIST.md`), proposals, and
archive records; and verification by Debug and Release builds, CTest, the CI, and the
documentation verifier. No VAW mechanism ran: no profile, binding, adapter, records, campaigns, or
premises, as the policy and the schema index said.

## Decision

The owner approved the smallest runtime addition: project binding and skill discovery. Applied:

- **A profile outside both repositories**, `pixieengine` revision 1, in
  `C:\Users\Sakamoto\dev\Void Agent Workplace\.local\profiles\pixieengine\` (ignored by that
  repository's Git, so machine-local), read by VAW's source commands through `-ProfilesPath`; the
  pinned VAW checkout is not edited. It maps VAW's authority roles to this repository's documents,
  routes seven source areas to the documents that own them, composes only `project-binding` and
  `skill-discovery` version 1, has no native skills of its own, and lists this repository's checks
  as its validation.
- **The binding**, `origin` (`remote:github.com/Sakamoto0110/PixieEngine`) to `pixieengine`/1, in
  the default registry, `%LOCALAPPDATA%\VoidAgentWorkplace\bindings.json`.
- **The procedure before a task**, in the development guide: resolve the binding for the task's
  routes and list the Claude skills; the task's archive record states what was resolved.
- **The two statements that the repository was not bound**, in the documentation policy and the
  schema index, reconciled with the binding's scope.

Not adopted: an installed Claude adapter package (the source CLI is enough for discovery),
workflow records, continuity checkpoints, campaigns (frozen in VAW), premises, versioning, and
packaging. PixieLib and PixieInspector are separate owners and were not bound.

## Evidence

VAW checkout `C:\Users\Sakamoto\dev\VAW` at `c9d81ba1fe538ddaaaa44f2ca48e1dc9d3dcfcb1`, version
0.2.0.0, unmodified (its untracked `.vs/` kept), run with PowerShell 7.6.6:

- `register-project-binding.ps1 ... -WhatIf` reported `would_register` with compatibility `all`;
  the registration then reported `registered`.
- `resolve-project-binding.ps1` reported `bound`, with all seven routes checked; with
  `-CompatibilityScope Routes -RouteId runtime,documentation`, `bound_scoped`, checking
  `documentation` and `runtime` and leaving the other five unchecked.
- `vaw.ps1 skills list -Agent claude` listed `project-binding` alone, with "common requirements
  only" as its binding compatibility; `skills show project-binding` returned its instructions and
  its two declared contracts.

## VAW friction found

- Resolution failed with "The property 'validation' cannot be found on this object" while the
  profile had no `validation` field, which the profile schema declares optional
  (`src/VoidAgent.Binding`, `@($profile.validation)` under strict mode). Registration had accepted
  the same profile. The profile now lists this repository's checks, which it should anyway.
- A profile outside the VAW checkout cannot use the relative `$schema` paths the bundled profiles
  use, so it omits `$schema`.
