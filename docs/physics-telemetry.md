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
| `physics.solve` | Native backend step, including backend sensor processing; not only awake-body constraint solving |
| `physics.scene.coupling.prepare` | D4 motor input/timers/intent, support queries, prediction and bounded body commands; recorded when motors exist |
| `physics.scene.motors` | D4 final support carry, motor motion, placement diagnostics and motor pose write-back; before D4 this included commands/input and integration |
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
(`on`/`off`), warm sample count (100..100000), and CSV path. An optional final
argument selects `sparse` (the compatible default), `compound`, `stack`, `sensors`
or `coupled`. The default fixture is a
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

## Extended native workloads

The optional fixtures exercise authored topology, native pair lifetimes and, in
`coupled`, scene characters; they add no runtime API or rendering. Scale (32 units/m),
substeps (four), seed, tick, cold and warm intervals are explicit. The population
argument always counts dynamic bodies; supporting static bodies are additional.

| Workload | Geometry and motion | Expected backend population |
|---|---|---|
| `sparse` | Original 32x32 boxes at 128-unit grid spacing; initial X speed 10 or zero, no gravity | N bodies, N shapes |
| `compound` | Two adjacent 16x32 half-boxes make each 32x32 body; same sparse motion | N bodies, 2N shapes |
| `stack` | Up to ten columns, 32.25-unit spacing with seeded X jitter; gravity 1100 down; a floor with top Y=0 | N+1 bodies/shapes, no sensors |
| `sensors` | One static 32x32 sensor per dynamic box on the sparse grid; two-sided filters exclude solid body/body contacts | 2N bodies/shapes, N sensors |
| `coupled` | Ten lanes 400 units apart, gravity 1100: a static floor, one 120x16 kinematic platform with a rider, a pusher and a stander per lane; every third lane has a ceiling over its platform. Dynamic boxes go round the lanes: the first in each lane is a 10 kg crate under the stander, the rest a row in front of the pusher cycling 1 kg, 10 kg and two-half compound boxes | 23+N bodies (13 static, 10 kinematic), plus one shape per compound box; 30 motors |

For `stack`, `awake` adds an in-tick zero-force wake request to each body;
`sleeping` leaves the initially still pile unforced. These labels are requests,
not guarantees that the pile stays fully awake or has settled after 300 ticks.
The CSV records actual end-of-tick awake counts; bodies may wake and sleep inside
the solve, so that count is not an active-solver-work counter.

For active `sensors`, bodies start 40 units left of their sensor, move at
80 units/s and reverse every 60 ticks. The periodic crossing creates real sensor
begin/end churn without solid contacts. The still variant starts centered and
can sleep while its sensor pairs remain present. All command-driver work is
inside the measured World tick; `benchmark.commands` measures it separately.
Comparing these variants includes command overhead and changed physics work.
It is not a pure sleeping/awake solver comparison.

In awake `coupled`, the platforms reverse every 120 ticks: lanes cycle a 60 units/s
translation, a 40 units/s rise into the ceiling (crush, then recovery) and a 0.3 rad/s
rotation. Riders jump every 150 ticks, pushers walk into their rows and back every 240
ticks, and every 480 ticks, when the platforms are back at their starts, the riders are
teleported onto them again so riding and crushing recur. The still variant leaves the
platforms and motors idle. Motors always run the coupled prepare/finish path, so the
`coupled` population of zero bodies measures the 30 motors on their own.

```powershell
bin/Release/physics-benchmark.exe 500 awake on 1200 stack.csv stack
bin/Release/physics-benchmark.exe 500 awake on 1200 sensors.csv sensors
bin/Release/physics-benchmark.exe 500 awake on 1200 coupled.csv coupled
```

CSV schema 2 adds the fixture/driver, gravity/scale, sensor population and the
four begin/end event counts. Schema 3 adds the `physics.scene.coupling.prepare`
stage and, per tick, the motors, those with a support, those on a kinematic
support, and those reporting blocked or unsafe placement. Blocked and unsafe
placement are scene diagnostics by design; the run fails on any other diagnostic.
Raw population/transition snapshots are read after
the timed step; final retained poses/velocities are checked for finiteness
outside measurement. That audit does not establish penetration limits or stack
stability. Runtime regressions check stable topology/handles, actual contact and
sensor generation, finite poses and enabled/disabled collection trace agreement.
Dense contact and sensor churn may allocate even when no geometry is rebuilt.

Ordinary C++ new/new[] calls intercepted by the benchmark executable are
counted inside the tick. Backend malloc and over-aligned allocations are
outside this counter's coverage. All variants use the same allocation counter,
so its cost is part of the measured fixture. Zero counted calls does not prove
zero total heap activity.

**Shared-engine scope change, 2026-10-28.** The counter replaces global new
only in the benchmark executable. With Runtime now in pixie.dll, it does not
intercept Engine/backend allocation sites in that DLL; new CSV runs describe
host new/new[] activity only, even though the CSV shape is retained. Do not
compare those counts with the pre-DLL qualification as evidence of unchanged
Engine allocations. The earlier allocation/sensitivity results remain valid
for their statically linked baseline, not the new distribution. CPU timing and
population telemetry still come from the native collector; their instrumentation
is distinct from this allocation counter. Requalification of Engine allocation
sites would require an explicitly scoped instrument in the DLL. No such new
allocator infrastructure or closure of physics acceptance is claimed here.

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
The extended workload qualification is recorded separately under
`.local/validation/physics-workloads-2026-10-26/`. Any other active compiler or
application can affect wall-clock tails on the same machine; process sampling
is context, not proof of uninterrupted idle or causal attribution of a spike.
The 2026-10-30 shared-engine run of all five workloads, including the mixed
`coupled` workload, is summarized in the
[physics qualification record](physics-qualification.md#workloads-and-stress),
with its raw evidence under `.local/validation/physics-completion-2026-10-30/`.
Its allocation column covers only the host executable. Renderer behavior and
owner acceptance remain separate gates under TODO-043.
