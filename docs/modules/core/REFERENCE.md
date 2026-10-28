# Core Technical Reference

**Document ID:** CORE-REFERENCE

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** core ownership, dependencies and lifecycle

**Surface:** technical-reference

**Boundary:** core

**Authority role:** normative

**Mutation:** authored

**Indexing:** include

Core owns logging, explicit frame time and fixed stepping, rate sampling, CPU telemetry, and CPU image decoding/encoding. It does not own a window, input routing, GL context, GPU resource or simulation World.

Its native source depends on PixieLib primitives/math and private stb image headers. No Platform, Render, App, GLFW or GLEW header belongs in this boundary. An Image is CPU-owned RGBA8 memory; GPU upload and readback belong to Render. Telemetry remains caller-owned and used on its owner thread.

Headers live in `pixie/core/`; the C++ namespace remains `pixie`. Out-of-line symbols use `PIXIE_CORE_API`; header-only code remains header-only. Shared runtime/toolchain requirements apply when C++ values cross the shared-engine boundary.

## Packaging and evidence

The internal object target is compiled with its lower-boundary dependencies only. The public consumer facade reaches the shared engine built by the root CMake target pixie_shared (output pixie.dll on Windows); this does not turn every included boundary into a permitted implementation dependency. Native C++ consumers must match toolchain/runtime settings. Core/Platform/Render/App ownership was established by the split; shared packaging is being qualified under [PIXIE-TODO-051](../../../TODO.md#pixie-todo-051--shared-engine-game-export-and-local-mcp-development-channel).

The [qualification matrix](../../shared-engine-export-validation.md) records bounded results and remaining gaps. A Debug build/test result does not establish Release, CI, Linux, export, MCP or final owner acceptance.

