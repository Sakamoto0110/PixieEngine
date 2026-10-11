# Archive Record Template

**Document ID:** TEMPLATE-ARCHIVE-RECORD

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** template for a dated archive snapshot

**Surface:** template

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** exclude

Create one file per archived event under `docs/archive/`, named
`<subject>-YYYY-MM-DD.md`, and add one row for it to the router in
`docs/archive/README.md`, with a one-line summary. Write each metadata field as its own paragraph, as in
other documents.

```text
# <Record title>

**Document ID:** ARCHIVE-<SUBJECT>-YYYYMMDD
**Schema version:** 1
**Kind:** roadmap/status or audit
**Lifecycle:** historical
**Subject:**
**Surface:** archive-record
**Boundary:** global or a boundary key
**Authority role:** evidence or non-normative
**Mutation:** snapshot
**Indexing:** include
**Archive ID:** PIXIE-ARCHIVE-NNN
**Type:** decision, promotion, completion, fix, finding, proposal, dependency, roadmap, or import
**Origin:** what produced the event: an ID such as PIXIE-TODO-002, or owner decision
**Author:** owner, claude, or codex: who wrote this record
**Reference date:** YYYY-MM-DD
**Commits:** full hashes, or with this record (parent <full hash>)
**Reference commit:** optional; the baseline described, when it differs from Commits
**Original path:** only when moved or imported; name the source repository
**Current state:** optional; link to the current owner of the subject
```

The body says what happened and why, and links deeper evidence instead of
copying it. After creation, change only narrowly identified metadata or links,
or append a dated reconciliation section; never rewrite the body.
