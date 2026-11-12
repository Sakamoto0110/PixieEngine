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

Platform owns the player window, context activation and presentation, device input and frame limiting. The internal object target depends on Core and GLFW. Render independently owns GL loading, viewport commands and GPU resources. App owns player input routing and debug-console policy.

Headers use `pixie/platform/`. Exported out-of-line symbols use `PIXIE_PLATFORM_API`. Native consumers share a single GLFW process instance with the engine. The Windows build uses `glfw3.dll`.

## Window and host contexts

`Surface` gives a host two operations: make its context available and report physical framebuffer size. For the GLFW-backed `Window`, `Create` takes window dimensions in GLFW screen coordinates, requests OpenGL 4.5 core and sets the visibility hint before creating the window. The default is visible. An owning window destroys its handle and terminates the GLFW session on destruction. The constructor taking `GLFWwindow*` borrows an existing handle and leaves ownership and GLFW lifetime with the host. `Handle()` is a borrowed callback/presentation integration handle.

Use the GLFW event thread for window creation and event dispatch. Make the context current on the rendering thread before GL operations, including `SetVsync` and presentation. `Close` sets the close flag. `PollEvents` dispatches pending events, while `WaitEvents` can wait with a timeout in seconds. `GetFramebufferSize` reports physical pixels independently of logical window dimensions. Release Render resources before releasing the context.

`HostSurface` stores the requested viewport dimensions and `Resize` updates that value only. The host creates, makes current and destroys its own context. `HostSurface::MakeCurrent` leaves the active context as supplied by the host. The host must keep the correct context current for each native GL operation, including teardown. An embedded C API session follows that ownership model.

## Frame limiting

`SetFrameRateCap` takes frames per second. Zero disables the wait, and positive finite rates set a fresh deadline. `WaitForFrame` uses the private `FrameLimiter` schedule and is separate from the swap interval. The limiter waits near the deadline, yields for the final interval and resynchronizes after a long stall. A frame cap controls presentation pacing. Fixed update frequency belongs to the step driver.

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

The internal object target is compiled with its lower-boundary dependencies only. The public consumer facade reaches the shared engine built by the root CMake target pixie_shared (output pixie.dll on Windows); this does not turn every included boundary into a permitted implementation dependency. Native C++ consumers must match toolchain/runtime settings. Core/Platform/Render/App ownership was established by the split; shared packaging is being qualified under [PIXIE-TODO-051](../../archive/todo-051-shared-engine-export-mcp-2026-11-01.md#pixie-todo-051--shared-engine-game-export-and-local-mcp-development-channel).

The [qualification matrix](../../shared-engine-export-validation.md) records bounded results and remaining gaps. A Debug build/test result does not establish Release, CI, Linux, export, MCP or final owner acceptance.

## Ordered device input

`Input` stores held state, per-update transients and an ordered event stream. Feed it on the same owner thread that reads it. The `Key`, `MouseButton`, `InputAction` and `InputModifiers` values match GLFW; the scan code is host data. The bool overloads synthesize transitions and derive modifiers from held keys. The action overloads preserve supplied modifiers, reject duplicate edges and accept key repeat only for a held key. Mouse buttons have no repeat action.

Feed cursor coordinates in physical framebuffer pixels from the top-left with Y down. A movement event carries its delta from the preceding cursor position. A button or scroll event snapshots the most recent cursor position. Scroll offsets are accumulated as supplied (positive Y is away from the user). Polling queries describe the final accumulated state, so a press and release can both be true for a control no longer down. `Events()` borrows an ordered span valid only until the host mutates `Input`. Do not mutate during dispatch.

Call `EndUpdate` after a fixed update consumes the state. It clears events, scroll and edges, advances the cursor-motion baseline and keeps held controls. Transients persist across frames that run no updates. On focus loss or a modal input handoff, `ReleaseAll` discards pending presses, movement and scroll and exposes cancellation releases. The current cursor position is retained. Repeated cancellation before consumption keeps one pending release per affected control. For game callbacks, polling and capture compatibility, see the [entity input guide](../../entity-input-and-code-components.md#device-contract).
