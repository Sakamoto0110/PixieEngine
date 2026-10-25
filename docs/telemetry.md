# CPU Telemetry

**Document ID:** ENGINE-TELEMETRY-GUIDE

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** collector ownership, CPU timing, gauges/counters, headless export and native display

**Surface:** guide

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

The public contract is [`pixie/Telemetry.hpp`](../engine/include/pixie/Telemetry.hpp). A collector
is owned by its caller, is not copyable/movable and is used on one thread. It has no dependency on
World, rendering or any game. Each World owns a collector exposed by `GetTelemetry()`.

## Registration and sampling

Register timing, gauge and counter names outside hot loops. Registrations persist until collector
destruction. Reusing a name with the same kind/unit returns the same handle; a mismatch or empty name
throws `invalid_argument`. Handles and timing references stay valid through registration and Reset.
They must not outlive their collector. Foreign handles and wrong-kind sampling are rejected.

Collectors start disabled. `SetEnabled` controls collection independently of any UI. Disabled scopes
do not read a clock. Enabled `TelemetryScope` measures inclusive CPU wall time; it performs no metric
registration or allocation. Nested scopes overlap and their totals must not be summed as independent
costs. A scope spanning Reset or any collection transition is discarded. An injected monotonic clock
supports deterministic tests. Clock functions must not throw.

Timing stats are last, arithmetic mean, peak, total milliseconds and exact sample count. Gauges are
finite doubles and may be negative. Counters are exact uint64 values; overflow is rejected rather
than wrapping. Invalid/nonfinite/negative durations are rejected. Disabling retains the last values;
Reset zeroes values while preserving registrations and the collection setting.

`Snapshot()` copies owned data in registration order. `WriteCsv(stream)` exports quoted names/units,
timing stats and exact counter values using a locale-independent decimal point. Both work without a
window and while disabled. Export/snapshot work may allocate and is outside the hot sample path.
Repeated snapshots/exports do not modify measurements or simulation state.

## World and native player

World records `world.step`, `world.behaviours`, and `system.pre.<name>` / `system.post.<name>`.
The step includes all its children, including deferred work; named-system records aggregate duplicate
names in the same phase. This instrumentation does not change system order, fixed time or dirty work.
It is disabled until the consumer enables the World's collector.

An App may override `GetTelemetry()` to expose a non-owning collector to the native player. Run
reacquires it after App callbacks; ProjectApp exposes its current World's collector, returning null
before session Start and after Stop. This is an
additive C++ source API: consumers must rebuild. No C API telemetry endpoint or C ABI change is added.

F3 shows rates and CPU metrics on the existing debug surface. Snapshot presentation refreshes at
2 Hz and truncates rows to fit; all records remain available through the API/CSV. Collection continues
while the display is hidden. The overlay reports host/app collection status, retaining values when
off. `host.update`, `host.render` and `host.observer` measure App update submission, render submission
and debug observation/submission respectively. They are CPU durations, not GPU timings or frame
latency, and exclude buffer swapping/frame limiting. Renderer synchronization is not introduced.

`--telemetry on|off` explicitly overrides host and exposed-app collection. With no override the host
collector starts off and the app chooses its own policy. F3+C accepts `-telemetry true|false` to apply
the same override during a run without changing display, VSync, frame cap or update rate. The current
override applies to a newly exposed collector as well, including a replacement World. Existing
VSync/frame-cap commands and input capture remain in effect; see [player debug](player-debug.md).

The [telemetry sample](../samples/telemetry/README.md) is a neutral headless/native consumer.
LogicSandbox's per-World collector carries its own timings and circuit counters. Circuit population
sampling remains game code. Neither hiding nor enabling telemetry suspends circuit simulation.

GPU timing, remote collection, automatic percentile histograms, multithreaded writers and C# editor
telemetry presentation are outside this promotion. Author verification and limitations are in the
[PIXIE-TODO-040 completion record](archive/todo-040-telemetry-2026-10-22.md).
