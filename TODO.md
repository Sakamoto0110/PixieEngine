# PixieEngine Promoted Work

**Document ID:** GLOBAL-TODO

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** formally promoted work, gates, dependencies, and completion criteria

**Surface:** work-scheduler

**Boundary:** global

**Authority role:** scheduler

**Mutation:** authored

**Indexing:** include

**Last promotion decision:** 2026-10-09

## Purpose and admission rule

This file is the only scheduler. It holds work that the owner has formally
promoted, with enough scope to execute and close it. It does not hold ideas
([`docs/proposals/`](docs/proposals/README.md)), direction
([`ROADMAP.md`](ROADMAP.md)), or completion narratives
([`docs/archive/`](docs/archive/README.md)).

An entry whose scope is large, or that decomposes into many small tasks, keeps
one entry here and its breakdown in [`TASKLIST.md`](TASKLIST.md). The rules are
in the [documentation policy](docs/governance/documentation-policy.md#scheduling-todomd-and-tasklistmd).

Unchecked or `promoted` entries are not automatically authorized to start; an
entry may keep an explicit gate after promotion.

## Current promoted work

### `PIXIE-TODO-001` — Establish the documentation foundation

**Status:** in-progress

**Source:** owner decision on 2026-10-09

**Accepted scope:** the root `README.md`, `ROADMAP.md`, `TODO.md`,
`TASKLIST.md`, and `DEPENDENCIES.md`; the documentation policy, schema, templates, proposal index,
and archive under `docs/`; and the unversioned `.local/` convention. The model
is derived from NekoLib's and adapted. No code, build files, or modules.

**Dependencies and gates:**

1. Owner review of the structure and the policy.
2. The initial commit requires separate owner authorization.

**Completion criteria:**

- every file in the accepted scope exists with valid metadata, checked by
  review because no verifier exists yet;
- every relative link resolves;
- the owner accepts the policy, or its corrections are applied; and
- the result is committed.

**Breakdown:** none
