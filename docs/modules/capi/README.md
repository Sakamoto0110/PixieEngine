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

Start with `capi/include/pixie_capi.h`. A host creates its own OpenGL 4.5 core context, makes it
current, loads `pixie.dll`, selects a development module when needed, and calls
`pixie_session_create`; each frame it feeds input, calls `pixie_session_advance` with the frame's
wall time and `pixie_session_render`, and displays the returned texture.

The C API session path never creates a player window: the host owns the window
and context. Each session retains its selected module until its game-owned
objects are destroyed. Changing the default module does not change existing
sessions. The same engine DLL also contains the native player facilities;
their presence does not transfer context ownership from a C API host.

- [Manifest](MANIFEST.md): build target, dependencies, and routes.
- [Technical reference](REFERENCE.md): the contract.
