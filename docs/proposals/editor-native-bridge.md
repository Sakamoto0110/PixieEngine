# Editor Native Bridge

**Document ID:** GLOBAL-PROPOSAL-EDITOR-BRIDGE

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** how the C# editor reaches native engine state

**Surface:** proposal

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## PIXIE-PROPOSAL-002

**State:** idea

**Idea:** The editor reaches the engine in-process through a C ABI (`capi`)
consumed with source-generated P/Invoke. The native Scene is the single source
of truth; C# holds handles only. Each component has a C# proxy class whose
properties call into `capi`, so PixieInspector's reflection-based binding,
which reads through the parent chain on every call, always sees live values.
Proxies are hand-written first and generated from native reflection metadata
later.

**Rationale:** A C ABI is portable across the cross-platform editor hosts,
unlike C++/CLI. Reflection-based binding works on proxies without changes to
PixieInspector. If PixieLib keeps identical memory layouts in C++ and C#,
its primitives cross the bridge without conversion.

**Constraints:** Until a generator exists, each component is defined twice. A
native crash takes the editor down. The identical layouts are PixieLib's
contract (`docs/layout.md` in PixieLib, from 0.2.0): the unsuffixed types are
`double` in both languages, with the same fields, order, and size. The earlier
mismatch, an `int` unsuffixed point in C++, was resolved there.

**Promotion target:** owner decision required

**Source:** [inception record](../archive/inception-2026-10-09.md)
