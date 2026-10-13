# A Named Conversion from Point to Vector

**Document ID:** GLOBAL-PROPOSAL-NAMED-POINT-CONVERSION

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** a method such as ToVector() next to the explicit cast from a point to a vector

**Surface:** proposal

**Boundary:** math

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## PXL-PROPOSAL-003

**State:** idea

**Idea:** Offer a named method, such as p.ToVector() (and the way back), doing
what the explicit conversion (PxVec2)p does today.

**Rationale:** A named method reads better than a cast where code only wants two
numbers. It is cosmetic: the conversion exists and stays explicit.

**Constraints:** The conversion must stay explicit: an implicit one lets point +
point compile again in C# (tested on 2026-10-12). In C++ the conversion is
pxToVec2 and pxToPoint, because an alias of a GLM type cannot have a
constructor; the names should match across the two sides.

**Promotion target:** owner decision required

**Source:** question 8.1, item 2, in section 4.18 of
[PXL-ARCHIVE-001](../archive/decision-notes-2026-10-12.md)