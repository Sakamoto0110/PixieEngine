# 2D Lighting Completed: PIXIE-TODO-052

**Document ID:** ARCHIVE-TODO-052-2D-LIGHTING-20261031

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** 2D lights, shadow volumes, emission, particle light, authoring, debug views, budgets and optional effects, accepted by the owner

**Surface:** archive-record

**Boundary:** global

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-088

**Type:** completion

**Origin:** PIXIE-TODO-052 (PIXIE-TASK-213 through PIXIE-TASK-218)

**Author:** claude

**Reference date:** 2026-10-31

**Commits:** code `b12bb4c`, `d7147f1`, `dd912de`, `f4f321a`, `ea4dc48`, `122359b`, `cd5f61e`, `003893b`, `304adea`, `d12d69c`, `c4f9a58dc1cd397ea64b73fd8276f45748b2e654`

**Reference commit:** `0ec8811`

**Current state:** [lighting requirements](../lighting-requirements.md), [L0 design record](../lighting-l0-design.md), [L5 consolidation record](../lighting-l5-consolidation.md), [light lab](../../samples/light-lab/README.md), [light machine](../../samples/light-machine/README.md)

## Accepted scope and outcome

**Source:** owner promotion on 2026-10-31, after completing PIXIE-TODO-043, with the scope discussed that day: ambient, point, spot and directional lights; occluders with shadows cast by platforms and other geometry; emissive surfaces; particle pools that may glow and light their surroundings; optional normal maps and ambient occlusion.

**Accepted scope:** [LIT-001 through LIT-019](../lighting-requirements.md), delivered through L0–L5. Optional effects (normal maps, ambient occlusion, bloom, fog) stay behind quality settings and must justify themselves in the rooms. Character surface materials, joints, global illumination and per-particle shadows are not in scope.

**Dependencies and gates:** L0 pins the data model, units, composition order and shadow technique (by measured prototypes, including on Mesa llvmpipe) and gets the owner's review before lighting code beyond those prototypes; owner decision 2026-10-31: that review was waived ("Yes, continue to L1"), not performed, so the [pinned choices](../lighting-l0-design.md) stay revisable. Scenes without lighting render pixel-identically. Native engine code never depends on the editor; C API additions raise its version. Reference images need the owner's approval with a stated reason. Physics behavior and its qualification must not regress.

**Completion criteria:** every LIT requirement is linked to its mechanism and evidence; the acceptance matrix passes in vs2026 and ci, Debug and Release; the four rooms have captures, editor/player parity and the owner's visual and readability review; budgets are recorded for named machines and rooms; unmet requirements remain explicit.

L0 pinned the design by measured prototypes: geometric shadow volumes with analytic penumbra won
on the RX 7600 and on llvmpipe (no leak, the lowest error and flicker, the lowest software cost).
L1 delivered scene-level opt-in, ambient, point, spot and directional lights, light layers, unlit
receivers, linear accumulation and the Aces curve, with neutral settings byte-identical to the
unlit renderer. L2 added `Occluder2D` from colliders, polygons, sprite alpha and the character's
capsule, with soft shadows at interpolated poses. L3 added `Emission2D` and `ParticleLight2D`. L4
added telemetry with CPU and GPU timing, quality caps, debug views (also over the development
channel), Inspector authoring with layer names and handles (C API 19), the Low preset's
half-resolution mask, and ambient occlusion, fog, bloom and normal maps, all off by default. The
four rooms live in `samples/light-lab` (1 and 2) and `samples/light-machine` (3 and 4). Deviations
from L0 (R32F occlusion mask, particle lights gathered on the CPU, 8-bit emission intensity,
per-piece self shadow) are recorded in the L0 record.

## Owner decisions and acceptance

- L0 review: waived on 2026-10-31 ("Yes, continue to L1"), not performed; the pinned choices stay
  revisable.
- Rooms: on 2026-10-31, after reviewing rooms 1 and 2 in the editor and rooms 3 and 4 in the
  player, the owner stated "estao aceitas". This is recorded testimony; no owner screenshot is
  attached. Agent-made captures are not the owner's evidence.
- Reference images: deferred to `PIXIE-TASK-212` on 2026-10-31, when the owner answered "parece
  tudo certo" to that recommendation.

## Task snapshot

**Parent:** PIXIE-TODO-052

- `PIXIE-TASK-213` — **Status:** done — L0: pin the lighting data model, units, composition order, compatibility rule and shadow technique by measured prototypes on the target GPU and Mesa llvmpipe; owner review of the pinned choices before further lighting code. On 2026-10-31 the [L0 design record](../lighting-l0-design.md) pinned them; the `lighting-prototype` tool (code b12bb4ccae0b0365f392584a3ba71bccaa7b4245) compared shadow volumes, polar maps and a distance field against an exact reference on the RX 7600 and on Mesa 26.2.4 llvmpipe, and shadow volumes with analytic penumbra won on accuracy, leaks and flicker, and on cost everywhere except two 1280x720, 64-light GPU loads where the distance field was 0.15–0.28 ms cheaper. The owner waived the review ("Yes, continue to L1"); it was not performed, and the choices stay revisable.
- `PIXIE-TASK-214` — **Status:** done — L1: lighting settings, ambient, point, spot and directional lights, light layers, unlit screen UI, linear accumulation, exposure and tone mapping; pixel-identical captures of scenes without lighting. Code d7147f1bf95df06d9f598acb011be988001e47a4 on 2026-10-31: LightingSettings2D, Light2D and LightReceiver2D; the render module's LightRenderer with receiver passes in SpriteRenderer and ShapeRenderer; the lit path in ProjectSession::Render only with settings. `lighting-tests` (9 tests: byte-identical neutral settings, inert lights without settings, falloff also at zoom 2, cone, directional, layers, unlit receivers, exposure and Aces, unlit screen canvas) passed on the RX 7600 and on Mesa 26.2.4 llvmpipe; test-only mutations dropping the layer test and the falloff exponent failed 2 tests (11 checks) and the exact restoration passed. 11 sample captures (12 steps, seed 42, 320x180) were byte-identical between the L0 and L1 Release builds; hello-gl differs between two runs of the same build, so it is not comparable. vs2026 and ci Debug/Release built with 0 warnings and passed 23/23 tests each; the editor built with 0 warnings. The L0 state (e16732b) also passed ci Debug/Release 22/22. Light-layer names, Inspector tools and paths/particles as receivers are later stages; no visual review was made (by instruction, visual results are assumed acceptable and remain open to the owner's review).
- `PIXIE-TASK-215` — **Status:** done — L2: occluders and soft shadows from colliders, polygons or sprite alpha, including physics-driven occluders and opt-in character shadows; leak, flicker and render-rate cases; rooms 1 and 2. Code dd912de2821cac3fdded04a5bee3a83514326196 and f4f321a4163849f0eafba13b2d69c8007eac12bf on 2026-10-31: Occluder2D (Collider, Polygon, SpriteAlpha sources, layers, selfShadow), Light2D castsShadows/shadowLayers/sourceRadius/sourceAngle, shadow volumes with analytic penumbra per shadowed light in an R32F mask, and `samples/light-lab` (rooms 1 and 2 side by side). `lighting-tests` (18 tests) passed on the RX 7600 and on Mesa 26.2.4 llvmpipe: a closed room of touching 2-unit walls with 0 lit pixels outside, soft shadows within 0.0051 (GPU) and 0.0046 (llvmpipe) of the exact area-light reference over 135 penumbra samples, an occluder at alpha 0.5 between two poses identical to the occluder at its midpoint (the render-rate case: shadows follow the interpolated pose at any frame rate), shadow layers and castsShadows, self shadow, parallel directional shadows, collider/capsule geometry and sprite-alpha outlines. Test-only mutations (dropping the shadow-layer test and the shadow-side test; ignoring sprite mirroring) failed 2 and 1 tests and the restorations, byte-identical by hash, passed. The light-lab capture after 120 steps at 1280x720 differs between the GPU and llvmpipe by at most 5/255 in 3 of 3,686,400 channels. Sample captures stayed byte-identical to the L0 build (hello-gl is not deterministic). vs2026 and ci Debug/Release passed 23/23 with 0 warnings; the editor built with 0 warnings. Flicker under motion rests on the L0 prototype's measurement of the same technique; no door is in room 2, since a moving door needs game code. Self shadow applies per convex piece, so compound pieces shadow each other. No visual review was made: by instruction, the look of the rooms is assumed acceptable and remains open to the owner's review.
- `PIXIE-TASK-216` — **Status:** done — L3: emissive sprites, shapes and text with transitions; emissive and light-casting particle pools with caps; room 3. Code ea4dc48676bfa20aad4b92a27158fd07463adb39 on 2026-10-31: Emission2D (tint, intensity 0-16 in 8 bits), ParticleLight2D (emissive, illuminates, radius, intensity, per-pool cap) and LightingSettings2D.maxParticleLights; an RGBA16F emission target and separate emission programs in the sprite, shape and particle renderers; `samples/light-machine` (room 3). `lighting-tests` (23 tests) passed on the RX 7600 and on Mesa 26.2.4 llvmpipe: emission at its exact color in darkness, a powered-down fade 1 → 0.75 → 0.5 → 0.25 → 0 giving pixels 100, 87, 72, 50, 0 with the intensity recorded at each step, intensity 2 brightening, a glow covered by a sprite drawn over it, an emissive pool at its own color and the same pool lit (dark), particle light following the point-light falloff and a pool and global cap (3 of 10, then 2 of 10). Test-only mutations dropping the emission from the combine and the particle splats failed 5 tests (15 checks) and the hash-identical restorations passed. The room 3 capture after 90 steps differs between the GPU and llvmpipe in 1,908 of 3,686,400 channels (at most 73/255), along the thin antialiased spark streaks; 11 sample captures stayed byte-identical to the L0 build. vs2026 and ci Debug/Release passed 23/23 with 0 warnings; the editor built with 0 warnings. Particle splats are gathered on the CPU, not drawn from pool memory on the GPU; powering the machine on and off needs game code, so room 3 shows fixed states and the transition is covered by the fade case. No visual review was made: by instruction, the look is assumed acceptable and remains open to the owner's review.
- `PIXIE-TASK-217` — **Status:** done — L4: Inspector authoring, gizmos, debug views, telemetry with CPU and GPU timing, budgets and quality presets, optional normal maps, ambient occlusion, bloom and fog; room 4. Done on 2026-10-31 in code 122359b4515d1373c8cc96988fdff05b212e04d3: every lighting component is reflected with units, ranges and tooltips, so the Inspector edits them and scenes save them; non-blocking GPU timestamps and `lighting.*` CPU timing and gauges in the World's telemetry; quality caps of 8/32/64 shadowed lights that drop the weakest; debug views of light bounds (shown in Edit) and occluder edges; `lighting-budget`, measuring named rooms with a screen panel on the RX 7600 at 1920x1080 and on llvmpipe at 1280x720 (the budget table in the [L0 design record](../lighting-l0-design.md#budgets-measured-at-l4)). `lighting-tests` (26) passed on the RX 7600 and llvmpipe; a test-only mutation reversing the cap's order failed its test and the hash-identical restoration passed; vs2026 and ci Debug/Release 23/23, editor 0 warnings, 11 sample captures unchanged. Then code cd5f61e2d5ac97fb536c44591e70799f70b2d9b4: LightLayers2D names with named Inspector checkboxes (C API 19: `pixie_light_layer_name`, `pixie_lighting_debug_get`/`_set`), editor handles for a light's range, direction and cone as one undo step each (Escape restores), Session > Lighting debug and the channel's `world.lighting_debug`, and the Low preset's half-resolution masks (8 shadowed lights at 1920x1080: 0.156 ms GPU shadows against 0.200 at Medium; a Low closed room still lets no light out). `lighting-tests` (28) passed on the RX 7600 and llvmpipe; the MCP/development-channel qualification passed 178 cases in Debug and in Release (`--skip-cold`), including the new light handle, undo, cancel, layer name and debug view cases; vs2026 and ci Debug/Release 23/23 with 0 warnings. Then code 003893be49417d969087b74cbe1324a1ed19ce6f: the optional effects behind LightingSettings2D fields, off by default (ambient occlusion, fog, bloom, and normal maps through NormalMap2D with a sprite-only normal pass); each has a lighting-tests case (32 tests on the RX 7600 and llvmpipe), test-only mutations disabling the four effects failed their four cases and the hash-identical restoration passed, and at 1920x1080 with 16 lights ambient occlusion, fog and bloom together cost 0.014 ms in the shadow stage and 0.172 ms in the combine (budget table in the design record); vs2026 and ci Debug/Release 23/23, 0 warnings. Whether each effect improves the rooms is the owner's review (L06). Then code 304adea and c4f9a58 (room code) and d12d69c (debug views) on 2026-10-31: `light-lab` and `light-machine` became code projects; room 2 gained a sliding door (`Door2D`, a kinematic occluder reversing every period: captures after 0, 60 and 120 steps show it moving with its shadow), room 3 a power switch (`PowerSwitch2D`, `PoweredLamp2D`, `PoweredSpawner2D`: P switches the logical state at once and the lamps fade over 0.6 s, shown together on a screen panel and in the `machine.powered` and `machine.lamp_intensity` gauges; a scene saved unpowered captured at 0.3 s shows "OFF (lamps fading)" with the lamps at half way, and at 1 s at their off values with the sparks stopped), and room 4 is room 3 with its panel open and F turning on ambient occlusion, fog and bloom. The light buffer and overdraw debug views (C API 19 bits 4 and 8, `world.lighting_debug`, Session > Lighting debug) replace the frame with the light alone or with the number of lights shading each pixel; `lighting-tests` (33) passed on the RX 7600 and llvmpipe, a test-only mutation ignoring the view failed its case (8 checks) and the hash-identical restoration passed. Matrix at c4f9a58dc1cd397ea64b73fd8276f45748b2e654: vs2026 and ci Debug/Release 27/27 with 0 warnings (the rooms add the generic C API module and project tests, which needed a scenario, a code-built room and SpawnAt), editor 0 warnings, MCP/development-channel qualification 178 cases in Debug and Release, 11 sample captures byte-identical to L0. No visual review was made: by instruction, the rooms' look is assumed acceptable and remains open to the owner's review under 218.
- `PIXIE-TASK-218` — **Status:** done — L5: consolidation: regression matrix, approved references, stress report, residual limits and the owner's visual review. On 2026-10-31 the [L5 consolidation record](../lighting-l5-consolidation.md) gave the parts that do not need the owner, at code c4f9a58dc1cd397ea64b73fd8276f45748b2e654: the regression matrix (every acceptance case with its evidence; shadows, physics-driven occluders and composition pass bounded, readability is open), a stress report rerun on the RX 7600 at 1920x1080 and on llvmpipe at 1280x720 (the worst room, 64 shadowed lights at High, takes 1.34 ms of GPU lighting and 2.00 ms of wall time at p99, within the declared 3 ms), and the residual limits. Open, for the owner: approving reference images for the rooms (as for PIXIE-TASK-212), the visual and readability review of rooms 1-4 including the optional effects (L06), and the waived L0 review. Owner acceptance on 2026-10-31: after reviewing rooms 1 and 2 in the editor (`samples/light-lab`) and rooms 3 and 4 in the player (`samples/light-machine`), the owner stated "estao aceitas". Reference images: the owner accepted on 2026-10-31 ("parece tudo certo") the recommendation to defer them to `PIXIE-TASK-212`, where the sample list and tolerance policy are chosen; the room captures remain machine-local evidence.

## Verification and evidence limits

The [L5 consolidation record](../lighting-l5-consolidation.md) owns the executed results, at code
`c4f9a58dc1cd397ea64b73fd8276f45748b2e654` on the owner's Windows machine (Ryzen 7 5700X, RX 7600,
Visual Studio 2026, MSVC 19.51): vs2026 and ci, Debug and Release, built with no warning and
passed 27 of 27 CTest tests each; the editor built with no warning; the development-channel
qualification passed 178 cases in Debug and in Release (`--skip-cold`); `lighting-tests` passed 33
tests on the RX 7600 and on Mesa 26.2.4 llvmpipe; 11 sample captures were byte-identical to the
L0 build (hello-gl differs between runs of one build). Test-only mutations failed and byte-for-byte
restorations passed. Budgets are declared for the RX 7600 at 1920 × 1080 only. Machine-local
evidence is under `.local/validation/lighting-l0-2026-10-31/`; it is not part of a clean clone's
proof. Clean-clone CI runs when the branch is published; nothing was pushed.

## Remaining limits

The [residual limits](../lighting-l5-consolidation.md#4-residual-limits) stand: shadow cost per
shadowed light and edge with caps that drop the weakest lights; per-piece self shadow; particle
lights unshadowed and gathered on the CPU; 8 light layers and two tone curves; the optional
effects are approximations, off by default; the door and power switch are sample code. A Choice
field set over the development channel to a number that is not one of its labels wrote 255 instead
of being refused; it is recorded as an editor follow-up, not fixed here.
