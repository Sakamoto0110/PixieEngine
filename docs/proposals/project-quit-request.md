# Project quit request

**Document ID:** PIXIE-PROPOSAL-020

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** Let a project request graceful exit from its native player

**Surface:** proposal

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

The owner requested Play/Quit and Resume/Quit menus in LogicSandbox on 2026-11-02.
Add World-scoped RequestPlayerExit/PlayerExitRequested functions in PlayerExit.hpp.
ProjectApp closes its Window after an update observes the request, preserving normal
Stop/session cleanup. Embedded editor sessions do not close their host. The request
does not change Project's virtual interface or its object layout; a new World starts clear.
Validate the consumer menu's request state and the native player build.
