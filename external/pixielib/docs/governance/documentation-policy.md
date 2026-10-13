# Documentation Policy

**Document ID:** GLOBAL-DOCUMENTATION-POLICY

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** authority, lifecycle, ownership, scheduling, archiving, and commit rules for PixieLib documentation

**Surface:** policy

**Boundary:** global

**Authority role:** normative

**Mutation:** authored

**Indexing:** include

This policy defines where each kind of PixieLib fact is owned and how documents
move between states. The structural vocabulary lives in the
[documentation schema](../schemas/documentation-schema.json); this policy owns
the meaning of that vocabulary.

The model is derived from the PixieEngine documentation policy and schema
(schema version 1, observed 2026-10-12 at PixieEngine `46ee6aa`), with the .NET
additions PixieInspector made to them (observed the same day at PixieInspector
`25d56df`); both are partial derivations of the NekoLib model that Void Agent
Workplace extracted. The derivation and every intentional difference are
recorded in the [schema index](../schemas/README.md). None of those projects is
an authority over this repository.

## Authority

Every fact has exactly one owner. Other documents may summarize a fact and link
to its owner; they must not maintain a competing mutable copy.

| Fact | Owner |
|---|---|
| Implemented behavior | Current source code |
| C++ targets, options, compile definitions, and the pinned GLM | `cpp/CMakeLists.txt` and `cpp/tests/CMakeLists.txt` |
| C# targets, references, package references, the package metadata, and its version | Each project file (`*.csproj`) under `dotnet/` |
| C# solution membership | `dotnet/PixieLib.slnx` |
| Continuous integration and publication | `.github/workflows/ci.yml` and `.github/workflows/publish.yml` |
| Dependencies (toolchain, libraries, packages), versions, status, and candidates | Root `DEPENDENCIES.md` |
| Public API | The public headers of the C++ side and the compiled assembly of the C# side; no accepted API baseline exists yet |
| The layout of every type both languages share, and the semantics both sides repeat | `docs/layout.md` |
| Module technical contract | `docs/modules/<boundary>/REFERENCE.md` |
| Module routing and identity | `docs/modules/<boundary>/MANIFEST.md` |
| What PixieLib is and is not, direction, guardrails, and freezes | Root `ROADMAP.md` |
| Formally promoted work, ordering, gates, and completion criteria | Root `TODO.md` |
| Decomposition of a promoted `TODO.md` entry into tasks | Root `TASKLIST.md` |
| Unpromoted ideas | One record per file under `docs/proposals/` |
| How to build, check, and release a change | `docs/development.md`, under the commit rules below |
| Confirmed defects | Module `ISSUES.md` |
| Unconfirmed observations | Module `FINDINGS.md` |
| Evidence requirements | Module `VALIDATION_REQUIREMENTS.md` |
| Executed evidence | Module `VALIDATIONS.md`, the tests, and CI runs |
| Chronology, completed work, superseded state, and dated rationale | `docs/archive/` |
| Documentation structure and vocabulary | `docs/schemas/documentation-schema.json` |
| Open questions and answers before promotion | `.local/` (unversioned; never authority) |

Generated output is never authority and is never versioned: `bin/`, `obj/`, and
the CMake build directories.

## The layout contract

PixieLib is one library in two languages. [`docs/layout.md`](../layout.md) owns
what makes the two sides one: the fields, order, and size of every shared type,
and the rules both sides follow so that the same data means the same thing and,
where the contract says so, gives the same bits. It is global because every
module depends on it. A module's `REFERENCE.md` links to the rows and rules
that concern it and does not copy them. Source comments cite it as
`docs/layout.md`.

## Scheduling: `TODO.md` and `TASKLIST.md`

`TODO.md` is the only scheduler. An entry enters it only through an explicit
owner decision with its scope, gates, and completion criteria formalized. A
change to the scope of a promoted entry also needs an owner decision and is
recorded in the entry with its date.

When an entry completes, it moves to `docs/archive/` as a snapshot, together
with its `TASKLIST.md` section if it has one, its verification, and the commits
that delivered it; `TODO.md` keeps a one-line pointer to the archive record.

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
- When the parent completes, its section moves to `docs/archive/` with the
  parent's record and leaves `TASKLIST.md`.
- Task IDs are global and stable (`PXL-TASK-001`); they are not renumbered
  when a parent changes, completes, or is archived.

`ROADMAP.md` states direction but does not authorize implementation. A
proposal, finding, issue, archive record, or direct owner decision becomes work
only through formal promotion to `TODO.md`.

## Dependencies

`DEPENDENCIES.md` lists every dependency: toolchain, libraries, and packages.
Each one has a status observed on the file's `Last checked` date:

- `ok`: present where it is consumed, at a current or deliberately pinned
  version;
- `outdated`: present, but a newer release exists and the gap matters;
- `not-installed`: not present yet.

A suggested dependency stays under candidates until the owner adopts it. A
status is an observation from its date, not a guarantee; recheck before a
version change. The build and project files own the wiring: which target or
project uses which dependency, at which version. `DEPENDENCIES.md` records why
and the status.

## Unversioned working documentation: `.local/`

`.local/` holds working documents that are deliberately not versioned and meant
for quick use, such as `.local/decisions/open-questions.md`. Everything in it is
safe to delete at any time. Questions are numbered so the owner can answer by
number. After an answer, an item is either promoted, expanded into `TODO.md`,
recorded in `ROADMAP.md`, turned into a proposal, or discarded.

`.local/` is never authority, and versioned documents never link into it,
because a clone does not contain it. An answer that shapes code, structure, or
direction must be promoted to a versioned owner; an unpromoted answer is not a
repository fact.

A session in a disposable environment, such as a cloud container, loses
`.local/` when the environment ends. There the questions are asked in the
conversation, numbered the same way, and each answer is promoted as soon as it
is given.

## Archive

`docs/archive/` owns the history of the repository: what changed, what was
completed, and why things changed. Every archived event is one file, however
small: a decision, a promotion, a completed `TODO.md` entry with its
`TASKLIST.md` section, a fix, a finding, a proposal outcome, a dependency
change, or a roadmap change. Material imported for provenance, from another
repository or from this repository's documentation before this policy, is
archived the same way.

Every record identifies what produced it, when, and in which commits:

- `Archive ID`: a stable `PXL-ARCHIVE-NNN`, assigned in order and never reused.
- `Type`: `decision`, `promotion`, `completion`, `fix`, `finding`, `proposal`,
  `dependency`, `roadmap`, or `import`.
- `Origin`: the identity of what produced the event, such as `PXL-TODO-001`,
  `PXL-PROPOSAL-002`, `MATH-FINDING-001`, or `owner decision`.
- `Author`: who wrote the record: `owner`, `claude`, or `codex`. It names the
  writer of the record, not of the commits, which carry the owner's identity.
- `Reference date`: when the event happened.
- `Commits`: the full hashes of the commits that carried the event. A record
  added in the same documentation-only commit as its event cannot name that
  commit; it says `with this record (parent <full hash>)`, and
  `git log -S <Archive ID>` finds it.
- `Reference commit`, optional: the baseline a record describes, when it
  differs from `Commits`.
- `Original path`, for a moved or imported record; imported records from
  another repository name it in this field.

A record about one module carries that module's key in `Boundary`; there is no
second, per-module history.

[`docs/archive/README.md`](../archive/README.md) is a router: one table row per
record, in `Archive ID` order, with the ID, date, type, origin, author, a
one-line summary, short commit hashes, and the link to the record. A row says
enough to decide whether a record is the one needed; the record holds the full
context. Find the event in the router, then read only its record.

- Records are `Lifecycle: historical` and `Mutation: snapshot`. The body and
  baseline are preserved; only narrowly identified metadata or link
  corrections and appended reconciliation sections are allowed.
- A record is true only for its baseline. It never owns current technical
  facts, open work, or direction.
- An imported record keeps the language of its body. The documents written
  before this policy (PXL-ARCHIVE-001 to PXL-ARCHIVE-006) are in Portuguese,
  under an English header. The source cites the decision notes,
  PXL-ARCHIVE-001, by path (`docs/notas.md, 4.13`), by question number
  (`(5.2)`), and by the question number of the InteractiveEditor rework
  (`P8.5`).

## Portals

A portal is a pointer-only document kept at a path that something outside the
documentation cites. It has `Surface: portal`, `Authority role: portal`,
`Indexing: pointer-only`, and a `Canonical` path, and says nothing else of
substance. [`docs/notas.md`](../notas.md) is the one portal: source comments
cite the path of the pre-policy notes, now PXL-ARCHIVE-001.

## Module surfaces

A module is one boundary under `docs/modules/<boundary>/`: `primitives`,
`math`, `events`, or `generator`. A module is a concept, and it covers both
languages where both have it; its `REFERENCE.md` describes both sides. A module
is created when its boundary is decided.

Required from creation:

- `MANIFEST.md` routes identity, build and project files, languages, target
  frameworks, dependencies, public API surface, documentation surfaces,
  evidence locations, and related boundaries. It does not reproduce public
  symbols.
- `README.md` is a concise introduction.
- `REFERENCE.md` is the single normative current technical contract.

Created on demand, together with their first record: `ISSUES.md`,
`FINDINGS.md`, `VALIDATION_REQUIREMENTS.md`, `VALIDATIONS.md`, `CHANGELOG.md`,
`INTERNALS.md`, `audits/`, and `migrations/`. The absence of an on-demand
surface means no record has been written; it is not evidence that no defect,
finding, or requirement exists.

`Related boundaries` aids traversal only. It does not create or imply a build
or package dependency.

## Tests and continuous integration

The tests under `cpp/tests/` and `dotnet/PixieLib.Tests/`, and the CI runs on
GitHub, are evidence: they demonstrate claims and never define behavior. A
check that compares bits across the two languages proves parity only for the
inputs and the compilers it ran with. A test console written outside the
repository, for a comparison or a deliberately broken copy, is evidence only
when its result is recorded with its inputs; otherwise it is a working aid.

## Commits

- Messages are in English and start with a prefix in parentheses: `(feat)`,
  `(fix)`, `(refactor)`, or `(docs)`. The subject is short; the body says what
  changed and why.
- Code and documentation go in separate commits. The documentation commit that
  records code work cites the hashes of the code commits.
- Commits carry the owner's identity only, as author and committer:
  `Rafael Sakamoto <rafael.sakamoto1@hotmail.com>`, with no co-author or tool
  attribution lines.
- Work goes to `main`. A session that can push only to a branch of its own
  pushes there, fast-forwards `main` to the same commit, and deletes its
  branch; no other branch is created without an owner decision.
- After a push, CI on GitHub is checked: a red `main` comes before anything
  else, and what only MSVC or the .NET Framework shows appears only there.
- History may be rewritten only before it is pushed; afterwards, a correction
  is a new commit.

## Releases

A new version starts by raising the `Version` of
`dotnet/PixieLib/PixieLib.csproj`, in the commit that makes the change that
needs it. Below 1.0, a change that breaks the API raises the minor version. The
owner publishes through `.github/workflows/publish.yml`, which runs CI, packs,
publishes to nuget.org, and creates the tag; sessions never create tags. The
C++ side is released by the same tag.

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

Versioned and `.local/` documentation is written in English, except the body of
an imported record. Code and code comments are in English. Documents must be
understandable without a private prompt, a conversation, an external
workspace, or knowledge of which person or tool wrote them. External
procedures, including agent tooling, may add emphasis but do not own a format,
path, authority rule, or acceptance criterion here.

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
