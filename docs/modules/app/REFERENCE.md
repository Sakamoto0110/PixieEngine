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

App composes Core timing and telemetry, Platform context/input/presentation and Render drawing. It owns `App`, `Run`, configuration, option parsing and native debug-console policy. Lower boundaries have no dependency on App. `Pixie::App` and the `Pixie::Engine` umbrella are consumer routes to the shared engine. Out-of-line symbols use `PIXIE_APP_API`. Public extension points remain in `pixie/app/`.

## Player loop and teardown

`Run` borrows the caller's `App` for the duration of the call. It creates a Platform `Window`, makes its context current and calls Render's `LoadGl` before `App::Start`. The window reference passed to `Start` is usable through `Stop`. App connects GLFW events to the debug console and game input. It scales window-space cursor coordinates to framebuffer pixels for the game. The callback state outlives the window.

In the ordinary player loop, the host polls events and applies debug settings. It accumulates elapsed wall time and invokes `Update` once per due fixed step with a borrowed `FrameTime` and `Input`. After each step it calls `Input::EndUpdate`. A frame with no update can still expose pending transients to `Render`. After one or more updates, the consumed transients have already been cleared. The first update of a batch sees the pending events, and later updates see the held state. See [ordered device input](../platform/REFERENCE.md#ordered-device-input).

For a drawable frame, `Run` sets the viewport, calls `Render` with the interpolation fraction, draws the player overlay, performs any configured final PNG capture and presents. A minimized frame continues to poll and advance while skipping drawing and presentation and waiting briefly for events. Vsync and the frame limiter affect pacing independently of the update rate.

On normal exit `Run` destroys the overlay, calls the development `beforeStop` hook if supplied and calls `App::Stop` with the context still current. Release the application's GPU objects in `Stop`. A failed `Start` also leads to `Stop` for partial initialization, as does a failed overlay creation. A failure before `Start`, such as a window or loader failure, returns without an App lifecycle call. The process result is 0 after normal loop exit and 1 on those startup failures. A failed final screenshot write is logged and does not change the loop result. The caller owns the `App` object after `Run` returns. Embedded C API sessions use a host context and their own driving entry points.

## Configuration and option parsing

`RunConfig` defaults to a visible 1280x720 player titled `PixieEngine`, with 60 updates per second, vsync on, no frame cap, no timed exit and no screenshot. A programmatically supplied update rate must be finite and positive. With no telemetry override, host CPU collection is off and the app controls its own collector. A supplied bool applies to the host and to the app collector returned by `GetTelemetry`. The pointer is optional, remains caller-owned and is reacquired after callbacks that can replace the collector.

`ParseRunOptions` reads option/value pairs. The common options are `--ups`, `--fps`, `--vsync`, `--telemetry`, `--quit-after` and `--screenshot`. Vsync and telemetry accept `on|off`. Rates are in Hz and the timeout is in seconds. Custom `RunOption` entries take precedence on a name match. A missing or unacceptable value or unknown option prints usage to stderr and returns false. Callers normally exit with status 2. Earlier assignments and callback effects may already have occurred. `ParseNumber` requires a fully consumed non-negative number. `ParseCount` requires a fully consumed integer in the supplied inclusive range.

## Development frame hooks and capture

Development builds add `--quit-after-steps`, `--seed` and `--size` (`<w>x<h`, each dimension 1 through 16384). The step-limited route, used with the ordinary update driver, suppresses physical input, feeds one step of time per loop until the requested count (subject to the fixed-step clamp) and draws with alpha one unless the host overrides it. A count of zero draws the initial state. The player can pass the default seed of 1 to its World setup, where an authored scene seed takes precedence. The App loop itself has no World dependency. `exactFramebufferSize` requests physical pixel dimensions and fails startup if they cannot be obtained. `hidden` controls initial visibility.

When supplied, `advanceFrame` replaces the built-in fixed-update branch and receives raw wall seconds. The development host then owns simulation timing, input consumption and any `renderAlpha` override. `beforeRender` follows advancement even while paused or minimized. `afterRender` runs after the game draw and before the debug overlay. It is called even for an empty framebuffer so deferred work can complete with an error. The final timed or step-limited `screenshotPath` capture occurs later, after the overlay and before presentation, and uses the current read framebuffer.

## Debug UI

F3 toggles rate display. F3+C toggles the command bar while preserving the previous rate panel state. The bar consumes game input while open. Both transitions resynchronize the cursor and cancel held game input, as does focus loss for held controls. A repeat alone cannot start a game key press. Debug controls adjust vsync, frame cap and telemetry collection independently of simulation rate. See [player debug controls](../../player-debug.md).

## Packaging and evidence

The internal object target is compiled with its lower-boundary dependencies only. The public consumer facade reaches the shared engine built by the root CMake target pixie_shared (output pixie.dll on Windows); this does not turn every included boundary into a permitted implementation dependency. Native C++ consumers must match toolchain/runtime settings. Core/Platform/Render/App ownership was established by the split; shared packaging is being qualified under [PIXIE-TODO-051](../../archive/todo-051-shared-engine-export-mcp-2026-11-01.md#pixie-todo-051--shared-engine-game-export-and-local-mcp-development-channel).

The [qualification matrix](../../shared-engine-export-validation.md) records bounded results and remaining gaps. A Debug build/test result does not establish Release, CI, Linux, export, MCP or final owner acceptance.
