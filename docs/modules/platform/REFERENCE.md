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

GL loading, viewport GL commands, render targets and GPU resources belong to Render. Window creation/presentation uses GLFW, not renderer calls. Window creation accepts initial visibility, defaulting to visible; a hidden host sets the GLFW visibility hint before creation rather than hiding a flashed visible window. App owns game/debug input policy; forwarding callbacks does not move DebugConsole into Platform. FrameLimiter stays an internal implementation used through Window.

Headers live in `pixie/platform/`; exported out-of-line symbols use `PIXIE_PLATFORM_API`. GLFW must have one shared process instance across the engine and native consumers; the current build uses glfw3.dll on Windows.

## Development local transport

With `PIXIE_DEV_CHANNEL`, LocalCommandChannel owns a Windows named-pipe worker
and bounded framing, not a World or session. Start accepts a bounded ASCII
channel name; the native pipe uses a protected owner-SID ACL, rejects remote
clients and permits one instance/client. Other platforms currently report
unsupported transport rather than opening a TCP fallback.

The worker queues at most one command and waits for completion. The host calls
Drain on its owner thread once per frame after advancement and before rendering,
including while paused. A deferred capture completes only after render or an
explicit unavailable-framebuffer error. Stop cancels pending I/O and joins the
worker before session references are released. These are caller ordering and
lifetime obligations; the transport does not itself validate native World state.
The qualification record distinguishes local ACL/lifecycle fixtures from
unexecuted cross-user, remote-network and Unix permission tests.

## Packaging and evidence

The internal object target is compiled with its lower-boundary dependencies only. The public consumer facade reaches the shared engine built by the root CMake target pixie_shared (output pixie.dll on Windows); this does not turn every included boundary into a permitted implementation dependency. Native C++ consumers must match toolchain/runtime settings. Core/Platform/Render/App ownership was established by the split; shared packaging is being qualified under [PIXIE-TODO-051](../../../TODO.md#pixie-todo-051--shared-engine-game-export-and-local-mcp-development-channel).

The [qualification matrix](../../shared-engine-export-validation.md) records bounded results and remaining gaps. A Debug build/test result does not establish Release, CI, Linux, export, MCP or final owner acceptance.
