# Physics Telemetry and Bounded Qualification

**Document ID:** RUNTIME-PHYSICS-TELEMETRY-GUIDE

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** native physics timing boundaries, retained synchronization buffers and headless qualification

**Surface:** guide

**Boundary:** runtime

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## Measurement boundaries

Physics uses the World's existing [CPU collector](telemetry.md). Names are
registered when its scene movement context is created, outside repeated sync
loops. Collection remains disabled by default; resetting or toggling collection
does not reconstruct physics or change simulation time. The C API stays at 16.

| Timing | Included work |
|---|---|
| `physics.scene.sync` | One native scene synchronization; normally two calls per World tick, before gameplay and before solving |
| `physics.scene.rebuild` | Backend world creation/reset, body creation/replacement, and new controller creation; nested inside sync |
| `physics.solve` | Existing native backend solve |
| `physics.scene.motors` | Existing motor commands/input, controller integration and motor pose write-back; this is not D4 body coupling |
| `physics.events.collect` | Existing backend contact/sensor event collection |
| `physics.scene.events.publish` | Copying native events to scene identities |
| `physics.scene.publish` | Retiring old shape identities and publishing rigid-body poses |

Durations are inclusive. Do not sum sync with its rebuild child, or these stages
with `world.step`/named-system parents. A zero recorded duration may be below
clock resolution. Edit sync calls are measured too when collection is enabled.

| Scene gauge | Meaning |
|---|---|
| `physics.scene.rebuilds` | Successfully created/replaced bodies and newly created controllers in the last tick; outside a tick, in the last explicit sync |
| `physics.scene.sync_calls` | Sync calls in that tick or explicit sync boundary |
| `physics.scene.bodies_published` | Rigid/compound scene body transforms published in the tick; standalone static colliders do not need pose write-back |
| `physics.scene.motor_count` | Current synchronized controller count |
| `physics.scene.pending_commands` | Zero for the current immediate engine API, which has no engine-side deferred command queue |

The pending-command gauge does not mean that the backend has no accumulated
force awaiting a solve. Existing `physics.bodies`, `physics.shapes`,
`physics.awake_bodies`, contact/sensor pairs and event counts remain available.
Disabled collection retains prior values according to the collector contract.

Synchronization reuses sorted active-ID buffers instead of allocating tree nodes
for temporary sets on every call. It still scans scene components and publishes
all synchronized body poses; sleeping bodies reduce backend solve work, not
every scene scan. Body geometry is rebuilt only after relevant authored changes.
Contact transitions, invalid configuration, geometry changes and debug snapshots
can still allocate. This is not a claim of universally allocation-free physics.

## Reproducible headless fixture

Build `physics-benchmark` from the normal test configuration, then run:

```powershell
bin/Release/physics-benchmark.exe 500 awake on 1200 output.csv
bin/Release/physics-benchmark.exe 500 sleeping off 1200 output-off.csv
```

Arguments are population (0..500), state (`awake`/`sleeping`), collection
(`on`/`off`), warm sample count (100..100000), and CSV path. The fixture is a
native World with seed 12345, zero gravity, scale 32 units/m, a 1/60-second tick
and four solver substeps. Unit-mass 32x32 boxes use a sparse 128-unit grid with
seeded X jitter. Awake boxes move at 10 units/s with no damping; sleeping boxes
start still. The fixture has no contacts, sensors, motors, window or rendering.

The first 60 ticks are labeled cold, including backend initialization. Another
300 ticks warm the scene, then the requested warm ticks are recorded. External
wall time brackets World::Step; stage measurements use timing-total differences
so both sync calls are included. Samples and output storage are prepared outside
the measured work; CSV output happens after sampling. A companion `.metrics.csv`
contains the final collector snapshot.

Ordinary C++ new/new[] calls are counted inside the tick. Backend malloc and
over-aligned allocations are outside this counter's coverage. All variants use
the same allocation counter, so its cost is part of the measured fixture.
Zero counted calls does not prove zero total heap activity.

For a qualification report, run 0/100/500 populations in both states with
collection on/off, interleaving at least three serial replicates. Preserve raw
rows, configuration/compiler, named machine/CPU/OS/RAM, seed, sample counts,
warm-up, render settings, p50/p95/p99 and worst tick. Keep cold and warm separate.
Nearest-rank percentiles and median per-run medians are suitable descriptive
statistics; retain tails and individual replicates rather than hiding spikes.

Collection on versus off estimates enabled collection cost. It does not measure
the cost of compiled-but-disabled instrumentation by itself. A reference build
with only the new scene probes removed, retaining the same optimized algorithm
and existing disabled probes, can estimate that additional disabled cost.
Preserve its exact source diff, binary hash, build logs and production-source
restoration evidence. Signed small deltas may reflect system/code-layout noise.

## Acceptance limits

The local 2026-10-26 qualification is recorded in
`.local/validation/physics-telemetry-2026-10-26/REPORT.md`, with raw/summary CSV,
machine metadata, reference diff and allocation-regression sensitivity evidence.
Those files are machine-local executed evidence, not a portable performance
guarantee. The sparse fixture establishes neither dense contact/sensor churn,
coupled-character performance, render/GPU latency nor a universal body budget.
Those workloads and owner acceptance remain separate gates under TODO-043.
