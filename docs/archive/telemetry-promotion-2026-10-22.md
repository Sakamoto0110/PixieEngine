# Generic Telemetry Promoted: PIXIE-TODO-040

**Document ID:** ARCHIVE-TELEMETRY-PROMOTION-20261022

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** owner continuation of generic telemetry extraction and active LogicSandbox integration

**Surface:** archive-record

**Boundary:** global

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-076

**Type:** promotion

**Origin:** owner decision

**Author:** codex

**Reference date:** 2026-10-22

**Commits:** uncommitted working tree based on `2365251e7ea50fe730e937c03bdd404f46a86f40`

**Current state:** [TODO.md](../../TODO.md)

The owner authorized continuation after the engine handoff. This first bounded extraction is
generic telemetry: collector-owned CPU timings, gauges/counters, optional scoped collection,
snapshots/export, named-system instrumentation and the existing native-player debug surface.
LogicSandbox consumes the collector; circuit semantics and circuit-specific sampling stay in the game.
The owner clarified that the actively developed sandbox will replace the older game version.

This promotes PIXIE-TODO-040 without a further approval gate for this scope. Lighting, audio,
collision/controller work, path caches, visibility infrastructure, queues and prefabs stay outside
this entry. No commit, push, publication or deletion of the older checkout was requested.

The explicit VAW binding resolved `bound_scoped` for engine, runtime, samples, build and documentation
with profile `pixieengine` revision 1; Codex discovery listed project-binding. VAW is at
`c9d81ba1fe538ddaaaa44f2ca48e1dc9d3dcfcb1`. Binding/discovery grant no additional authority.
