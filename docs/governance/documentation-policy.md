# Documentation Policy

**Document ID:** GLOBAL-DOCUMENTATION-POLICY

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** authority, lifecycle, ownership, scheduling, and archiving rules for PixieEngine documentation

**Surface:** policy

**Boundary:** global

**Authority role:** normative

**Mutation:** authored

**Indexing:** include

This policy defines where each kind of PixieEngine fact is owned and how
documents move between states. The structural vocabulary lives in the
[documentation schema](../schemas/documentation-schema.json); this policy owns
the meaning of that vocabulary.

The model is derived from the NekoLib documentation policy and schema (schema
version 1, observed 2026-10-09). The derivation and every intentional
difference are recorded in the [schema index](../schemas/README.md). NekoLib is
the origin of the model, not an authority over this repository.

## Authority

Every fact has exactly one owner. Other documents may summarize a fact and link
to its owner; they must not maintain a competing mutable copy.

| Fact | Owner |
|---|---|
| Implemented behavior | Current source code |
| Build configuration, targets, toolchain, and dependency wiring | Build project files (build system pending owner decision) |
| Dependencies (toolchain, third-party, own projects), versions, status, and candidates | Root `DEPENDENCIES.md` |
| Public native API | The module's public header, once it exists |
| Module technical contract | `docs/modules/<boundary>/REFERENCE.md` |
| Module routing and identity | `docs/modules/<boundary>/MANIFEST.md` |
| What the engine is and is not, direction, guardrails, and freezes | Root `ROADMAP.md` |
| Formally promoted work, ordering, gates, and completion criteria | Root `TODO.md` |
| Decomposition of a promoted `TODO.md` entry into tasks | Root `TASKLIST.md` |
| Unpromoted ideas | One record per file under `docs/proposals/` |
| Confirmed defects | Module `ISSUES.md` |
| Unconfirmed observations | Module `FINDINGS.md` |
| Evidence requirements | Module `VALIDATION_REQUIREMENTS.md` |
| Executed evidence | Module `VALIDATIONS.md`, tests, and scenarios |
| Chronology, completed work, superseded state, and dated rationale | `docs/archive/` |
| Documentation structure and vocabulary | `docs/schemas/documentation-schema.json` |
| Open questions and answers before promotion | `.local/` (unversioned; never authority) |

## Scheduling: `TODO.md` and `TASKLIST.md`

`TODO.md` is the only scheduler. An entry enters it only through an explicit
owner decision with its scope, gates, and completion criteria formalized.

`TASKLIST.md` is a child scheduler of `TODO.md`. It exists so that an entry
whose scope is large, or that decomposes into many small tasks, does not
pollute `TODO.md`. Its rules:

- Every section belongs to exactly one `TODO.md` entry, named in its `Parent`
  field, and that entry links back to the section in its `Breakdown` field.
  A task without an existing parent is invalid.
- Tasks decompose the parent's accepted scope; they never extend it. Work
  outside that scope is promoted to `TODO.md` first.
- The parent completes only when each of its tasks is `done` or `dropped`, and
  every `dropped` task states why.
- When the parent completes, its section moves to `docs/archive/` as a snapshot
  and leaves `TASKLIST.md`; `TODO.md` keeps a one-line pointer to the archive
  record.
- Task IDs are global and stable (`PIXIE-TASK-001`); they are not renumbered
  when a parent changes, completes, or is archived.

`ROADMAP.md` states direction but does not authorize implementation. A
proposal, finding, issue, archive record, or direct owner decision becomes work
only through formal promotion to `TODO.md`.

## Unversioned working documentation: `.local/`

`.local/` holds working documents that are deliberately not versioned, such as
`.local/decisions/open-questions.md`. Questions are numbered so the owner can
answer by number. After an answer, an item is either promoted, expanded into
`TODO.md`, recorded in `ROADMAP.md`, turned into a proposal, or discarded.

`.local/` is never authority, and versioned documents never link into it,
because a clone does not contain it. An answer that shapes code, structure, or
direction must be promoted to a versioned owner; an unpromoted answer is not a
repository fact.

## Archive

`docs/archive/` owns the chronology of the repository. It holds completed
`TODO.md` entries and `TASKLIST.md` sections, superseded `ROADMAP.md` snapshots,
dated decision rationale, and material imported from other repositories for
provenance.

- Archive records are `Lifecycle: historical` and `Mutation: snapshot`. The body
  and baseline are preserved; only narrowly identified metadata or link
  corrections and appended reconciliation sections are allowed.
- A record states its `Reference date` and `Reference commit`. Use
  `none (before the initial commit)` or `not recorded` rather than inferring a
  commit.
- Moved or imported records add `Original path`; imported records name the
  source repository in it.
- An archive record is true only for its baseline. It never owns current
  technical facts, open work, or direction.

## Module surfaces

A module is one boundary under `docs/modules/<boundary>/`. Modules are created
when their code exists, not before.

Required from creation:

- `MANIFEST.md` routes identity, build projects, dependencies, public API
  surface, documentation surfaces, evidence locations, and related boundaries.
  It does not reproduce public symbols.
- `README.md` is a concise introduction.
- `REFERENCE.md` is the single normative current technical contract.
- `HISTORY.md` is a factual, chronological, append-only module timeline.

Created on demand, together with their first record: `ISSUES.md`,
`FINDINGS.md`, `VALIDATION_REQUIREMENTS.md`, `VALIDATIONS.md`, `CHANGELOG.md`,
`INTERNALS.md`, `audits/`, and `migrations/`. The absence of an on-demand
surface means no record has been written; it is not evidence that no defect,
finding, or requirement exists.

`Related boundaries` aids traversal only. It does not create or imply a build
or package dependency.

## Metadata

Indexable documents carry the bold-label metadata defined by the schema, one
value per physical line, near the top of the file. `Kind`, `Lifecycle`,
`Surface`, `Boundary`, `Authority role`, `Mutation`, and `Indexing` are
independent dimensions. Paths in metadata are repository-relative with forward
slashes.

- `current` is maintained with the repository.
- `frozen` stays live but cannot expand without its recorded unfreeze condition.
- `historical` is a dated snapshot and never current-state authority.
- An empty registry states its baseline explicitly.

Document IDs and record IDs are stable and never renumbered or reused.

## Language and authoring

Versioned and `.local/` documentation is written in English. Documents must be
understandable without a private prompt, a conversation, an external workspace,
or knowledge of which person or tool wrote them. External procedures, including
agent tooling, may add emphasis but do not own a format, path, authority rule,
or acceptance criterion here.

## Conflict handling

- If source and a current normative reference disagree, record the exact
  conflict and stop for disposition; do not silently rewrite either side.
- Evidence demonstrates claims but does not define behavior.
- A finding becomes an issue only after verification.
- Historical bodies are not rewritten to match later truth; later outcomes are
  recorded separately.

## Enforcement

No documentation verifier exists yet. Until one does, these rules are applied
by review only, and a document that satisfies them has not been mechanically
checked. Claims about structural validity must say so.
