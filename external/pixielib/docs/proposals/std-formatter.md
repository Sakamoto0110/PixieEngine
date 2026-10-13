# std::formatter for the Primitives

**Document ID:** GLOBAL-PROPOSAL-STD-FORMATTER

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** a std::formatter for the C++ primitives, next to ToString

**Surface:** proposal

**Boundary:** primitives

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## PXL-PROPOSAL-004

**State:** idea

**Idea:** Add a std::formatter specialization for the C++ primitives (and
pxToString for the vectors and matrices), producing the same text as ToString.

**Rationale:** std::format and std::print could take a primitive directly. The
engine consumes the primitives in C++.

**Constraints:** Enters if the engine asks for it. The text must stay the one of
the layout contract: the shortest round-trip digits, no scientific notation, the
same in C# and C++. No operator<< was planned either.

**Promotion target:** owner decision required

**Source:** question 2.9, in section 4.10 of
[PXL-ARCHIVE-001](../archive/decision-notes-2026-10-12.md)