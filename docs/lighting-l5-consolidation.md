# 2D Lighting: L5 Consolidation Record

**Document ID:** GLOBAL-LIGHTING-L5-CONSOLIDATION

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** the L5 consolidation of 2D lighting: regression matrix against the acceptance cases, stress report, residual limits, and what remains for the owner's review

**Surface:** guide

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## 1. Status

Gate L5 of [PIXIE-TODO-052](archive/todo-052-2d-lighting-2026-10-31.md), task `PIXIE-TASK-218`, on
2026-10-31, at code `c4f9a58dc1cd397ea64b73fd8276f45748b2e654`. This record covers the parts of L5 that do not need the
owner: the regression matrix, the stress report and the residual limits. Two parts remain open
because they belong to the owner: approving reference images for the rooms (LIT-018, as for
`PIXIE-TASK-212`) and the visual and readability review of the four rooms (LIT-014, consumers
L01–L06). Both were settled on 2026-10-31 (section 5).

The [requirements](lighting-requirements.md) own the cases; the [L0 design record](lighting-l0-design.md)
keeps the pinned choices and the L4 budget tables; the module references own the API.

## 2. Regression matrix

The acceptance cases of the requirements (section 9), each with the evidence that covers it now.
"Passes" means bounded tests or captures that a test-only mutation has shown able to fail; "open"
names what is missing. All `lighting-tests` cases ran on the RX 7600 and on Mesa 26.2.4 llvmpipe.

| Case | Requirements | Evidence | Status |
|---|---|---|---|
| Compatibility | LIT-002 | `NeutralSettingsLeaveTheImageUnchanged` (0 bytes differ at default settings, and a lowered ambient does change the image), `LightsWithoutSettingsDoNothing`; at every stage 11 sample captures were byte-identical to the L0 build (hello-gl differs between two runs of one build, so it is not comparable) | Passes |
| Light types | LIT-003–005 | `PointLightFollowsItsFalloff` (also at zoom 2), `SpotLightKeepsToItsCone`, `DirectionalLightCoversTheView`, `LightLayersIncludeAndExclude`, `LightLayersHaveNames`, `ExposureAndToneMapping` | Passes |
| Shadows | LIT-006–008 | `ClosedRoomDoesNotLeak` and `LowQualityShadowsAtHalfResolution` (0 lit pixels outside touching walls), `SoftShadowsMatchTheReference` (within 0.0051 of an exact area-light reference), `ShadowLayersChooseTheOccluders`, `SelfShadowDarkensOnlyWhenAsked`, `DirectionalShadowsAreParallel`, `OccluderGeometryFollowsColliders`, the sprite-alpha outline cases; flicker rests on the L0 prototype's frame-to-frame measurement of the same technique | Passes bounded; no recorded room sequence with a moving camera |
| Physics-driven occluders | LIT-006 | `OccludersCastFromTheirInterpolatedPose` (an occluder at render alpha 0.5 between two poses shadows exactly as at their midpoint, so shadows follow the drawn pose at any render rate); room 1's pushed box and opted-in character; room 2's rotating platform, spinning bar, falling box and sliding door (`Door2D`), captured after 0, 60 and 120 steps | Passes bounded; 30/60/144 FPS not run on a display |
| Emissive and transitions | LIT-009 | `EmissionFollowsItsIntensity` (fade 1 → 0 recorded per step); room 3's power switch: the logical state and "(lamps fading)" on the panel and in the `machine.powered` and `machine.lamp_intensity` gauges, captured mid-fade and settled | Passes |
| Particles | LIT-010–011 | `EmissiveParticlesGlowUnlit`, the illuminating and non-illuminating pool cases, the pool and global caps (3 of 10, then 2 of 10); room 3's sparks and dust | Passes |
| Composition | LIT-012 | `ScreenCanvasIsNeverLit`, `UnlitReceiversKeepTheirColor`, gizmos and debug drawing over the lit frame; room 3's panel is screen canvas | Bounded cases pass; pan and zoom near bright sources is the owner's review |
| Readability | LIT-014 | Rooms 3 and 4: powered, unpowered (P) and with effects on and off (F); the Low preset | Open: the owner's review |
| Authoring | LIT-015–016 | Every lighting component reflected with units, ranges and tooltips (Inspector edit, save, reopen); the MCP/development-channel qualification's lighting cases (light handles as one undo step, Escape cancel, layer names, debug views over the channel); `LightingDebugDrawsBoundsAndOccluders`, `LightingDebugViewsReplaceTheFrame`; the devchannel `LightingDebugViewsOverTheChannel` case | Passes |
| Budgets | LIT-017 | `LightingRecordsTelemetry`; the stress report in section 3 | Passes, with the declared budget |
| Captures and references | LIT-018 | Fixed-step, seeded captures of every room on the RX 7600 and llvmpipe; `lighting-tests` runs in software | Captures pass; references open for the owner |
| Platform and boundaries | LIT-019 | C API 19 for the additions; the `game-export` and `project-code` tests (no development code in exports); native code has no editor dependency | Passes |

Build matrix at `c4f9a58dc1cd397ea64b73fd8276f45748b2e654`: vs2026 and ci, Debug and Release, built with 0 warnings and passed 27 of 27 CTest tests each (the 23 before, and the generic C API module and project tests of the two room projects); the editor built with 0 warnings; the MCP/development-channel qualification passed 178 cases in Debug and in Release (`--skip-cold`); `lighting-tests` passed 33 tests on the RX 7600 and on llvmpipe; 11 sample captures (12 steps, seed 42, 320 × 180) were byte-identical to the L0 build (hello-gl differs between runs of one build).

## 3. Stress report

`lighting-budget` (`tests/benchmarks/LightingBudget.cpp`, described with the [L4 budgets](lighting-l0-design.md#budgets-measured-at-l4))
rerun on the final code: named rooms drawn through a `ProjectSession` with a screen panel open, one
cold frame and then 240 warm frames (60 on llvmpipe), vs2026 Release. Rooms have 64 boxes (0 lights)
or 128; lights are shadowed points; the burst is 2000 illuminating, emissive particles; "effects" is
ambient occlusion, fog and bloom together. Milliseconds; "wall" is the whole frame with `glFinish`.

RX 7600, 1920 × 1080:

| Room | Wall p50 / p95 / p99 / max | Lighting CPU p50 / p99 | GPU lights p50 | GPU shadows p50 / p99 / max | GPU combine p50 | Cold frame |
|---|---|---|---|---|---|---|
| 0 lights | 0.27 / 0.32 / 0.36 / 0.72 | 0.03 / 0.05 | 0 | 0 / 0 / 0.003 | 0.068 | 10.6 |
| 8 lights, Low (half-size masks) | 0.51 / 0.55 / 0.60 / 0.72 | 0.07 / 0.10 | 0 | 0.156 / 0.158 / 0.166 | 0.068 | 0.7 |
| 8 lights, Medium | 0.56 / 0.59 / 0.61 / 0.65 | 0.07 / 0.10 | 0 | 0.199 / 0.203 / 0.208 | 0.068 | 1.0 |
| 16 lights | 0.82 / 0.86 / 1.19 / 1.74 | 0.07 / 0.09 | 0 | 0.415 / 0.419 / 0.434 | 0.068 | 5.7 |
| 16 lights, effects | 1.06 / 1.10 / 1.17 / 1.19 | 0.10 / 0.12 | 0 | 0.428 / 0.432 / 0.438 | 0.242 | 2.0 |
| 16 lights + 2000-particle burst | 1.30 / 1.37 / 1.40 / 1.48 | 0.21 / 0.26 | 0.329 | 0.422 / 0.425 / 0.425 | 0.068 | 1.6 |
| 64 lights, High | 1.91 / 1.96 / 2.00 / 2.23 | 0.13 / 0.18 | 0 | 1.263 / 1.268 / 1.281 | 0.069 | 3.4 |
| 64 lights, Medium (32 dropped) | 1.15 / 1.21 / 1.25 / 1.27 | 0.09 / 0.14 | 0 | 0.671 / 0.674 / 0.682 | 0.068 | 1.4 |

Mesa 26.2.4 llvmpipe, 1280 × 720:

| Room | Wall p50 / p99 / max | GPU shadows p50 / p99 | GPU combine p50 | Cold frame |
|---|---|---|---|---|
| 0 lights | 3.6 / 4.2 / 4.3 | 0 / 0 | 2.2 | 198 |
| 8 lights, Low | 11.1 / 12.5 / 12.8 | 7.5 / 8.6 | 2.1 | 388 |
| 8 lights, Medium | 12.0 / 13.5 / 13.7 | 8.1 / 9.2 | 2.1 | 398 |
| 16 lights | 19.2 / 21.8 / 27.3 | 15.4 / 20.7 | 2.1 | 408 |
| 16 lights, effects | 28.3 / 31.1 / 35.9 | 18.0 / 23.9 | 8.2 | 655 |
| 16 lights + burst | 47.0 / 52.3 / 55.2 | 17.6 / 21.0 | 2.2 | 493 |
| 64 lights, High | 57.1 / 66.4 / 67.7 | 53.3 / 64.1 | 2.1 | 405 |
| 64 lights, Medium | 32.5 / 35.8 / 43.7 | 28.7 / 39.4 | 2.1 | 403 |

Against the declared budget (RX 7600, 1920 × 1080: up to 64 shadowed lights and 2000 particle
lights within 3 ms of GPU lighting at p99), the worst room, 64 lights at High, takes 1.34 ms of GPU
lighting at p99 (lights, shadows and combine) and 2.00 ms of wall time. The figures match the L4
tables within a few hundredths of a millisecond: counting lights in the light buffer's alpha (the
overdraw view) costs nothing measurable. On llvmpipe the CPU and GPU columns coincide, each room's GPU
shadow maximum has one sample near 200 ms, which we take to be the cold frame's timestamp read late
(p99 leaves it out), and the figures are for
correctness at small sizes, not a budget. Raw CSVs (p50/p95/p99/max of every stage, and the counts)
are machine-local evidence.

## 4. Residual limits

What the delivered lighting does not do, or does only within a bound, as of this record. None of
these blocks a required case above; each is a candidate for later work or for the owner's call.

- **Shadows.** Shadow volumes cost per shadowed light and per occluder edge; the quality caps (8,
  32, 64) drop the weakest shadowed lights over the cap instead of drawing them unshadowed. Self
  shadow applies per convex piece, so the pieces of a compound collider shadow each other. The Low
  preset's half-resolution mask softens shadow edges and lets light within a pixel of a wall. In two
  64-light loads at 1280 × 720 on the GPU, the L0 distance field was 0.15–0.28 ms cheaper.
- **Particles.** Particle lights are gathered on the CPU from pool memory, not drawn from it on the
  GPU, and never cast shadows; there is no aggregate shadowed light per pool.
- **Layers and color.** 8 light layers; tone mapping is `None` or `Aces`; emission intensity is
  0–16, stored in 8 bits.
- **Optional effects.** Ambient occlusion is a 2D approximation around occluder edges that darkens
  only the ambient light; fog is height fog along world y with one color; bloom is one half-size
  threshold and blur; normal maps apply to sprites only (not shapes, text or particles) and were not
  timed. All are off by default and unreviewed.
- **Debug views.** The overdraw view counts the lights that reach each pixel after the range and
  receiver-layer tests, not every fragment rasterized; light bounds draw scene lights, not particle
  lights.
- **Timing.** GPU timings come from timestamp queries read a few frames late; on llvmpipe the CPU
  and GPU figures coincide, and llvmpipe is for correctness, not a budget target.
- **Rooms.** The door and the power switch are sample code, not engine components. Room captures
  are machine-local evidence, not references.
- **Outside lighting.** Setting a Choice field over the development channel to a value that is not
  one of its labels (the number 1 for `Light2D.type`) wrote 255 instead of being refused; the
  qualification now sets it by label. Fixed after this record: the editor refuses a name that is
  not one of the choices, and `pixie_field_set` and `pixie_record_set` refuse an enum value outside
  its names (`choice-outside-its-names-is-refused` in the editor physics suite, and the C API test).

## 5. Owner decisions

1. **Reference images.** On 2026-10-31 the owner accepted ("parece tudo certo") the recommendation
   to defer them to `PIXIE-TASK-212`, which chooses the samples and the tolerance policy. Captures
   of every room (fixed steps, seed 1, 1280 × 720, RX 7600 and llvmpipe) remain machine-local
   evidence; none is a reference.
2. **Visual and readability review.** On 2026-10-31, after reviewing rooms 1 and 2 in the editor
   and rooms 3 and 4 in the player, the owner stated "estao aceitas". This is recorded testimony;
   no owner screenshot is attached.
3. **The L0 choices** stay as pinned, with their review waived, not performed
   ([section 9 of the L0 record](lighting-l0-design.md#9-choices-open-to-the-owners-review)); each
   remains revisable.
