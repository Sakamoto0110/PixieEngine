# Archive Router

**Document ID:** GLOBAL-ARCHIVE-INDEX

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** router to the archive records, one row per record

**Surface:** index

**Boundary:** global

**Authority role:** non-normative

**Mutation:** append-only

**Indexing:** include

One row per archive record, in `Archive ID` order. A row says enough to decide
whether a record is the one you need; the record holds the full context. The
rules are in the [documentation policy](../governance/documentation-policy.md#archive).

Records PXL-ARCHIVE-001 to 006 are the documents written before the policy,
kept whole and in Portuguese. The source cites the decision notes,
PXL-ARCHIVE-001, by path (`docs/notas.md, 4.17`, through a
[portal](../notas.md)), by question number (`(5.2)`: look for the section of
the notes that cites it), and by the question number of the InteractiveEditor
rework (`P8.5`).

| ID | Date | Type | Origin | Author | Summary | Commits | Record |
|---|---|---|---|---|---|---|---|
| PXL-ARCHIVE-001 | 2026-10-12 | import | owner decision | claude | Decision notes (pt-BR): what PixieLib is and is not, every answered question from 1.1 to 9.1 with its commits, the commit rules | `9601cea` | [`decision-notes-2026-10-12.md`](decision-notes-2026-10-12.md) |
| PXL-ARCHIVE-002 | 2026-10-12 | import | owner decision | claude | Open questions (pt-BR), all answered: where each section's answers went, and the answer-by-number convention | `9601cea` | [`open-questions-2026-10-12.md`](open-questions-2026-10-12.md) |
| PXL-ARCHIVE-003 | 2026-10-02 | import | owner decision | claude | PixieLib handoff (pt-BR) from the InteractiveEditor rework: the decisions brought from there, the questions it raised, how to verify | `9601cea` | [`pixielib-handoff-2026-10-02.md`](pixielib-handoff-2026-10-02.md) |
| PXL-ARCHIVE-004 | 2026-10-02 | import | owner decision | claude | History (pt-BR): why PixieLib exists and why the 2023 code is the way it is | `9601cea` | [`history-2026-10-02.md`](history-2026-10-02.md) |
| PXL-ARCHIVE-005 | 2026-10-02 | import | owner decision | claude | Diagnosis of the 2023 code (pt-BR): what it did, checked with the AddressSanitizer; nothing fixed | `9601cea` | [`diagnosis-2023-code-2026-10-02.md`](diagnosis-2023-code-2026-10-02.md) |
| PXL-ARCHIVE-006 | 2026-10-12 | import | owner decision | claude | Layout contract (pt-BR), as it stood before its English rewrite at the same path | `9601cea` | [`layout-contract-2026-10-12.md`](layout-contract-2026-10-12.md) |
| PXL-ARCHIVE-007 | 2026-10-12 | completion | PXL-TODO-001 | claude | The documentation policy adopted: derived from PixieEngine, PixieInspector, and VAW; four modules; the pre-policy documents archived whole | `9601cea`, `94a1a83`, `7830896`, `c61a061`, `6a7a482`, `e9f6d0f`, `a858cdd`, and the record's own | [`documentation-foundation-2026-10-12.md`](documentation-foundation-2026-10-12.md) |
