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

Create one file per record under `docs/archive/`, named
`<subject>-YYYY-MM-DD.md`, and add it to the archive index.

```text
# <Record title>

**Document ID:** ARCHIVE-<SUBJECT>-YYYYMMDD
**Schema version:** 1
**Kind:** audit or roadmap/status
**Lifecycle:** historical
**Subject:**
**Surface:** archive-record
**Boundary:** global or a boundary key
**Authority role:** evidence or non-normative
**Mutation:** snapshot
**Indexing:** include
**Reference date:** YYYY-MM-DD
**Reference commit:** full hash, none (before the initial commit), or not recorded
**Original path:** only when moved or imported; name the source repository
**Current state:** link to the current owner of the subject
```

Write each metadata field as its own paragraph, as in other documents. After
creation, change only narrowly identified metadata or links, or append a
dated reconciliation section; never rewrite the body.
