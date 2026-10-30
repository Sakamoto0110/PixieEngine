# App Technical Reference

**Document ID:** APP-REFERENCE

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** app ownership, dependencies and lifecycle

**Surface:** technical-reference

**Boundary:** app

**Authority role:** normative

**Mutation:** authored

**Indexing:** include

App composes Core timing/telemetry, Platform context/input/presentation and Render drawing. It owns App, Run, run configuration/options, the native debug console and overlay policy. Lower boundaries never depend on App.

Run creates a Window, makes its Surface current, loads GL through Render, starts the App, drives explicit fixed Update and interpolated Render calls, draws the debug overlay, captures when requested and presents through Platform. Callback input/debug composition currently uses GLFW handles in App; it must not move context creation or GPU drawing back into this layer.

Destroy overlay and application GPU resources before the owning Window destroys the context. The callback state outlives the Window. Host-owned C API sessions do not run this player loop or acquire a player window merely because the same shared engine contains App.

The existing Pixie::Engine umbrella remains a consumer route through App's public facade. Public facades link the shared artifact; they do not permit reverse includes among the internal object targets. Out-of-line symbols use `PIXIE_APP_API`, and inline/template game extension points remain available to consumers. Existing run options retain their meaning while TODO-051's new capture/development options are being qualified.

## Packaging and evidence

The internal object target is compiled with its lower-boundary dependencies only. The public consumer facade reaches the shared engine built by the root CMake target pixie_shared (output pixie.dll on Windows); this does not turn every included boundary into a permitted implementation dependency. Native C++ consumers must match toolchain/runtime settings. Core/Platform/Render/App ownership was established by the split; shared packaging is being qualified under [PIXIE-TODO-051](../../../TODO.md#pixie-todo-051--shared-engine-game-export-and-local-mcp-development-channel).

The [qualification matrix](../../shared-engine-export-validation.md) records bounded results and remaining gaps. A Debug build/test result does not establish Release, CI, Linux, export, MCP or final owner acceptance.
