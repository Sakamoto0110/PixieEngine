# Engine Module Boundaries

**Document ID:** GLOBAL-PROPOSAL-ENGINE-MODULES

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** initial split of the engine into modules and their dependency direction

**Surface:** proposal

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## PIXIE-PROPOSAL-001

**State:** idea

**Idea:** Split the engine into these boundaries, with dependencies pointing
only downward in this list:

| Boundary | Language | Artifact | Owns |
|---|---|---|---|
| `core` | C++ | static library | Logging, asserts, time, file access, IDs and handles |
| `platform` | C++ | static library | Window, input, and GL context; GLFW in the player, a host-provided surface in the editor |
| `render` | C++ | static library | Thin OpenGL 4.5 wrapper and the 2D renderer |
| `assets` | C++ | static library | Asset handles, loaders, and cache |
| `scene` | C++ | static library | The Scene document: entities, components, hierarchy, serialization, reflection |
| `runtime` | C++ | static library | The World: instantiation from a Scene, systems, fixed-step loop, play and stop |
| `capi` | C++ | shared library | The C ABI the editor consumes; nothing else depends on it |
| `player` | C++ | executable | The deployable game: links the engine statically and loads a packaged project |
| `editor` | C# | managed assemblies | Interop over `capi`, viewport, hierarchy, inspector through InteractiveEditor, play mode, export |
| `tools` | to be decided | to be decided | Cook and export from editor assets to runtime formats |

PixieLib sits below `core`. Third-party code sits outside the boundaries.

**Rationale:** The runtime never depends on the editor, while the editor
depends on the runtime. The Scene (document) and the World (live instance) are
separate, so play mode and the player share one path. Linking the engine
statically into the player yields one executable plus data; the editor
receives one native shared library.

**Constraints:** Physics, audio, scripting, and 3D are outside the first cut.
Boundary keys become `docs/modules/<boundary>/` only when their code exists.

**Promotion target:** owner decision required

**Source:** [inception record](../archive/inception-2026-10-09.md)
