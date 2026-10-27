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

**Last checked:** 2026-10-13

Vendored third-party sources live under `external/<name>/`, unmodified from
the release archive whose hash is recorded below. Where no hash is published
(GitHub tag archives), the recorded hash is the one computed on download.

This is the master list of what PixieEngine and its samples depend on:
toolchain, graphics, the owner's own projects, and candidates under
consideration. It owns which dependency is used, at which version, why, and its
current status. The CMake files own the wiring. Dependencies of the owner's
other projects, such as PixieInspector's GUI binding, belong to their own
`DEPENDENCIES.md`.

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
| Visual Studio Community 2026 (MSVC) | C++ compiler | 18.10; MSVC toolsets 14.51, 14.44, 14.16 | — | `ok` | Builds the native side in C++20 (accepted 2026-10-10), with the C++ runtime linked statically (accepted 2026-10-11), so executables do not need the Visual C++ Redistributable. |
| CMake | Native build system (accepted 2026-10-10) | 4.3.1, bundled with Visual Studio | — | `ok` | Not on `PATH`; available from the Visual Studio developer shell. A standalone install is needed only to build outside Visual Studio or on another OS. GLFW 3.5.1 builds through its own `CMakeLists.txt`. |
| Ninja | Native build backend | Bundled with Visual Studio | — | `ok` | Same `PATH` note as CMake. |
| .NET SDK | The C# editor (`editor/`) and PixieInspector | 10.0.401 and 10.0.301 | — | `ok` | The editor targets `net10.0` and builds with `dotnet build editor/PixieEditor.slnx` (2026-10-13). |
| Git | Version control | 2.55.0 | — | `ok` | |
| Python | The documentation verifier, `scripts/check_docs.py` | 3.14.7 | — | `ok` | Standard library only. Any Python 3.10 or newer runs it. |

## Graphics and platform

| Dependency | Used for | In use | Latest | Status | Notes and suggestions |
|---|---|---|---|---|---|
| OpenGL 4.5 core | Rendering | AMD Radeon RX 7600, driver 32.0.31041.1004 (2026-08-16) | — | `ok` | Confirmed on 2026-10-11: the hello-gl sample logs `4.5.0 Core Profile Context 26.8.1.260810`. |
| GLEW | GL function loader | 2.3.1 source release, unmodified, in `external/glew/`; archive `glew-2.3.1.tgz`, SHA-256 `b64790f94b926acd7e8f84c5d6000a86cb43967bd1e688b03089079799c9e889` | 2.3.1 (2026-01-24) | `ok` | Built as a static library in Debug and Release (2026-10-11). Modified BSD and MIT licenses (`external/glew/LICENSE.txt`). Adopted 2026-10-09 as the starting loader. Build from source as a static library (`GLEW_STATIC`). Under the core profile: set `glewExperimental = GL_TRUE` before `glewInit()`, then drain `glGetError()` once before installing the debug callback, because `glewInit()` leaves a spurious `GL_INVALID_ENUM`. On Wayland/EGL hosts, GLEW must be built with EGL support. |
| stb_easy_font | Debug text in the hello-gl sample | `stb_easy_font.h` and `LICENSE` from `nothings/stb` at commit `2c980bb59875b0d32144a71867fbdebb2f77cd20`, unmodified, in `external/stb/`; header SHA-256 `7b48b52a316477766c0a7a096b33a29bcbd847b6fab4fabd32bb3fdb7cea1a60` | No releases; `master` at the pinned commit (2026-08-02) | `ok` | MIT or public domain. Text built from quads, no texture. Adopted 2026-10-11 for the sample only. |
| stb_image | PNG images for sprites | `stb_image.h` (v2.30) from `nothings/stb` at commit `2c980bb59875b0d32144a71867fbdebb2f77cd20`, the same commit and `LICENSE` as stb_easy_font, unmodified, in `external/stb/`; header SHA-256 `594c2fe35d49488b4382dbfaec8f98366defca819d916ac95becf3e75f4200b3` | No releases; `master` at the pinned commit (2026-08-02) | `ok` | MIT or public domain. Adopted 2026-10-15 with PIXIE-PROPOSAL-007. Its implementation is compiled once, in the engine's `Image.cpp`, for PNG only (`STBI_ONLY_PNG`) and from memory (`STBI_NO_STDIO`), so file names never reach it. Included as a `SYSTEM` header, so its warnings stay out of the engine's `/W4 /WX`. |
| stb_truetype | Text: TrueType fonts rasterized into atlases | `stb_truetype.h` (v1.26) from `nothings/stb` at commit `2c980bb59875b0d32144a71867fbdebb2f77cd20`, the same commit and `LICENSE` as stb_easy_font, unmodified, in `external/stb/`; header SHA-256 `ecd30b05e0dd4fea3a13c26810dd9e1992dc379049482c393d5a19e6b5090aab` | No releases; `master` at the pinned commit (2026-08-02) | `ok` | MIT or public domain. Adopted 2026-10-15 with PIXIE-TODO-030. Its implementation is compiled once, in the engine's `Font.cpp` (`STBTT_STATIC`), and reads fonts from memory, so file names never reach it. Only fonts with `glyf` outlines rasterize; CFF fonts are refused. Included as a `SYSTEM` header, so its warnings stay out of the engine's `/W4 /WX`. |
| Inter | The particles sample's screen-layer font, and the text tests' | `inter-regular` from the shared resources (`vdr cp inter-regular`): Inter 4.001 Regular, the static TTF of the official `rsms/inter` release 4.1, unmodified, in `samples/particles/fonts/` with its license `inter-license.txt`; SHA-256 `40d692fce188e4471e2b3cba937be967878f631ad3ebbbdcd587687c7ebe0c82` | 4.1 (2024-11-16) | `ok` | SIL Open Font License 1.1; the license file travels with each copy. An asset, not code: the engine ships no font, and a project copies one into its folder. Adopted 2026-10-15 with PIXIE-TODO-030. |
| GLFW | Player window and input | 3.5.1 source release, unmodified, in `external/glfw/`; archive `glfw-3.5.1.zip`, SHA-256 `ea79bc5feffc254c87291980c2d0bce9acebb68c4983b79f961dcd2cb8a611a0` | 3.5.1 (2026-07-31) | `ok` | Built through its own CMake project in Debug and Release (2026-10-11). zlib/libpng license (`external/glfw/LICENSE.md`). |

## Runtime

| Dependency | Used for | In use | Latest | Status | Notes and suggestions |
|---|---|---|---|---|---|
| EnTT | Component storage of the World (entities, components, views) | v4.0.0 source, unmodified, in `external/entt/`; archive `v4.0.0.tar.gz` (GitHub tag archive), SHA-256 `32a2ff2c72cb047dfd57306006ef238820b70da7c6ce4e7e8a507ac63365212e` | v4.0.0 (2026-07-23) | `ok` | MIT license. Adopted 2026-10-13; used header-only from `external/entt/src` through the `EnTT::EnTT` interface target, not through its own CMake project. v4.0.0 requires C++20. Components stay plain structs, so the storage can be replaced later. |
| GLM | Vectors and matrices behind PixieLib's math | 1.0.3 release, unmodified, in `external/glm/`; archive `glm-1.0.3.zip`, SHA-256 `1c0a0fced9b0d87c7b7bc94e40be490cff6d4c83c25db8488d8f33754e7fdeb2` (the digest GitHub publishes for the asset) | 1.0.3 (2025-12-31) | `ok` | MIT or Happy Bunny license (`external/glm/copying.txt`). Brought by PixieLib (`pixie::math`), which pins 1.0.3; given to it as its `FetchContent` source. Not used directly by the engine. |
| nlohmann/json | Reading and writing scene files (`runtime/src/Scene.cpp`) | v3.12.0 single header and license, unmodified, in `external/nlohmann_json/`; release asset `json.hpp`, SHA-256 `aaf127c04cb31c406e5b04a63f1ae89369fccde6d8fa7cdda1ed4f32dfc5de63`; `LICENSE.MIT` from the tag, SHA-256 `46a65cffd1ea955132d95a8dd921640714a8d6b537d2e4e482d31145ae95b603` | v3.12.0 (2025-04-11) | `ok` | MIT license. Adopted 2026-10-14 for the scene format (PIXIE-PROPOSAL-005, PIXIE-TODO-018). Header-only, through the `nlohmann_json::nlohmann_json` interface target; linked privately by the runtime library, so no public header includes it. GitHub publishes no digest for the asset; the hash was computed on download. Readable text scenes for diffs and debugging; a binary format can come later. |

### Collision backend (adopted 2026-10-22)

| Dependency | Used for | In use | Latest observed | Status | Notes |
|---|---|---|---|---|---|
| Box2D | Private runtime collision queries and capsule contact primitives | 3.2.0, unmodified source under `external/box2d/`, except its `.gitattributes`, left out because a nested one would override the root `-text` rule | 3.2.0, checked 2026-10-22 | `ok` | Owner selected backend plus our character controller. MIT (`external/box2d/LICENSE`). Static build through upstream CMake, single precision; upstream samples/tests are not built as a subproject. Public runtime headers expose no b2 types. Archive `https://codeload.github.com/erincatto/box2d/zip/refs/tags/v3.2.0`, SHA-256 `ed94013e8d16b19876757920bfa6d7663358aef8ba9407cc18146da6b714029e`. Runtime queries use one worker; no implicit global physics stepping. |

## Editor side

The engine has no dependency on a GUI toolkit or binding. Its contract with an
editor host is an active OpenGL 4.5 core context and a texture to display; any
host that provides that works.

The C# editor (`editor/`, since 2026-10-13) takes one PixieInspector view
package, and its host uses the binding and the version that package brings:
`PixieInspector.ImGui` 0.3.0 brings Hexa.NET.ImGui 2.2.9, which the editor uses
with Hexa.NET.GLFW 1.0.5 and Hexa.NET.ImGui.Backends and
Hexa.NET.ImGui.Backends.GLFW 1.0.18, the versions PixieInspector's own ImGui
host uses (`editor/PixieEditor/PixieEditor.csproj`). `PixieInspector.Avalonia`
0.1.0 would bring Avalonia 12.1.3. Those are dependencies of the editor
project, not of the engine. A change of binding is PixieInspector's decision;
its own `DEPENDENCIES.md` lists them.

## Own projects

| Dependency | Used for | In use | Latest | Status | Notes and suggestions |
|---|---|---|---|---|---|
| PixieLib (C++) | Primitives and math for the native engine | v0.2.0 source, unmodified, in `external/pixielib/`; archive `v0.2.0.tar.gz` (GitHub tag archive), SHA-256 `942d70a7b34be9cdd927eb675c4d304d9a6f463b0c7b04f54bcd5feae45f3389` | v0.2.0 (`Sakamoto0110/PixieLib`, tag `v0.2.0`, 2026-10-12) | `ok` | MIT license. Built through its own CMake project (`external/pixielib/cpp`) as a `SYSTEM` subdirectory; the engine links `pixie::pixie` and `pixie::math` (2026-10-13). Its GLM `FetchContent` takes the vendored `external/glm/` through `FETCHCONTENT_SOURCE_DIR_PIXIE_GLM`, so a configure never downloads it. 0.2.0 separates points from vectors. The archive matches the tag in the local checkout, line endings aside. |
| PixieLib (.NET) | Primitives for the C# editor | 0.2.0, through `PixieInspector.ImGui` | 0.2.0 on nuget.org (`net10.0`, `net481`) | `ok` | Same layout as the C++ side (`docs/layout.md` in PixieLib): unsuffixed types are `double` in both languages. The editor receives 0.2.0 through PixieInspector, whose core depends on it, and uses `PxColorRgba` in its proxies; a direct reference must be 0.2.0 or newer. |
| PixieInspector | The inspector of the C# editor | `PixieInspector.ImGui` 0.2.1, referenced by `editor/PixieEditor/PixieEditor.csproj` (since 2026-10-13; 0.2.0 and 0.1.0 before, the same day) | 0.2.1 on nuget.org (2026-10-13; MIT; tag `v0.2.1` of `Sakamoto0110/PixieInspector`), whose ImGui view draws a mixed row's label in amber, with a tooltip; 0.2.0, the same day, added fields with their own readers and writers (`AddField`), display units, and an arc editor: `PixieInspector` (core), `PixieInspector.ImGui`, `PixieInspector.Avalonia`, `PixieInspector.WinForms`, `PixieInspector.Wpf` (the last two `net10.0-windows`) | `ok` | Consumed only as a package: no project reference, copied code, or commit reference. The ImGui editor references `PixieInspector.ImGui` alone, which brings the core (0.2.1, the same exact version), PixieLib 0.2.0, and Hexa.NET.ImGui 2.2.9. Each view requires the core at the same exact version, because it uses the core's internals; forcing another core version raises NU1608. A change to the inspector is made in its repository and returns as a new package version; below 1.0, a breaking change raises the minor version. The local checkout `dev/PixieInspector` (renamed from `dev/InteractiveEditor` on 2026-10-13) is only for developing the inspector; the engine consumes PixieInspector from NuGet, as the `PixieInspector.ImGui` package. |

## Candidates

Suggested and not adopted. A candidate moves to a table above when the owner
adopts it.

| Candidate | Would be used for | Latest | Why it is suggested |
|---|---|---|---|
| glad 2 | GL function loader, if GLEW is replaced | v2.0.8 (2024-09-29); `glad2` branch active (2026-09-28) | Generated for exactly `gl:core=4.5`, so a call outside 4.5 core fails to compile; takes any loader callback, independent of who created the context. |
| miniaudio | Audio | 0.11.25 (2026-03-03) | Single header; no shared library to ship. |


### Inspector 0.3.0 consumer adoption (2026-10-27)

The editor selects PixieInspector.ImGui 0.3.0 and its exact core pair [0.3.0],
from the coordinated core/four-view release. The existing Hexa.NET binding and
backends remain pinned as above; this is not a GUI binding migration. The core
still requires the compatible PixieLib 0.2.0 managed package. The separate
PixieLib 0.2.1 release includes a native Forward correction and does not imply
that the Engine's vendored C++ snapshot was replaced.

The adoption gate requires an isolated restore using only the public nuget.org
source, package repository-commit verification, an Editor Release build, bounded
EngineSession/native checks and PR/main CI. GitHub release-asset validation is
separate evidence and cannot satisfy the public-restore gate by itself.
Runtime-named category/mask bridging for particle profiles remains a separate
Engine UI integration task; selecting the package does not implement that bridge.
