# PIXIE-TODO-004: Documentation Verifier

**Document ID:** ARCHIVE-TODO-004-20261013

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completed PIXIE-TODO-004, its task breakdown, and its verification

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-016

**Type:** completion

**Origin:** PIXIE-TODO-004 (PIXIE-TASK-016 to PIXIE-TASK-019)

**Author:** claude

**Reference date:** 2026-10-13

**Commits:** `512ca454c83a5538738648f089ffaa10448b7bc0`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** `scripts/check_docs.py`, [`docs/development.md`](../development.md#checks-before-a-commit), and the [policy](../governance/documentation-policy.md#enforcement)

## Entry as completed

**Status:** completed on 2026-10-13

**Source:** owner decision on 2026-10-13: a small script checking metadata, links, and scheduler rules

**Accepted scope:** a Python script under `scripts/` that checks the versioned Markdown for: the required metadata fields and the schema's vocabulary; relative links and anchors; the scheduler rules (each `TASKLIST.md` section has a `TODO.md` parent that links back, task IDs are unique, `dropped` tasks state a reason); and an archive router that lists every record under the same ID. The development guide and the policy's enforcement section describe it.

**Dependencies and gates:** none

**Completion criteria:** (1) the script passes on the repository; (2) a broken link, metadata value, scheduler back-link, and router row, each in a copy, make it fail with a message naming the file; (3) `docs/development.md` lists it among the checks before a commit.

**Commits:** `512ca454c83a5538738648f089ffaa10448b7bc0` (the script); the development guide and the policy are updated with this record.

## Breakdown as completed

- `PIXIE-TASK-016` — **Status:** done — `scripts/check_docs.py`: collect the versioned Markdown (outside `external/`, `build/`, `bin/`, and `.local/`) and check relative links and their anchors.
- `PIXIE-TASK-017` — **Status:** done — Check metadata: the required fields of indexable documents and their values against `docs/schemas/documentation-schema.json`, and the extra fields of archive records.
- `PIXIE-TASK-018` — **Status:** done — Check the schedulers and the archive router: section parents and back-links, unique task IDs, reasons on dropped tasks, and one router row per record with the same ID.
- `PIXIE-TASK-019` — **Status:** done — Make one deliberate break of each kind in a copy and watch the script fail; describe the script in `docs/development.md` and in the policy's enforcement section.

## Verification

Machine: Windows 11, Python 3.14.7.

- `python scripts/check_docs.py` on the repository, after this record was
  added: 39 documents checked, 0 problems.
- Each kind of break, made in a copy of the repository's tracked and
  untracked files outside `external/`, fails the script with exit code 1 and
  a message naming the file and line:
  - a link to a missing file (`README.md` pointing at `TODOX.md`);
  - a metadata value outside the vocabulary (`Lifecycle: alive` in
    `ROADMAP.md`);
  - a `Breakdown` that links to another entry's section (PIXIE-TODO-005);
  - a router row renamed to an unused ID, reported as out of order and as a
    record without a row;
  - a duplicated task ID, and a task set to `dropped` without a reason.
- While it was written, the first run flagged the placeholder metadata inside
  the code block of the archive-record template; fenced code is now skipped
  when reading metadata.
- Not checked by the script, and left to review: single ownership of facts,
  claims matching the source, and the content of evidence.
