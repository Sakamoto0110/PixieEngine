# Explicit VAW Binding Registry

**Document ID:** ARCHIVE-VAW-EXPLICIT-REGISTRY-20261016

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** an explicit workspace-local registry for VAW discovery across sessions

**Surface:** archive-record

**Boundary:** global

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-071

**Type:** fix

**Origin:** owner decision

**Author:** codex

**Reference date:** 2026-10-16

**Commits:** with this record (parent `4cf6daebd485f38f87bf09be7bcbc59e14bb9216`)

**Current state:** the [development guide](../development.md#void-agent-workplace)

## Problem and correction

The adoption in [PIXIE-ARCHIVE-070](vaw-binding-2026-10-16.md) used the default
AppData registry. Claude reported PixieEngine's entry there, while Codex could
read only a different, older registry at the same absolute path. The reads
succeeded but their hashes and contents differed. The underlying filesystem
mechanism was not established; changing the default registry was unnecessary.

The owner requested a working correction. A dedicated registry was registered at
`C:\Users\Sakamoto\dev\Void Agent Workplace\.local\bindings\pixieengine.json`,
next to the existing machine-local profiles. The development procedure now passes
that exact path through `-BindingsPath` for resolution and skill discovery.
It retains the existing `pixieengine`/1 profile, the seven routes, and VAW
`c9d81ba1fe538ddaaaa44f2ca48e1dc9d3dcfcb1` without source changes.

The new registry contains only origin / remote:github.com/Sakamoto0110/PixieEngine
to pixieengine/1. Its SHA-256 is
`78683105760A76220C847796234DC5520ACCCF9538616711F3EBF5F27298EAAB`,
matching the registry content Claude reported. Neither AppData registry was
changed. The historical adoption record remains unchanged.

## Verification and scope

Full resolution with the explicit registry returned `bound`, checking all seven
routes. Scoped resolution returned `bound_scoped`, checking `documentation` and
`runtime`. The source CLI's Claude catalog returned the single `project-binding`
entry, and `skills show project-binding` returned its instructions; both results
were complete. The documentation verifier checked 113 documents with zero
problems, and `git diff --check` passed. This change affects discovery
configuration and documentation only; it does not alter engine behavior, adopt
additional VAW capabilities, or fix the separate optional `validation` defect.

Claude committed it as the owner's next task, by the procedure it describes: with
VAW at `c9d81ba1fe538ddaaaa44f2ca48e1dc9d3dcfcb1` and the explicit registry, the
`documentation` route, the only one the change touches, resolved `bound_scoped`
for `pixieengine`/1, and the Claude catalog listed `project-binding`.
