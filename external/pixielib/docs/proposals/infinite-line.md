# The Infinite Line

**Document ID:** GLOBAL-PROPOSAL-INFINITE-LINE

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** PxLine, the infinite line, for clipping and half-planes

**Surface:** proposal

**Boundary:** math

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## PXL-PROPOSAL-002

**State:** idea

**Idea:** Add the infinite line (PxLine and pxLine_t), for clipping and
half-plane tests, next to the segment.

**Rationale:** The segment came first because the engine needs it now (debug
drawing, gizmos, edges, collision, lines cast with a maximum distance). Clipping
and half-planes need a line without ends.

**Constraints:** Enters only when something asks for it. Its representation (a
point and a direction, or a normal and a distance) is open. In C++ it belongs to
pixie::math, as the segment, because it uses a vector.

**Promotion target:** owner decision required

**Source:** question 8.2, in section 4.17 of
[PXL-ARCHIVE-001](../archive/decision-notes-2026-10-12.md)