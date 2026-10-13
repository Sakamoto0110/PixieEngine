# C API

**Document ID:** CAPI-README

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** introduction to the capi module: one shared library per game, with a C-only interface

**Surface:** introduction

**Boundary:** capi

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

`capi` is short for "C API". It turns a game into a shared library whose interface is C only, so
that any language able to call C can host it: the C# editor through P/Invoke, tools, and tests. The
library runs the game's `Project` in a session, draws it into an OpenGL texture of the host's
context, takes the host's input, and lists entities, component types, and fields, which it reads
and writes through the runtime's field descriptors.

Start with `capi/include/pixie_capi.h`. A host creates its own OpenGL 4.5 core context, makes it
current, loads the game's library (for example `bin/Release/boids-capi.dll`), and calls
`pixie_session_create`; each frame it feeds input, calls `pixie_session_advance` with the frame's
wall time and `pixie_session_render`, and displays the returned texture.

The library never creates windows or calls GLFW; the host owns the window and the context. One
library hosts one game, the one whose `CreateProject` it was linked with.

- [Manifest](MANIFEST.md): build target, dependencies, and routes.
- [Technical reference](REFERENCE.md): the contract.
