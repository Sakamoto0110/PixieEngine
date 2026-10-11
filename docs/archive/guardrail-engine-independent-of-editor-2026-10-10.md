# Guardrail: the Engine Never Depends on the Editor

**Document ID:** ARCHIVE-ENGINE-EDITOR-20261010

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** promotion of the dependency direction from PIXIE-PROPOSAL-001 to a roadmap guardrail

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-004

**Type:** roadmap

**Origin:** PIXIE-PROPOSAL-001 (partial promotion, owner decision)

**Author:** claude

**Reference date:** 2026-10-10

**Commits:** `ccf1db9e7537f20f3d9eb127c492984ee1eefa5e`

**Current state:** [`ROADMAP.md`](../../ROADMAP.md#active-guardrails), [`docs/proposals/engine-module-boundaries.md`](../proposals/engine-module-boundaries.md)

The owner asked whether the engine depends on the editor. In the proposed
module split it does not, and that direction was promoted on its own: the
editor depends on the engine, never the reverse. What the editor needs on the
native side enters the engine as a general feature; the player building and
running with nothing from the editor is the proof. The module split itself
stays a proposal.

Accepted on 2026-10-10 and written into `ROADMAP.md` in the commit above.
