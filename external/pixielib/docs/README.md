# Documentation Index

**Document ID:** GLOBAL-DOCUMENTATION-INDEX

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** index of the PixieLib documentation

**Surface:** index

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

Where each kind of fact lives is decided by the
[documentation policy](governance/documentation-policy.md); this index only
routes.

## At the root

- [`README.md`](../README.md): what PixieLib is, how to get it, and examples.
- [`ROADMAP.md`](../ROADMAP.md): what PixieLib is and is not, who uses it,
  direction, and guardrails.
- [`TODO.md`](../TODO.md) and [`TASKLIST.md`](../TASKLIST.md): promoted work and
  its breakdowns.
- [`DEPENDENCIES.md`](../DEPENDENCIES.md): every dependency and its status.

## Contracts

- [`layout.md`](layout.md): the layout of every type both languages share, and
  the semantics both sides repeat.
- The modules, one boundary each, with a manifest, an introduction, and the
  technical reference:

| Module | Covers | Languages |
|---|---|---|
| [`primitives`](modules/primitives/README.md) | Point, size, rectangle, region, padding, RGBA and HSL colors, their text and conversions | C++, C# |
| [`math`](modules/math/README.md) | Vectors, matrices, 2D transforms, projections, segment, the arithmetic between points and vectors | C++, C# |
| [`events`](modules/events/README.md) | Callbacks, event handlers, events, subscriptions | C++ |
| [`generator`](modules/generator/README.md) | The source generator that writes the C# types in three precisions | C# |

## Working

- [`development.md`](development.md): building, checking, comparing bits, CI,
  and releases.
- [`proposals/`](proposals/README.md): unpromoted ideas.
- [`archive/`](archive/README.md): the history, one record per event, and the
  documents written before the policy.

## Structure

- [`governance/documentation-policy.md`](governance/documentation-policy.md):
  authority, lifecycle, scheduling, archive, and commit rules.
- [`schemas/`](schemas/README.md): the structural schema and its derivation.
- [`templates/`](templates/): the shapes of new documents.
- [`notas.md`](notas.md): a portal from the old path of the decision notes.
