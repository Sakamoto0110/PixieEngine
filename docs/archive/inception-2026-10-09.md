# Project Inception: Scope and Early Rationale

**Document ID:** ARCHIVE-INCEPTION-20261009

**Schema version:** 1

**Kind:** audit

**Lifecycle:** historical

**Subject:** initial scope discussion, accepted early decisions, and the rationale behind open recommendations

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-002

**Type:** decision

**Origin:** owner decisions during the scope discussion

**Author:** claude

**Reference date:** 2026-10-09

**Commits:** `f081b81ddd4ec3c9301a28aba581bb1a94681dbf`

**Reference commit:** none (before the initial commit)

**Current state:** [`ROADMAP.md`](../../ROADMAP.md), [`docs/proposals/`](../proposals/README.md)

This record preserves the reasoning of the discussion that started the
repository, from 2026-10-08 to 2026-10-09. It is true only for that baseline.
Accepted decisions are owned by `ROADMAP.md`; recommendations that were not
accepted remain proposals or open questions.

## Starting point

- The repository directory was empty and not under version control.
- PixieLib existed as a C++ library (`Sakamoto0110/PixieLib`, idle since
  2023-08) with a C# side under design. No local clone was present.
- InteractiveEditor existed as a C# reflection-based inspector with WinForms
  and WPF views; cross-platform hosts were in progress.
- The owner proposed C++17 or C++20, OpenGL with GLFW and GLEW, PixieLib for
  primitives, and InteractiveEditor as the editor's binder.

## Accepted during the discussion

- **2D first.** The renderer and the math needed first are 2D.
- **OpenGL 4.5 core.** Rationale below.
- **PixieLib's role:** basic primitives plus forwarding to math libraries.
- **The editor is gaining cross-platform hosts.** This weakens any
  Windows-only editor assumption.
- **Documentation:** English, the structure in the
  [documentation policy](../governance/documentation-policy.md), derived from
  the NekoLib model.
- **Location:** the repository stays in its current directory until the agent
  workplace tooling is reliable enough to host it.

## Rationale: OpenGL 4.5 core

- **Direct state access (4.5).** Objects are created and edited by name
  (`glCreateTextures`, `glTextureStorage2D`, `glNamedBufferData`) and bound only
  to draw. Classic bind-to-edit leaves global binding state behind, the most
  common source of silent renderer bugs.
- **Debug output (4.3, core).** `glDebugMessageCallback` reports errors with a
  readable message at the offending call instead of polling `glGetError`.
- **Immutable storage and persistent mapping (4.4),** separate vertex format
  (4.3), and shader-declared bindings (4.2) match a 2D sprite batcher.
- **4.5 rather than 4.6:** 4.6 adds mainly SPIR-V shaders, which a 2D renderer
  does not need.
- **Known cost:** macOS stops at OpenGL 4.1. A macOS port would be a separate
  rendering backend, not a lower GL version. On Windows, Intel integrated GPUs
  older than Haswell are excluded. Linux (Mesa, NVIDIA) supports it.

## Recommendations not yet accepted

These were offered with reasons and remain open at this baseline.

- **C++20 without modules.** Concepts for the templated primitives, `std::span`
  for GPU uploads, designated initializers for renderer descriptors, defaulted
  comparisons, `std::source_location` for logging and asserts, `<bit>`,
  `consteval` type IDs, and `std::format`. Modules were excluded because the
  tooling is still immature. PixieLib C++ must still be checked under C++20.
- **glad 2 instead of GLEW.** It is generated for the exact GL version, has no
  library to link, and also loads WGL. GLEW requires `glewExperimental` under
  the core profile.
- **Scene versus World.** The Scene is the serializable document the editor
  edits; the World is the live runtime instantiated from it. Play in the
  editor copies a Scene into a World and discards it afterwards.
- **Native bridge.** A C ABI consumed through P/Invoke, with C# proxy classes
  whose properties call into the engine, so InteractiveEditor's
  reflection-based binding reads live values. Cost: each component is defined
  twice until a generator exists, and a native crash takes the editor down.
- **Editor viewport.** GLFW owns its window and cannot render into a foreign
  native window. Options considered: a per-host native surface, or offscreen
  rendering whose pixels the host displays. The owner took this question.
- **Build system.** Visual Studio projects in the editor's solution were
  recommended for a Windows-only editor. Cross-platform hosts make CMake the
  more natural choice.

The module boundaries and the vertical-slice plan from the same discussion are
kept as proposals.
