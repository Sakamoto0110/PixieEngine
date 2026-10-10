# Task List Section Template

**Document ID:** TEMPLATE-TASKLIST-SECTION

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** template for one TASKLIST.md section under a TODO.md parent

**Surface:** template

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** exclude

Open a section in `TASKLIST.md` only for an existing `TODO.md` entry, and set
that entry's `Breakdown` field to the section. Tasks decompose the parent's
accepted scope; they never extend it.

```text
## `PIXIE-TODO-001` — Parent title

**Parent:** PIXIE-TODO-001

- `PIXIE-TASK-001` — **Status:** open — Description.
- `PIXIE-TASK-002` — **Status:** dropped — Description. **Reason:** why.
```

Task statuses are `open`, `in-progress`, `done`, and `dropped`. When the parent
completes, move the whole section to `docs/archive/`.
