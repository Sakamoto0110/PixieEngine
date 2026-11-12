# C API

**Document ID:** CAPI-README

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** introduction to the shared engine C interface and development game modules

**Surface:** introduction

**Boundary:** capi

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

`capi` is short for "C API". It exposes the shared engine through a C interface,
so the C# editor, tools and tests can host it. Development game modules supply
game code through a versioned factory. The engine runs the game's `Project` in a session, draws it into an OpenGL texture of the host's
context, takes the host's input, and lists entities, component types, and fields, which it reads
and writes through the runtime's field descriptors.

Use `include/pixie_capi.h` from the selected SDK (source declaration
`capi/include/pixie_capi.h`). Load `pixie.dll` and check the ABI, SDK
identity and capabilities. A host with module-loading support can select a
compatible development game module. To create a session, make the host's
OpenGL 4.5 core context current and call `pixie_session_create`. Each frame,
feed input, call `pixie_session_advance` with elapsed seconds, then
`pixie_session_render` and display the returned texture. The session owns
the texture. Destroy the session with the same context current.

The C API session path never creates a player window: the host owns the window
and context. Each session retains its selected module until its game-owned
objects are destroyed. Changing the default module does not change existing
sessions. The same engine DLL also contains the native player facilities;
their presence does not transfer context ownership from a C API host.

- [Manifest](MANIFEST.md) for the build target, dependencies and source routes.
- [Technical reference](REFERENCE.md#sdk-identity-and-profiles) for SDK profiles,
  module admission and the session/field contracts.
