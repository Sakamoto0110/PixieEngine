# Platform Technical Reference

**Document ID:** PLATFORM-REFERENCE

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** platform ownership, dependencies and lifecycle

**Surface:** technical-reference

**Boundary:** platform

**Authority role:** normative

**Mutation:** authored

**Indexing:** include

Platform owns the player window, context creation/currentness/presentation, input data and frame limiting. It may depend on Core and GLFW; it must not include or link the Render implementation.

Surface carries context activation and framebuffer size. Window implements the GLFW-backed surface and owns the session it creates; an explicitly supplied GLFW handle is borrowed. HostSurface borrows an already-current host context and stores the requested viewport size. It never creates, switches or destroys that host context. The host must satisfy current-context/thread requirements before every native rendering operation.

GL loading, viewport GL commands, render targets and GPU resources belong to Render. Window creation/presentation uses GLFW, not renderer calls. App owns game/debug input policy; forwarding callbacks does not move DebugConsole into Platform. FrameLimiter stays an internal implementation used through Window.

Headers live in `pixie/platform/`; exported out-of-line symbols use `PIXIE_PLATFORM_API`. GLFW must have one shared process instance across the engine and native consumers; the current build uses glfw3.dll on Windows.

## Packaging and evidence

The internal object target is compiled with its lower-boundary dependencies only. The public consumer facade reaches the shared engine built by the root CMake target pixie_shared (output pixie.dll on Windows); this does not turn every included boundary into a permitted implementation dependency. Native C++ consumers must match toolchain/runtime settings. Core/Platform/Render/App ownership was established by the split; shared packaging is being qualified under [PIXIE-TODO-051](../../../TODO.md#pixie-todo-051--shared-engine-game-export-and-local-mcp-development-channel).

The [qualification matrix](../../shared-engine-export-validation.md) records bounded results and remaining gaps. A Debug build/test result does not establish Release, CI, Linux, export, MCP or final owner acceptance.

