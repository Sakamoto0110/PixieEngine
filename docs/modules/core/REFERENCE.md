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

Core owns CPU-only timing, logging, telemetry and images. Its native source depends on PixieLib primitives and math and privately uses stb_image for PNG decoding. Platform, Render, App, GLFW, GLEW and the simulation World are outside this implementation boundary. Render owns GPU upload and readback.

Headers use `pixie/core/` and namespace `pixie`. Out-of-line symbols use `PIXIE_CORE_API`. Timing value types and the fixed-step driver are header-only.

## Time and framebuffer dimensions

`FramebufferSize` is a width and height in physical pixels, both zero by default. The producer supplies a drawable size. A zero extent may describe a minimized or otherwise unavailable framebuffer.

`FrameTime` is passed explicitly. In a fixed update, `delta` is the step in seconds, `elapsed` is the simulation time before that step, `index` counts updates from zero and `alpha` is zero. For rendering, `delta` is the supplied frame interval in seconds, `elapsed` is the completed simulation time, `index` comes from the frame driver and `alpha` is the fraction used to interpolate previous and current state. An alpha of one selects current state.

`FixedStepper` takes a finite positive rate in updates per second, default 60 Hz. `Advance` clamps each finite wall-time contribution to [0, 0.25] seconds and invokes the callback synchronously for each step due, including zero calls if the accumulator is still short. `RenderTime` reports the remainder divided by the step and passes through the supplied delta and index. `Drop` clears the remainder while preserving `Steps()` and `Simulated()`. A host normally polls input, calls `Advance`, clears input transients after each update, then uses `RenderTime` when it draws. See [the player loop](../app/REFERENCE.md#player-loop-and-teardown).

`RateMeter` counts updates and drawn frames separately. Supply monotonic `steady_clock` times. `Sample` closes the current window once at least one second has passed, divides by the actual duration and resets the interval counters. The getters report the last completed interval, or zero before the first sample.

## CPU telemetry and logging

A `Telemetry` collector belongs to its owner thread. It starts disabled and takes a non-null clock, defaulting to `steady_clock::now`. Register a stable non-empty name once per metric. A repeat with the same kind and unit returns the same handle, and conflicting registrations throw `std::invalid_argument`. Timing is measured in milliseconds, gauges hold finite values in caller-chosen units and counters use exact unsigned 64-bit increments. Disabled, invalid, nonfinite or overflowing writes are rejected as applicable.

`MetricHandle` belongs to its collector. A handle and the borrowed result of `Timing` survive later registration and reset while the collector lives. `Timing` throws for a foreign or wrong-kind handle. `SetEnabled` preserves existing measurements and `Reset` clears values without unregistering names. Both an enable-state transition and a reset invalidate active scopes. `TelemetryScope` measures inclusive CPU wall time, so nested durations overlap. Disabled or invalid scopes read no clock. `Snapshot` copies the metrics in registration order and `WriteCsv` writes that order to the caller's stream. Both remain available when collection is off.

`Log` and its severity helpers emit tagged lines to standard error. They are the provisional native logging surface.

## CPU image data

`Image` owns top-row-first RGBA8 bytes. `DecodePng` and `LoadPngFile` return premultiplied RGB for sprite filtering. They return an empty optional and a diagnostic on decode or file-open failure. `CheckerboardImage` builds an opaque magenta/dark fallback with the requested cell size in pixels. Supply positive dimensions and cell size.

`EncodePng` expects positive dimensions and a span containing a complete, tightly packed, top-row-first image with straight alpha. It writes stored (uncompressed) deflate blocks, so the file is roughly the size of the pixels. `WriteFileBytes` returns false on open or write failure. For GL readback and texture orientation, see [Render targets and readback](../render/REFERENCE.md#targets-and-readback).

## Packaging and evidence

The internal object target is compiled with its lower-boundary dependencies only. The public consumer facade reaches the shared engine built by the root CMake target pixie_shared (output pixie.dll on Windows); this does not turn every included boundary into a permitted implementation dependency. Native C++ consumers must match toolchain/runtime settings. Core/Platform/Render/App ownership was established by the split; shared packaging is being qualified under [PIXIE-TODO-051](../../archive/todo-051-shared-engine-export-mcp-2026-11-01.md#pixie-todo-051--shared-engine-game-export-and-local-mcp-development-channel).

The [qualification matrix](../../shared-engine-export-validation.md) records bounded results and remaining gaps. A Debug build/test result does not establish Release, CI, Linux, export, MCP or final owner acceptance.
