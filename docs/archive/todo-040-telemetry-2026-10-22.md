# Generic CPU Telemetry Completed: PIXIE-TODO-040

**Document ID:** ARCHIVE-TODO-040-TELEMETRY-20261022

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** generic collector, World instrumentation, native display and active LogicSandbox integration

**Surface:** archive-record

**Boundary:** global

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-077

**Type:** completion

**Origin:** PIXIE-TODO-040

**Author:** codex

**Reference date:** 2026-10-22

**Commits:** `9d022c1639383f3397cfe7ac52057db1b20860be`, and this record in `a18fe6ba9ecadb94b148f6cce7d23019bd277c70`

**Current state:** [telemetry guide](../telemetry.md), [public API](../../engine/core/include/pixie/core/Telemetry.hpp)

## Accepted entry

**Status:** completed

**Source:** [owner continuation](telemetry-promotion-2026-10-22.md)

**Accepted scope:** main-thread CPU timings, gauges/counters, optional collection, snapshots/export,
named World systems, native debug display and active LogicSandbox integration. Circuit semantics,
GPU timing and other extraction/game infrastructure candidates are outside scope.

**Dependencies and gates:** preserve existing player debug work; no additional approval gate.

**Completion criteria:** aggregation/lifetime/disable/reset tests, neutral/native evidence, overhead
measurements, engine Debug/Release and active game Release regressions; documented API and limits.

## Task snapshot

**Parent:** PIXIE-TODO-040

- `PIXIE-TASK-168` — **Status:** done — Collector-owned metrics, scoped timings, export and headless tests.
- `PIXIE-TASK-169` — **Status:** done — World systems and native collection/display, preserving input and simulation contracts.
- `PIXIE-TASK-170` — **Status:** done — Active LogicSandbox consumer, propagation regressions and save preservation.
- `PIXIE-TASK-171` — **Status:** done — Engine configurations, neutral/native use, overhead, failure detection, contract and evidence.

## Result

`Telemetry` owns stable metric registrations; each World owns an instance. CPU timing scopes are
inclusive, noncopyable and main-thread-only, with arithmetic means and disable/reset boundaries.
Gauges, exact uint64 counters, owned snapshots and quoted CSV work without rendering. Sampling uses
registered handles and allocates no storage. Disabled scopes do not read a clock.

World instruments steps, aggregate Behaviours and named systems. App can expose an optional collector;
ProjectApp/session return null before Start/after Stop and expose the current World otherwise. Native
F3 shows rates/metrics; F3+C accepts `-telemetry true|false`, and players accept `--telemetry on|off`.
Visibility is separate from collection. The C++ API is additive and requires rebuilding consumers;
the C API surface/version is unchanged. Existing VSync/frame-cap/input behavior remains covered.

The actively developed LogicSandbox consumes this engine infrastructure, replacing its local timing
and moving circuit detail to F4. Circuit population sampling stays game-owned and runs independently
of panel visibility, including headless updates. No circuit evaluation or propagation rule changed.
The older game checkout was preserved; no commit, push, publication or independent acceptance occurred.

## Verification

Windows build 26200, Ryzen 7 5700X, approximately 16 GB RAM, Radeon RX 7600; MSVC x64 with static CRT
and warnings as errors. Debug and Release engine builds passed. Final CTest runs passed all **11
scenarios in each configuration**, including **31 engine tests and 94 runtime tests**. The C# editor
built Release with zero warnings/errors. The active game built its Release targets and passed its
full **151-test suite**; the final linked adapter also passed four focused telemetry tests.

Deterministic clock tests cover disabled clock reads, inclusive scopes and reset/transition boundaries;
other checks cover independent Worlds, stable handles/references, overflow, units and export. A copied
collector with a deliberately wrong mean failed two assertions, verifying check sensitivity without
mutating product code. The neutral sample exported 600 headless steps and zero samples when disabled;
its native mode ran 120 updates. A native calculator capture explicitly verified 65,944 pieces and
98,018 wires and exported 19 metrics. All 15 original player save files retained their SHA-256 hashes.

An initial game run exposed an existing expansion-test save leak into later template fixtures. Its
save directory was isolated and cleaned up; the full rerun passed. A concurrent final Debug/Release
CTest repetition collided in project-code's shared temporary directory. Debug finished successfully,
then a serial full Release repetition passed. These tooling failures and final logs were retained;
configuration tests must run serially while that fixture shares its WORK directory.

Three alternating runs of 1,000,000 scopes each, after registration, measured median **2.6450 ns per
disabled scope** and **49.7847 ns per enabled scope**, with zero timed-loop allocations. No active
build/test overlapped this measurement. This includes checksum/loop work and is a local CPU
microbenchmark, not an application FPS gain. The calculator image is visual/integration evidence,
captured with a 60 FPS cap while other checks ran; loading contributes to its session-wide CPU peaks.

Source/binary hashes, hardware, CSV, screenshot, initial/final logs and the detailed author report are
retained as machine-local validation artifacts. The explicit VAW routes/profile are recorded in the
[promotion](telemetry-promotion-2026-10-22.md). Documentation and whitespace checks passed.

## Limits and follow-ups

No GPU timings, multithreaded writers, remote transport, percentile histograms or editor telemetry UI.
Registrations retain storage until collector destruction; nested timings overlap. Native rows truncate
to the viewport and sampled gauges can lag. Lighting, movement/collision, audio, caches, visibility,
queues and prefabs require separately bounded work.
