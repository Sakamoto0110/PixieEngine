# Documentation Policy

**Document ID:** GLOBAL-DOCUMENTATION-POLICY

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** authority, lifecycle, ownership, scheduling, archiving, and commit rules for PixieEngine documentation

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
version 1, observed 2026-10-09), and its authority traversal, self-review, and
stop rules from the documentation references of Void Agent Workplace (observed
2026-10-12 at `c9d81ba`), which extracted the NekoLib model. PixieLib and
PixieInspector derive their documentation from this one. The derivation and
every intentional difference are recorded in the
[schema index](../schemas/README.md). None of those projects is an authority
over this repository. Since 2026-10-16 the repository is bound, on the owner's
machine, to a Void Agent Workplace profile for project binding and skill
discovery only (see the [development guide](../development.md#void-agent-workplace));
the binding grants no authority, and the documentation is still checked by this
repository's own verifier, not by Void Agent Workplace.

## Authority

Every fact has exactly one owner. Other documents may summarize a fact and link
to its owner; they must not maintain a competing mutable copy.

| Fact | Owner |
|---|---|
| Implemented behavior | Current source code |
| Build configuration, targets, toolchain, and dependency wiring | CMake files: the root `CMakeLists.txt`, `CMakePresets.json`, `cmake/`, and each directory's `CMakeLists.txt` |
| Dependencies (toolchain, third-party, own projects), versions, status, and candidates | Root `DEPENDENCIES.md` |
| How to build, check, and commit a change | `docs/development.md`, under the commit rules below |
| What a sample shows and how to run it | `samples/<sample>/README.md` |
| Third-party source code | `external/<name>/`, unmodified from the release recorded in `DEPENDENCIES.md` |
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
| Executed evidence | Module `VALIDATIONS.md`, tests, scenarios, and the verification recorded in archive records |
| Chronology, completed work, superseded state, and dated rationale | `docs/archive/` |
| Documentation structure and vocabulary | `docs/schemas/documentation-schema.json` |
| Open questions and answers before promotion | `.local/` (unversioned; never authority) |

Generated output is never authority and is never versioned: `build/`, `bin/`, and the root
`PixieEngine.slnx`, which the configure step writes. Change their source (the CMake files)
instead.

## Authority traversal

When sources disagree or a fact must be established, each source is used only
for the facts it owns, in this order:

1. Source code and the CMake files establish behavior, targets, dependencies,
   and build properties.
2. Public headers establish the public native API.
3. Module `REFERENCE.md` files and this policy establish the documented
   contracts.
4. `ROADMAP.md` establishes direction, guardrails, and freezes.
5. `TODO.md` and `TASKLIST.md` establish promoted work, gates, and completion.
6. Tests, samples, scenarios, and recorded verification demonstrate bounded
   claims.
7. Archive records preserve chronology and rationale at their baselines.
8. Proposals, findings, `.local/`, and the documents of other repositories are
   non-normative leads.

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
- Task IDs are global and stable (`PIXIE-TASK-001`); they are not renumbered
  when a parent changes, completes, or is archived.

`ROADMAP.md` states direction but does not authorize implementation. A
proposal, finding, issue, archive record, or direct owner decision becomes work
only through formal promotion to `TODO.md`.

## Dependencies

`DEPENDENCIES.md` lists every dependency: toolchain, third-party libraries, and
the owner's own projects. Each one has a status observed on the file's
`Last checked` date:

- `ok`: present where it is consumed, at a current or deliberately pinned
  version;
- `outdated`: present, but a newer release exists and the gap matters;
- `not-installed`: not present yet.

A suggested dependency stays under candidates until the owner adopts it. A
status is an observation from its date, not a guarantee; recheck before a
version change. Vendored code is recorded with its release, source, and the
SHA-256 of the archive or file it came from. The CMake files own the wiring:
which target uses which dependency, built how. `DEPENDENCIES.md` records why and
the status. A dependency of another project of the owner, such as the GUI
binding of PixieInspector, belongs to that project's `DEPENDENCIES.md`; this one
lists only what the engine and its samples consume.

## Unversioned working documentation: `.local/`

`.local/` holds working documents that are deliberately not versioned and meant
for quick use, such as `.local/decisions/open-questions.md`. Everything in it is
safe to delete at any time. Questions are numbered so the owner can
answer by number. After an answer, an item is either promoted, expanded into
`TODO.md`, recorded in `ROADMAP.md`, turned into a proposal, or discarded.

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
completed, and why things changed. Each distinct archived event has one record:
a decision, a promotion, a completed `TODO.md` entry with its `TASKLIST.md`
section, a fix, a finding, a proposal outcome, a dependency change, or a roadmap
change. An event need not be split into permission, promotion and completion
records that repeat the same facts. Material imported from other repositories
for provenance is archived the same way.

Owner clarification, 2026-10-22: a plain authorization such as "you may
implement" does not need a separate archive entry that repeats the scheduler and
the eventual implementation report. Keep accepted scope in TODO/TASKLIST and
preserve it in the completion snapshot. A distinct promotion record is useful
when it preserves an actual scope/lifecycle transition or rationale that would
otherwise be lost. Do not multiply decision, authorization and promotion records
for the same fact. Existing historical records retain their IDs and bodies.

Every record identifies what produced it, when, and in which commits:

- `Archive ID`: a stable `PIXIE-ARCHIVE-NNN`, assigned in order and never
  reused.
- `Type`: `decision`, `promotion`, `completion`, `fix`, `finding`, `proposal`,
  `dependency`, `roadmap`, or `import`.
- `Origin`: the identity of what produced the event, such as
  `PIXIE-TODO-002`, `PIXIE-PROPOSAL-001`, `RENDER-FINDING-003`, or
  `owner decision`.
- `Author`: who wrote the record: `owner`, `claude`, or `codex`. It names the
  writer of the record, not of the commits, which carry the owner's identity.
- `Reference date`: when the event happened.
- `Commits`: the full hashes of the commits that carried the event. A record
  added in the same documentation-only commit as its event cannot name that
  commit; it says `with this record (parent <full hash>)`, and
  `git log -S <Archive ID>` finds it.
- `Reference commit`, optional: the baseline a record describes, when it
  differs from `Commits`.
- `Original path`, for a moved or imported record; imported records name the
  source repository in it.

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
- An imported record keeps the language of its body, under an English header.

## Module surfaces

A module is one boundary under `docs/modules/<boundary>/`. A module is created
when its boundary is decided, not when code first appears. Code that exists
before its boundary is decided, such as the provisional `engine/` library, is
documented by its public headers, its directory's `README.md` where one exists,
and the archive record of the work that produced it.

Required from creation:

- `MANIFEST.md` routes identity, build projects, dependencies, public API
  surface, documentation surfaces, evidence locations, and related boundaries.
  It does not reproduce public symbols.
- `README.md` is a concise introduction.
- `REFERENCE.md` is the single normative current technical contract.

Created on demand, together with their first record: `ISSUES.md`,
`FINDINGS.md`, `VALIDATION_REQUIREMENTS.md`, `VALIDATIONS.md`, `CHANGELOG.md`,
`INTERNALS.md`, `audits/`, and `migrations/`. The absence of an on-demand
surface means no record has been written; it is not evidence that no defect,
finding, or requirement exists.

`Related boundaries` aids traversal only. It does not create or imply a build
or package dependency.

## Samples, scenarios, and oracles

A sample demonstrates what the engine already does. It is documented by one
`README.md` in its own directory: what it shows, how to build and run it, and
its options. It is not a module.

A project with a purpose beyond demonstration needs more: a scenario proves a
specific claim, and an oracle breaks something on purpose to show that the
failure is caught. Its `README.md` also states the claim or the failure, the
expected outcome, and how to read the result. What such a project demonstrates
is evidence; it never defines behavior.

## Tests and evidence

Tests, samples, and measured runs are evidence: they demonstrate claims and
never define behavior. A measurement holds only for the machine, configuration,
and commit it ran with, and is recorded with them, as PIXIE-TODO-002 recorded
its frame and update rates. A console or program written outside the
repository, for a comparison or a deliberately broken copy, is evidence only
when its result is recorded with its inputs; otherwise it is a working aid.

Owner evidence clarification, 2026-10-22: an explicit written record that a
result was visually checked using screenshots or reviewed by a human remains
valid visual-review evidence even when the image file is no longer available.
State who/what was checked when recorded, and distinguish this testimony from
an inspectable artifact or a fresh independent reproduction. Do not retroactively
claim that testimony is an available screenshot. If neither an artifact nor an
explicit verification record supports the claimed evidence, reject the unsupported
proof claim; retain the observation without inventing verification. This does
not convert a visual review into timing measurements or automated test evidence.

## Commits

- Messages are in English and start with a prefix in parentheses: `(feat)`,
  `(fix)`, `(refactor)`, or `(docs)`. The subject is short and imperative; the
  body says what changed and why.
- Code and documentation go in separate commits. The documentation commit that
  records code work cites the hashes of the code commits.
- Vendored third-party code goes in its own commit, separate from the code that
  uses it.
- Commits carry the owner's identity only, as author and committer:
  `Rafael Sakamoto <rafael.sakamoto1@hotmail.com>`, with no co-author or tool
  attribution lines.
- Work goes to `main` on `Sakamoto0110/PixieEngine`. A session that can push
  only to a branch of its own pushes there, fast-forwards `main` to the same
  commit, and deletes its branch; no other branch is created without an owner
  decision. Nothing is pushed without an owner decision.
- History may be rewritten only before it is pushed; afterwards, a correction
  is a new commit.
- Continuous integration runs on every push to `main`
  ([`docs/development.md`](../development.md#continuous-integration)). Its run
  is checked after every push, and a red `main` comes before anything else.

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
understandable without a private prompt, a conversation, an external workspace,
or knowledge of which person or tool wrote them. External procedures, including
agent tooling, may add emphasis but do not own a format, path, authority rule,
or acceptance criterion here.

## Stop rules

Stop, preserve the evidence, and ask the owner for a disposition when:

- source and a current normative reference disagree; do not silently rewrite
  either side;
- the requested documentation outcome needs a change to code, the public API,
  or the build;
- work would cross an active freeze, a guardrail, or the requested boundary;
- a finding lacks the evidence to become an issue;
- work would promote a proposal, finding, or decision that the owner has not
  accepted;
- the owner of a fact, a module, or a dependency cannot be established;
- uncommitted work that was already in the tree overlaps the intended change;
  or
- an archive record would have to be rewritten instead of reconciled.

A stop authorizes no broader investigation or change. Report the smallest exact
conflict and the sources that expose it. Evidence demonstrates claims but does
not define behavior, a finding becomes an issue only after verification, and
historical bodies are not rewritten to match later truth.

## Self-review

Before declaring documentation work complete, check that:

- every required document exists and carries the metadata the schema requires,
  with stable IDs;
- every relative link and anchor resolves with the repository's casing;
- each fact has one owner, and summaries link to it instead of copying it;
- current claims match the source and the CMake files;
- proposals and findings are not presented as decisions or scheduled work;
- evidence is not presented as contract, and records only what actually ran;
- archive records keep their baseline and body, and the router lists every
  record with the same ID;
- gaps and what was not run are stated; and
- another session or agent can continue from the repository alone.

Report what was checked and what was not, separately: source, build, runs,
documentation, and the commit or push.

## Enforcement

`scripts/check_docs.py` mechanically checks part of these rules: relative
links and their anchors, no link into `.local/`, the metadata fields that the
schema requires and their vocabulary, unique Document IDs, the scheduler rules
(parents, back-links, unique task IDs, reasons on dropped tasks), and an archive
router that lists every record once, in order, under its own ID. The rest
(single ownership of facts, claims that match the source, honest evidence) is
applied by review only. Claims about structural validity say which of the two
they rest on.
