# 2D Lighting: L0 Design Record

**Document ID:** GLOBAL-LIGHTING-L0-DESIGN

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** the L0 choices for 2D lighting: data model, units and color space, composition order, compatibility rule and the shadow technique chosen by measured prototypes

**Surface:** guide

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## 1. Status

Gate L0 of [PIXIE-TODO-052](../TODO.md#pixie-todo-052--2d-lighting), task `PIXIE-TASK-213`, on
2026-10-31. This record fixes what the [lighting requirements](lighting-requirements.md) left as
"pin at L0", with the measurements behind the shadow technique.

Owner decision, 2026-10-31: asked whether to waive the L0 review so work continues into L1, the
owner answered "Yes, continue to L1" (relayed from the main session). The review of these choices
was waived, not performed: the choices below are pinned so that L1 can proceed, they remain open
to that review, and any of them can still be revised. Section 9 lists them for it. Once
implemented, public headers and the module references own the API and contract; this record
keeps the rationale.

Baseline: specification commit `6aa6a4cc4e19a750b6056f70ee4fccb55eb7105a`; prototype commit
`b12bb4ccae0b0365f392584a3ba71bccaa7b4245` (`tests/benchmarks/LightingPrototype.cpp`).

## 2. Data model

Components follow the existing scene conventions: one settings component per scene, like
`PhysicsSettings2D`; per-entity components drawn at the entity's interpolated `Transform2D`;
colors authored as `pxColorRgba`; new fields on existing components default to values that change
nothing.

| Owner | Field | Unit / range | Default |
|---|---|---|---|
| `LightingSettings2D` (one per scene; enables lighting) | `ambient` color, `ambientIntensity` | sRGB color; linear multiplier ≥ 0 | white, 1 |
| | `exposure` | linear multiplier > 0 | 1 |
| | `toneMapping` | `None` (clamp) or `Aces` (fitted curve) | `None` |
| | `quality` | `Low`, `Medium`, `High` (section 6) | `Medium` |
| | `maxParticleLights` | global cap on particle light splats | 2048 |
| | optional effects (normal maps, ambient occlusion, bloom, fog) | flags and parameters, shaped at L4 | off |
| `Light2D` | `type` | `Point`, `Spot`, `Directional` | `Point` |
| | `color`, `intensity` | sRGB color; linear multiplier ≥ 0 | white, 1 |
| | `range` | world units; light reaches exactly 0 there (point, spot) | 300 |
| | `falloff` | exponent p ≥ 0 of `saturate(1 - (d/range)^2)^p` | 2 |
| | `sourceRadius` | world units; the light's size, which sets shadow softness (point, spot) | 8 |
| | `sourceAngle` | angular radius, radians in code and degrees in the Inspector (directional) | 0.5° |
| | `coneAngle`, `coneSoftness` | full angle, radians/degrees; fraction 0–1 of the half-angle that fades (spot) | 60°, 0.2 |
| | direction | the entity's rotation: +X turned toward +Y (spot, directional) | — |
| | `castsShadows`, `enabled` | bool | true, true |
| | `lightLayers`, `shadowLayers` | masks over the named light layers: receivers lit, occluders that block | all, all |
| `Occluder2D` | `source` | `Collider` (the entity's collider or compound shapes), `Polygon`, `SpriteAlpha` | `Collider` |
| | `polygon`, `alphaThreshold` | local points (convex pieces); 0–1 | —, 0.5 |
| | `layers` | named light layers it belongs to, matched against `shadowLayers` | first layer |
| | `selfShadow` | whether it shadows its own interior (section 6) | false |
| | `enabled` | bool | true |
| `LightReceiver2D` (on an entity with `Sprite`, `Shape` or `Text`) | `lightLayers` | receiver mask over the named light layers | all |
| | `unlit` | shown at its own color, ignoring light and tone mapping | false |
| `Emission2D` (on an entity with `Sprite`, `Shape` or `Text`) | `color`, `intensity` | sRGB tint; linear multiplier ≥ 0 of the surface's own color, added unlit | white, 1 |
| `ParticleLight2D` (on a `ParticleSystem`, like `ParticlePhysics2D`) | `emissive` | particles draw unlit into the emission buffer, not lit | false |
| | `illuminates` | each live particle adds an unshadowed light splat | false |
| | `lightRadius`, `lightIntensity`, `lightCap` | world units; linear multiplier; splats per pool | 24, 1, 256 |

Receiver, emission and particle-light data are components of their own rather than new fields
of `Sprite`, `Shape`, `Text` and `ParticleSystem`: scenes that do not use them keep their saved
text, and the existing renderers' instance layouts stay as they are. An entity without
`LightReceiver2D` receives every layer's light.

Light layers are up to 8 slots, named per scene by a `LightLayers2D` singleton as
`CollisionLayers2D` names collision layers, so a receiver's mask fits one byte per pixel
(section 4). L1 implements the masks as plain integers (bits 0 to 7); the names and the
Inspector's layer editor come with authoring at L4, since the editor's layer-mask editor reads
collision layer names only. A scene with lights or occluders but no
`LightingSettings2D` is not lit: those components are inert and the scene loader warns once. A
second settings component is ignored with a warning, as duplicate physics settings are.

A splat takes the particle's color and its current alpha, so fading particles dim their light. When
a pool has more live particles than `lightCap`, the first `lightCap` in pool order cast light; the
global cap applies after the pool caps, in World order; dropped splats are counted in telemetry.

## 3. Units and color space

- Positions, sizes, ranges and source radii are world units. The world has Y down, as
  `Camera2D` documents; angles are radians from +X toward +Y in code, degrees in the Inspector.
- Intensities are dimensionless linear multipliers. A white light of intensity 1 at distance 0
  adds 1.0 to the light buffer, which shows an albedo at its own brightness. Physical units stay
  out of scope.
- Colors are authored in sRGB and decoded with the exact sRGB curve. The world's albedo is drawn
  as today, into an 8-bit sRGB-encoded target with today's blending, so blending stays in gamma
  space as it is now (a deliberate limit that keeps unlit content identical); it is decoded to
  linear when combined.
- Light accumulates in a linear RGBA16F buffer. The combine multiplies linear albedo by light,
  adds emission, applies exposure, tone-maps and encodes with the exact sRGB curve for display.

## 4. Composition order

Today, [`ProjectSession::Render`](../runtime/src/ProjectSession.cpp) clears to the project
background, draws sprites below layer 0, paths, the remaining sprites, particles and world shapes,
then edit gizmos, physics debug and the project overlay (submitted to the same shape batch and
flushed with the world shapes), then the screen canvas. Lit scenes follow this order:

1. World pass into the lighting targets, at the output's size: albedo (RGBA8, cleared to the
   project background), emission (RGBA16F: emitted radiance in RGB, from L3) and receivers
   (RGBA8: the layer mask in red and the unlit flag in green, written where coverage is at least
   0.5). Sprites below
   layer 0, paths, remaining sprites, particles and world shapes keep today's order and blending;
   emissive particle pools write emission only. World shapes are flushed here, apart from the
   overlays.
2. Light accumulation into the light buffer: cleared to ambient; then each light, with its
   shadow (section 5), spot cone and receiver layers; then particle light splats, additive and
   unshadowed.
3. Optional effects on linear values (L4): ambient occlusion and normal maps modulate light;
   bloom and fog work on the combined radiance before tone mapping.
4. Combine into the session's output (the editor's render target or the player's window):
   `tonemap(exposure * (albedo * light + emission))`, encoded; unlit pixels blend back to their
   albedo by their unlit weight.
5. Unlit world overlays: edit gizmos, physics debug, the project overlay.
6. The screen canvas, unchanged and never lit.

L1 writes the receiver target in a second pass rather than as a second output of the world pass:
when some entity has a `LightReceiver2D`, its sprites, texts and shapes are drawn again with a
receiver shader, so the existing shaders and instance layouts stay untouched. Paths and particles
are not drawn there and take the receiver beneath them; their own receiver data, with emission,
comes at L3.

The lit buffers have the output's resolution and are recomputed every frame from interpolated
poses, so there is no reprojection to seam or jitter. One behavior differs, in lit scenes only:
`ShapeRenderer::Flush` draws by shape kind, so today a gizmo of one kind can be drawn under a
world shape of a later kind; splitting the flush puts every overlay over every world shape.

## 5. Shadow technique

**Decision: geometric shadow volumes with analytic penumbra coverage.** For each shadow-casting
light, every occluder edge that faces away from the light (within its range) is drawn as a quad
spanning its umbra and penumbra, extruded along the tangents to the light's source disk; the
fragment adds the fraction of the source's angular extent the edge hides into a per-light
occlusion mask; the light then reads the mask. Directional lights extrude along one direction,
with `sourceAngle` as the source's angular radius. Spot lights add the cone in the light's
fragment.

### Method

The prototype renders the same scenes with three techniques and an exact reference:

- `volumes`: as above, back-facing edges, additive occlusion mask (R16F), clamped to 1.
- `polar`: back-facing edges drawn into one row per light of a 1D polar map (512, 1024 or 2048
  angles), the nearest of three rays across each texel, a min pyramid, a blocker search that
  narrows to the nearest blocker's window, then 24 filtered taps (percentage-closer soft shadows).
- `sdf`: occluders filled into an occupancy texture over the view plus 320 units (1, 2 or 4 world
  units per texel, snapped to the world grid), a signed distance field by jump flooding, then a
  sphere trace toward each light with a signed-clearance penumbra; a ray that enters an occluder
  is blocked.
- `reference`: per pixel, 1024 directions spread evenly over the source disk's angular extent,
  each tested exactly against every occluder (convex polygon ray tests); visibility is the
  fraction not hidden.

All of them follow the same self-shadowing rule (section 6). Visibility (0–1) is compared within
the light's range: mean and 99th-percentile absolute error and signed bias; *leak*: the share of
umbra pixels, eroded by 2 pixels, lit above 0.05, and the brightest of them; *acne*: the share of
fully lit pixels, eroded by 2 pixels, darkened below 0.95. Flicker: over 90 frames at 60 steps
per second, with the camera panning 0.37 × 0.21 pixels per frame, the change of the per-pixel
error from frame to frame, for a scene with a rotating platform, a sliding door and a moving light
(`moving`) and for the scene held still (`camera`). Cost: GPU timestamp queries around shadow
preparation, light accumulation and the combine, plus CPU time to update and issue a frame, 30
warm-up frames and 120 measured, with every box turning each frame so occluders are rebuilt.

Scenes, 960 × 720, one light of range 340: `mixed` (box, rotated plank, a small box near the
light, an L of two touching boxes, a 3-unit wall, a thin post, two boxes in a row; source radius
12), `mixed-soft` (the same, source radius 30), `closed-room` (four 4-unit walls touching exactly
at the corners around the light; nothing outside may be lit), `corner` (a light near the outer
corner of two touching boxes).

### Results on the owner's GPU

AMD Radeon RX 7600 (driver 32.0.31036.15, GL 4.5 core 23.20.30.260716), AMD Ryzen 7 5700X,
Windows 11 Pro 10.0.26300, MSVC 19.51, vs2026 Release, prototype commit `b12bb4cc`.

| Scene | Technique | Mean error | Bias | p99 error | Leak (share / max) | Acne |
|---|---|---|---|---|---|---|
| mixed | volumes | 0.0025 | −0.0024 | 0.11 | 0 / 0 | 0 |
| mixed | polar 1024 | 0.0125 | −0.0038 | 0.29 | 4.2% / 0.75 | 0.70% |
| mixed | sdf 2 | 0.0281 | −0.0168 | 0.44 | 1.2% / 0.94 | 1.9% |
| mixed-soft | volumes | 0.0081 | −0.0079 | 0.35 | 0 / 0 | 0 |
| mixed-soft | polar 1024 | 0.0497 | +0.0036 | 0.40 | 48% / 0.46 | 5.1% |
| mixed-soft | sdf 2 | 0.0616 | −0.0489 | 0.47 | 1.7% / 0.68 | 3.8% |
| closed-room | volumes | 0.0001 | 0.0001 | 0 | 0 / 0 | 0 |
| closed-room | polar 1024 | 0.0011 | 0.0003 | 0 | 0 / 0.036 | 0 |
| closed-room | sdf 2 | 0.0005 | −0.0002 | 0 | 0 / 0 | 0 |
| corner | volumes | 0.0000 | 0.0000 | 0 | 0 / 0 | 0 |
| corner | polar 1024 | 0.0015 | −0.0012 | 0.06 | 0 / 0 | 0 |
| corner | sdf 2 | 0.0076 | −0.0028 | 0.30 | 1.6% / 1.0 | 1.5% |

Polar at 2048 angles and sdf at 1 unit per texel barely change the mixed scenes (mean error within
12%, leaks alike): the errors come from the soft-shadow approximation, not from resolution. The sdf "leaks" in `corner` are its
own-interior rule acting on the union of touching occluders (section 6), not light through walls.

| Flicker sequence | Technique | Mean error | Frame-to-frame change: mean / p99 |
|---|---|---|---|
| moving | volumes | 0.0006 | 0.0004 / 0.0005 |
| moving | polar 1024 | 0.0077 | 0.0034 / 0.070 |
| moving | sdf 2 | 0.0200 | 0.0061 / 0.072 |
| camera | volumes | 0.0000 | 0.0001 / 0.0005 |
| camera | polar 1024 | 0.0030 | 0.0011 / 0.0054 |
| camera | sdf 2 | 0.0138 | 0.0020 / 0.027 |

GPU time per frame at 1920 × 1080, p50 (p99) in milliseconds, all stages; "none" is the same
lighting without shadows. CPU time to update and issue a frame stayed below 0.1 ms for every row.

| Lights / boxes | none | volumes | polar 1024 | sdf 2 |
|---|---|---|---|---|
| 1 / 128 | 0.04 (0.05) | 0.07 (0.08) | 0.12 (0.41) | 1.37 (2.08) |
| 16 / 32 | 0.13 (0.20) | 0.45 (0.50) | 0.87 (1.13) | 1.25 (1.58) |
| 16 / 128 | 0.09 (0.10) | 0.35 (0.50) | 0.81 (0.85) | 1.28 (1.60) |
| 16 / 512 | 0.11 (0.30) | 0.65 (1.02) | 2.21 (2.45) | 1.34 (1.66) |
| 64 / 128 | 0.16 (0.17) | 0.97 (1.35) | 1.84 (1.95) | 1.48 (1.86) |
| 64 / 512 | 0.17 (0.19) | 1.39 (1.82) | 3.98 (4.15) | 1.64 (2.04) |

The distance field's preparation alone costs 1.0–1.5 ms at 2 units per texel and 9.3 ms at 1 unit
(16 lights, 128 boxes); at 4 units it falls to 0.23 ms, but 4-unit walls then leak.

### Results on Mesa llvmpipe

Mesa 26.2.4 from mesa-dist-win (the release CI pins, archive SHA-256 checked), llvmpipe with
LLVM 23.1.2 and 256-bit vectors, GL 4.6 core, on the same machine (Ryzen 7 5700X, 8 cores, 16
threads), vs2026 Release, prototype commit `b12bb4cc`.

Quality and flicker match the GPU's to the third decimal for every technique and scene (volumes:
mean error 0.0025 `mixed`, 0.0081 `mixed-soft`, 0 leaks and acne; frame-to-frame change 0.0004
`moving`), so the comparison above holds in software too.

llvmpipe runs the commands as they are issued, so CPU and GPU time coincide; the table gives the
GPU timestamps' p50 (p99) in milliseconds at 1920 × 1080.

| Lights / boxes | none | volumes | polar 1024 | sdf 2 |
|---|---|---|---|---|
| 1 / 128 | 3.9 (4.6) | 4.3 (6.4) | 5.4 (6.3) | 51.6 (65.6) |
| 16 / 32 | 7.1 (8.0) | 15.1 (16.7) | 39.5 (47.0) | 66.8 (84.5) |
| 16 / 128 | 6.9 (8.0) | 14.9 (17.0) | 44.1 (50.9) | 65.9 (83.5) |
| 16 / 512 | 7.9 (12.5) | 21.5 (26.8) | 122.6 (169.0) | 76.0 (122.8) |
| 64 / 128 | 12.2 (14.2) | 47.0 (48.6) | 101.7 (119.0) | 95.8 (123.0) |
| 64 / 512 | 11.4 (12.3) | 52.8 (69.4) | 236.5 (273.6) | 100.7 (142.3) |

Volumes are the cheapest shadowed technique at every load, at both resolutions, in software. The
distance field's preparation alone costs 46 ms at 2 units per texel (283 ms at 1 unit). The
combine step costs 3.2–3.8 ms at this size whatever the technique; CI's lighting tests therefore
render small frames (256 × 256 or less), where one shadowed light costs about a millisecond.

### Why volumes

- Quality: the only technique without leaks or acne in every scene, and the closest to the
  reference (mean error 0.0025 hard, 0.0081 soft). Its residual error is over-darkening where the
  penumbrae of separate occluders overlap, since their occlusion is summed (bias −0.008 at source
  radius 30): darker crossings, never light through geometry.
- Stability: its frame-to-frame error change is at the reference's own quantization (1/1024)
  under a rotating platform, a sliding door, a moving light and a sub-pixel camera pan, 8 to 30
  times below the others; shadows are recomputed from exact geometry, with no texel grid to swim.
- Cost: the cheapest on the owner's GPU at every load at 1920 × 1080 and up to 16 lights at
  1280 × 720 (all 1.4 ms or less at p50, 1.8 ms or less at p99). At 1280 × 720 with 64 lights and
  32 or 512 boxes, the distance field, whose cost barely depends on light count, was 0.15–0.28 ms
  cheaper. On llvmpipe they are the cheapest at every load. Volumes grow with lights times the
  screen area of shadow quads; culling occluders per light by bounds, which the prototype did not
  do, reduces it.
- Fit: exact edges agree with the physics shapes they come from; directional lights need only a
  different extrusion; quality steps (mask resolution, shadowed-light cap) are independent of the
  scene's geometry.

Rejected: polar maps, because the blocker search mis-estimates penumbrae with several blockers
(48% of the soft scene's umbra lit above 0.05, 5% acne), they flicker 8 times more, cost 2–3 times
more per light, and directional lights would need a second kind of map. The distance field,
because thin walls leak unless every hit blocks, which loses the inner penumbra (bias −0.05 soft);
its resolution must track wall thickness (walls of at least 2 texels); and it merges touching
occluders. Its cost is nearly flat in light count, and a coarse one may still serve ambient
occlusion at L4.

### Limits of this evidence

- The reference models a light as a disk whose angular extent is uniformly bright; volumes
  compute that quantity analytically, so their agreement validates the implementation against an
  independent exact one, not the physical model.
- Occluders were boxes only; circles, capsules and sprite outlines become convex polygons
  (section 6), which these scenes did not exercise. One light per quality scene: overlap between
  lights is additive in every technique and was not compared.
- Timing used a hidden 64 × 64 window rendering offscreen, no world sprites, no culling of
  occluders per light, and `glFinish` after every frame; the prototype's CPU work is far smaller
  than the engine's gathering will be. Cold start and shader compilation were not measured.
- Release only, one GPU with one driver and llvmpipe on one CPU, one session each; the numbers
  hold for that machine, configuration and commit. CI's runners have other CPUs.

## 6. Other L0 choices

- **Self-shadowing.** An occluder never shadows its own interior: its sprite receives each light
  as if the occluder were absent, while other occluders still shadow it. Shading by facing side
  comes from optional normal maps. `selfShadow` makes an occluder shadow itself, leaving only a
  soft rim on the light's side lit; volumes do it by extruding that occluder's light-facing edges
  instead. "Own" means every shape of the entity; the prototype applied the rule per convex piece,
  and L2 decides per entity with an occluder-ID mask if compound occluders show the difference.
  L2 (2026-10-31) kept the per-piece rule: no room has shown the difference yet, and the review
  that would decide it is pending.
- **Mask precision.** L2 stores the occlusion mask in R32F, not the prototype's R16F: summed in
  half floats, the fractions of adjacent edges stopped just short of 1 and left a faint value of 1
  or 2 (out of 255) behind closed walls.
- **Occluder geometry.** Boxes and polygons are used as they are; circles and capsules become
  polygons whose chords stay within 0.5 world units of the curve (8 to 32 segments per circle);
  sprite alpha is traced once per image, region and threshold into convex pieces and cached, not
  per frame. All follow the interpolated pose used for drawing, including kinematic platforms,
  dynamic bodies and an opted-in character's capsule.
- **Aggregate particle shadow (LIT-011).** Out of scope: a pool's light splats stay unshadowed. A
  shadowed light that follows an emitter is an ordinary `Light2D` on the emitter's entity.
- **Shadowed-light budget (LIT-008).** Quality presets bound the shadow-casting lights in view:
  `Low` 8 with a half-resolution occlusion mask, `Medium` 32, `High` 64, both at full resolution.
  Over the cap, the lights with the smallest contribution (intensity times covered screen area)
  are dropped, not drawn unshadowed, which would leak; the dropped count is in telemetry and the
  debug view.
- **Readability.** The reduced-effects preset and a minimum ambient (LIT-014) are settings fields
  shaped with the quality presets at L4.

## 7. Compatibility rule

A scene without `LightingSettings2D` runs today's `ProjectSession::Render` unchanged: no lighting
targets, no extra flush, no new shader outputs, so it renders pixel-identically by construction;
the existing captures and snapshot tests are the check at L1. Scenes saved before lighting load
unchanged, since every new field defaults to a neutral value; unknown fields survive save as
today. With settings at their defaults (ambient white at 1, exposure 1, no tone mapping) and no
lights, the combine computes `encode(decode(albedo))` with no emission; L1 must show it
identical to the unlit image, byte for byte, on the owner's GPU and on llvmpipe.

## 8. Where the code goes

The render module owns GL resources, the lighting targets, the shadow technique and the combine,
beside the existing 2D renderers. The runtime owns the components, gathers lights, occluders and
emissive data from the World at interpolated poses (as `SpriteDrawing` does), and
`ProjectSession::Render` composes the passes, so the editor and the player share one path
(LIT-001). Native code never depends on the editor, and new C API entries raise its version
(LIT-019).

## 9. Choices open to the owner's review

Pinned on 2026-10-31 with the review waived (section 1); each remains revisable.

1. Shadow technique: geometric shadow volumes with analytic penumbra (section 5).
2. Data model, defaults and units as in sections 2 and 3, including receiver, emission and
   particle-light data as components of their own, 8 light layers and `toneMapping` limited to
   `None` and `Aces`.
3. Composition order as in section 4, with gizmos over all world shapes in lit scenes.
4. Self-shadowing off by default, with `selfShadow` per occluder (section 6).
5. No aggregate shadowed light per particle pool.
6. Shadowed-light caps of 8, 32 and 64 by preset, dropping the weakest over the cap.
