# PixieEngine Dependencies

**Document ID:** GLOBAL-DEPENDENCIES

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** every dependency of the engine and its editor, with status and suggestions

**Surface:** dependency-registry

**Boundary:** global

**Authority role:** normative

**Mutation:** authored

**Indexing:** include

**Last checked:** 2026-10-09

This is the master list of what PixieEngine depends on: toolchain, graphics,
UI, the owner's own projects, and candidates under consideration. It owns which
dependency is used, at which version, and its current status. Build files will
own the wiring once they exist.

## Status

| Status | Meaning |
|---|---|
| `ok` | Present where it is consumed, at a current or deliberately pinned version |
| `outdated` | Present, but a newer release exists and the gap matters |
| `not-installed` | Not present yet |

"Latest" is the newest upstream release seen on the `Last checked` date.
Statuses are observations from that date, not guarantees. Recheck before a
version change.

## Toolchain

| Dependency | Used for | In use | Latest | Status | Notes and suggestions |
|---|---|---|---|---|---|
| Visual Studio Community 2026 (MSVC) | C++ compiler | 18.10; MSVC toolsets 14.51, 14.44, 14.16 | — | `ok` | Full C++20 support. The language standard is still an open decision. |
| CMake | Native build, if chosen | 4.3.1, bundled with Visual Studio | — | `ok` | Not on `PATH`; available from the Visual Studio developer shell. The build system is still an open decision. A standalone install is needed only to build outside Visual Studio or on another OS. |
| Ninja | Native build backend | Bundled with Visual Studio | — | `ok` | Same `PATH` note as CMake. |
| .NET SDK | Editor and InteractiveEditor | 10.0.401 and 10.0.301 | — | `ok` | |
| Git | Version control | 2.55.0 | — | `ok` | |

## Graphics and platform

| Dependency | Used for | In use | Latest | Status | Notes and suggestions |
|---|---|---|---|---|---|
| OpenGL 4.5 core | Rendering | AMD Radeon RX 7600, driver 32.0.31041.1004 (2026-08-16) | — | `ok` | Inferred from the GPU and driver; the context version has not been queried yet. The first window should log `GL_VERSION`. |
| GLEW | GL function loader | — | 2.3.1 (2026-01-24) | `not-installed` | Adopted 2026-10-09 as the starting loader. Build from source as a static library (`GLEW_STATIC`). Under the core profile: set `glewExperimental = GL_TRUE` before `glewInit()`, then drain `glGetError()` once before installing the debug callback, because `glewInit()` leaves a spurious `GL_INVALID_ENUM`. On Wayland/EGL hosts, GLEW must be built with EGL support. |
| GLFW | Player window and input | — | 3.5.1 (2026-07-31) | `not-installed` | Vendor the 3.5.1 source and build it with the engine. |

## Editor UI

| Dependency | Used for | In use | Latest | Status | Notes and suggestions |
|---|---|---|---|---|---|
| Dear ImGui | Editor UI (tentative) | 1.91.6, through ImGui.NET, in InteractiveEditor | v1.92.9 and v1.92.9b, with `-docking` tags (2026-07-31) | `outdated` | Use the docking branch; see [ImGui version](#imgui-version). |
| ImGui.NET | C# binding used by InteractiveEditor's ImGui view | 1.91.6.1 | 1.91.6.1, released 2025-01-06 | `outdated` | No NuGet release in 21 months, and it is stuck before Dear ImGui 1.92. |
| Hexa.NET.ImGui | C# binding (alternative) | — | 2.2.9 (Dear ImGui 1.92.2b, 2025-10-10); 3.1.0-experimental (1.92.9b, 2026-07-31) | `not-installed` | Docking, DockBuilder, multi-viewport, and native backends (`Hexa.NET.ImGui.Backends`, prerelease and experimental builds only). |
| Silk.NET (Windowing, Input, OpenGL.Extensions.ImGui) | InteractiveEditor's ImGui host | 2.23.0 | 2.23.0 | `ok` | Its ImGui extension requires ImGui.NET; moving to another binding drops the extension. |

## Own projects

| Dependency | Used for | In use | Latest | Status | Notes and suggestions |
|---|---|---|---|---|---|
| PixieLib (.NET) | Primitives for the C# side | NuGet 0.1.0, consumed by InteractiveEditor | 0.1.0 (2026-10-09; MIT; `net10.0`, `net481`) | `ok` | |
| PixieLib (C++) | Primitives for the native engine | — | `Sakamoto0110/PixieLib`, `cpp/` directory, release v0.1.0 | `not-installed` | No local clone. How the engine consumes it (submodule or vendored) is an open decision. |
| InteractiveEditor | Inspector and binder for the editor | Local checkout `dev/InteractiveEditor`, branch `rework-claude` at `a718ce9`, equal to `origin` | `a718ce9` | `ok` | Hosts: WinForms, WPF, Avalonia 12.1.3, ImGui, Terminal.Gui 2.5.0. Remote: `Sakamoto0110/InteractiveEditor_Rework`. |

## Candidates

Suggested and not adopted. A candidate moves to a table above when the owner
adopts it.

| Candidate | Would be used for | Latest | Why it is suggested |
|---|---|---|---|
| glad 2 | GL function loader, if GLEW is replaced | v2.0.8 (2024-09-29); `glad2` branch active (2026-09-28) | Generated for exactly `gl:core=4.5`, so a call outside 4.5 core fails to compile; takes any loader callback, independent of who created the context. |
| stb_image (`nothings/stb`) | Image loading | No releases; pin a commit | Single header, public domain; covers PNG for sprites. |
| miniaudio | Audio | 0.11.25 (2026-03-03) | Single header; no shared library to ship. |
| glm | C++ math | 1.0.3 (2025-12-31) | Mirrors GLSL types. Adopt only if PixieLib C++ forwards to it, since PixieLib owns the math surface. |
| nlohmann/json | Scene and project serialization | v3.12.0 (2025-04-11) | Readable text scenes for diffs and debugging. A binary format can come later for the player. |

## ImGui version

**Suggestion:** Dear ImGui **1.92.x, docking branch**, with **Hexa.NET.ImGui**
replacing ImGui.NET on the C# side.

- **Docking** is what an editor layout needs: hierarchy, inspector, viewport,
  and asset panels as dockable windows. Both bindings expose it, but only Hexa
  tracks the current release.
- **1.92** reworked fonts and textures (dynamic font sizes, texture updates
  from the backend). ImGui.NET has had no release since 2025-01 and is still on
  1.91.6.
- **Which Hexa version:** 2.2.9 is the newest non-experimental build (1.92.2b),
  but it is a year old. The 3.x line tracks upstream (1.92.9b) and is labeled
  experimental. Decide after a short trial in InteractiveEditor's ImGui host,
  where the view's existing checks must still pass.
- **Cost:**
  - Port `InteractiveEditor.ImGui` from the `ImGuiNET` API.
  - Replace `Silk.NET.OpenGL.Extensions.ImGui` with Hexa's GLFW and OpenGL3
    backends or a small custom renderer.
  - Accept a binding whose backend packages are not marked stable.

**Related observation:** with an ImGui editor, the scene viewport can be an
ImGui window that displays a texture the engine rendered offscreen, provided
the engine and the editor share one GL context in one process. This bears on
the open viewport decision; it is not a decision.
