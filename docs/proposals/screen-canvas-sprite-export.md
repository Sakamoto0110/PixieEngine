# Export the sized ScreenCanvas sprite overload

**Document ID:** PIXIE-PROPOSAL-019

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** Restore shared-library access to the existing sized and tinted sprite API

**Surface:** proposal

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## Scope

The owner requested completion of the LogicSandbox build on 2026-11-02.
The existing ScreenCanvas::Sprite overload accepting size, tint and filter lacks
PIXIE_RUNTIME_API, causing LNK2019 for shared-engine consumers. Add the export
annotation without changing rendering behavior or the function signature.

## Validation

Build all LogicSandbox Release targets and run its CTest suite. Record results
in the consumer build report. This correction makes no performance claim.