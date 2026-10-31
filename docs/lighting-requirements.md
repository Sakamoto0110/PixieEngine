# 2D Lighting: Requirements and Proposed Design

**Document ID:** GLOBAL-LIGHTING-REQUIREMENTS

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** proposed 2D lights, shadows, emissive surfaces and particles, composition, authoring, budgets and staged acceptance

**Surface:** guide

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## 1. Status and ownership

Design baseline: 2026-10-31. The [physics specification](rigid-body-physics-requirements.md#11-lighting-and-room-sequence-after-physics)
required that lighting receive a render contract and acceptance budget of the same detail before
any lighting code. The owner promoted this work on 2026-10-31 as
[PIXIE-TODO-052](../TODO.md#pixie-todo-052--2d-lighting); TODO.md remains the scheduler.

This document owns the proposed lighting design input and its acceptance matrix. The
[consumer requirements](circuit-puzzle-requirements.md#4-lighting-quality-critical-requirements)
(L01–L06) own the game's needs; [ROADMAP.md](../ROADMAP.md) owns direction. On implementation,
public headers and the module references own the actual API and contract. Use the stable IDs
below when splitting work. Choices marked "pin at L0" are proposals until that gate decides them.

Starting point: the session renders sprites, paths, particles and shapes in world space, then
gizmos, physics debug and the project overlay, then the screen canvas, all through OpenGL 4.5
core, in the editor and the generic player alike. There is no light, shadow or emissive concept.

## 2. Scope and completion boundary

In scope: ambient, point, spot and directional lights; occluders and soft shadows, including
moving and rotating occluders driven by physics; emissive sprites, shapes and text; emissive and
light-casting particle pools; light layers; a defined composition order with unlit screen UI;
linear-space accumulation, exposure and tone mapping; editor authoring, preview parity, debug
views; telemetry and budgets; deterministic captures. Normal maps, ambient occlusion, bloom and
fog are optional effects behind quality settings (consumer L06): they must justify themselves in
the rooms' review and are not prerequisites.

Out of scope: 3D or volumetric lighting, global illumination or bounced light, baked lightmaps,
reflections, per-particle shadows, colored or translucent shadow casters (a deferred choice),
physically based light units, and character surface materials (a separate physics item).
Joints are unrelated: a directional light needs no physics joints.

Scenes without lighting components must render exactly as they do today. Lighting is opt-in per
scene.

## 3. Ownership and data model

Names describe proposed responsibilities, not final declarations.

| Owner | Authored data | Runtime data |
|---|---|---|
| Scene lighting settings (one per scene) | Ambient color and intensity, exposure, tone mapping, quality preset, optional effects | Light buffers, shadow resources, telemetry |
| Light | Type (point, spot, directional), color, intensity, radius and falloff, cone angle and softness, casts shadows, layer masks | Culling state, shadow data |
| Occluder | Shape source (the entity's collider or compound shapes, an explicit polygon, or the sprite's alpha), enabled, layer masks | Interpolated geometry per frame |
| Emissive surface | Emissive color and intensity on Sprite, Shape or Text; unlit flag | Fades driven by game code |
| Particle pool | `emissive` and `illuminates` flags, per-particle light radius, intensity and color, a per-pool cap | Light splats per frame |
| Sprite material (optional) | Normal map, light height | — |

**LIT-001 — One lighting path.** Editor Edit and Play and the generic player use the same native
renderer and produce the same lit image for the same state and camera. Rendering never steps the
simulation; lights, occluders and emissive surfaces use the same interpolated poses as drawing.
Native lighting stays usable without C#.

**LIT-002 — Opt-in and compatible.** A scene with no lighting settings and no lights renders
pixel-identically to the current renderer. Adding settings with full ambient and no lights is
also visually neutral. Existing saves load unchanged; unknown fields survive save.

## 4. Lights and units

**LIT-003 — Light types.** Ambient: one scene color and intensity. Point: position, radius,
falloff curve. Spot: point plus direction (the entity's rotation), cone angle and edge softness.
Directional: a direction only, parallel rays across the view, for sun, moon or window light.
Every light has a color, a linear intensity and an enabled flag.

**LIT-004 — Units and color space.** Positions and radii are world units; angles are radians in
code and degrees in the Inspector. Intensity is a dimensionless linear multiplier (pin at L0;
physical units are out of scope). Lighting accumulates in linear space; sprite colors are
decoded from sRGB, and the result is tone-mapped and encoded for display. Exposure and the tone
curve are scene settings.

**LIT-005 — Light layers.** Lights affect receivers by layer mask, and occluders block lights by
layer mask, using named slots like collision layers. Screen UI, panels, text overlays, gizmos and
debug drawing are never lit (consumer L04). A receiver can be marked unlit.

## 5. Occluders and shadows

**LIT-006 — Occluders.** An occluder component casts shadows from the entity's existing collider
or compound shapes by default, so physics and shadows agree; an explicit polygon or the sprite's
alpha are alternatives. Occluders follow interpolated poses, including kinematic platforms,
doors and dynamic bodies. A character may opt in to cast a shadow through its capsule. An
occluder's own sprite is lit on its facing side (self-shadowing rule pinned at L0).

**LIT-007 — Shadow technique.** Pin at L0 by measured comparison on the target hardware and on
Mesa llvmpipe, from: geometric shadow volumes from occluder edges (exact, soft edges by penumbra
wedges), per-light 1D polar shadow maps (bounded cost, soft edges by filtering), or a distance
field built per frame. Whatever is chosen must give soft shadows, configurable quality, and
stable results under motion.

**LIT-008 — No leaks or flicker.** Light does not leak through closed geometry or at wall corners
and touching occluders; moving or rotating occluders and a moving camera do not make shadows
flicker, swim or acne (consumer L02). Shadow-casting lights are bounded per frame and degrade by
quality setting, never silently.

## 6. Emissive surfaces and particles

**LIT-009 — Emissive surfaces.** Sprites, shapes and text can carry emissive color and intensity
that shows at full strength in darkness and feeds bloom when enabled. Game code drives it from
simulation state, with fades whose logical state remains unambiguous (consumer L03). Color must
not be the only cue: that is a content rule, not an engine guarantee.

**LIT-010 — Emissive particles.** A pool flagged `emissive` draws its particles unlit, at their
own color and intensity, and feeds bloom. Cost stays close to today's particle drawing.

**LIT-011 — Light-casting particles.** A pool flagged `illuminates` adds each live particle to the
light buffer as an unshadowed light splat with its radius, intensity and color, capped per pool
and globally. Pools choose this separately: sparks can light the floor while dust neither glows
nor lights. Per-particle shadows are not provided; an optional single aggregate light per pool
may cast a shadow (pin at L0 whether it is in scope).

## 7. Composition and effects

**LIT-012 — Composition order.** Pinned at L0 and documented, proposed as: world albedo; light
accumulation (ambient, lights with shadows, particle light splats); combine; emissive surfaces
and emissive particles; tone mapping; optional bloom and fog; unlit world overlays (gizmos,
physics debug, project overlay); screen canvas UI. No seams between layers, no camera jitter at
supported resolutions and zooms, no lighting on UI (consumer L04).

**LIT-013 — Optional material response.** Normal maps (per-sprite texture and light height) and
2D ambient occlusion (darkening near occluders and contacts) are off by default, toggled by
quality settings, and measured. Bloom and fog likewise (consumer L06).

**LIT-014 — Readability.** A reduced-effects preset and a minimum ambient keep player silhouettes,
floor edges, sockets and panel text readable (consumer L01). Darkness must never hide required
interactables; that is a level-design rule the rooms check.

## 8. Authoring, diagnostics and budgets

**LIT-015 — Authoring.** The Inspector exposes the components with units and tooltips; viewport
gizmos edit radius, cone and direction; occluder outlines show the casting geometry. Undo/redo,
save/reopen, Play/Stop, two sessions and invalid input (negative radius, nonfinite values,
duplicate settings) behave as physics authoring does, without assertions.

**LIT-016 — Debug views.** Toggle light bounds, occluder geometry, shadow data, the light buffer
and an overdraw view, in the editor and the player, and through the development channel for
captures.

**LIT-017 — Telemetry and budgets.** Record CPU and GPU time separately (GPU timer queries) for
light accumulation, shadows, particle splats and optional effects; light, shadow-caster,
occluder and illuminating-particle counts; resolution and quality settings. Budgets are declared
per named machine and room, with p50/p95/p99 and worst frame (consumer L05). Lights and occluders
outside the view are culled.

**LIT-018 — Captures and references.** Lit frames are capturable deterministically with the
existing fixed-step capture; reference images for the rooms need the owner's approval, as for
PIXIE-TASK-212. CI renders the lighting tests on Mesa llvmpipe, so they must run in software.

**LIT-019 — Platform and boundaries.** OpenGL 4.5 core as today; no development code in exports;
C API additions raise its version; native engine code never depends on the editor.

## 9. Acceptance matrix

Every case names its scene, camera, seed, resolution, renderer and tolerance. Visual cases need
captures plus written observation; the owner's review is required for look and readability.

| Case | Requirements | Observable evidence / proposed pass condition |
|---|---|---|
| Compatibility | LIT-002 | Existing samples and scenes without lighting capture pixel-identically before and after |
| Light types | LIT-003–005 | Analytic falloff and cone checks on a flat receiver; directional rays parallel; layers include and exclude as authored |
| Shadows | LIT-006–008 | Corner, touching-occluder and door cases show no leaks; a rotating platform and a moving camera show no flicker across a recorded sequence |
| Physics-driven occluders | LIT-006 | Platforms, doors, boxes and an opted-in character cast shadows at their interpolated poses at 30/60/144 render FPS |
| Emissive and transitions | LIT-009 | Powered/unpowered lamps fade on command; the logical state is recorded with the visual transition |
| Particles | LIT-010–011 | An emissive pool glows unlit; an illuminating pool lights the floor; a non-illuminating pool does not; caps hold under a burst |
| Composition | LIT-012 | Pan/zoom near bright sources with panels open: no seams, jitter or lit UI |
| Readability | LIT-014 | The same room powered, unpowered and in reduced effects keeps the listed elements readable |
| Authoring | LIT-015–016 | No-code create/edit/undo/save/reopen/Play/Stop and invalid edits, scripted over the development channel |
| Budgets | LIT-017 | Named stress rooms with 0/16/64 lights and illuminating particle bursts, CPU and GPU recorded separately, warm and cold apart |

Bounded tests must show sensitivity, for example disabling occlusion, layers or tone mapping in a
test-only variant. Look, readability and atmosphere require the owner's review in the rooms.

## 10. Proposed delivery gates and rooms

| Stage | Deliverable | Exit gate |
|---|---|---|
| L0 | Pin data model, units, composition order, shadow technique (measured prototypes) and compatibility rule | Owner review of the pinned choices |
| L1 | Lighting pipeline: settings, ambient, point, spot, directional, layers, unlit UI, linear accumulation and tone mapping | Compatibility captures identical; light-type and layer cases |
| L2 | Occluders and soft shadows, including physics-driven occluders and opt-in character shadows | Leak, flicker and render-rate cases; room 1 and room 2 |
| L3 | Emissive surfaces, emissive and illuminating particle pools, transitions | Emissive and particle cases; room 3 |
| L4 | Authoring, gizmos, debug views, telemetry and budgets, optional effects behind quality settings | Authoring and budget cases; room 4 |
| L5 | Consolidation: regression matrix, references, stress report, limits, owner visual review | Unresolved required cases remain open |

Rooms, reusing the physics rooms where they fit: (1) a dark room with one movable light, static
occluders and a physical box; (2) several lights with rotating and moving occluders (platforms,
a door); (3) a powered and unpowered machine with emissive indicators, sparks that light the floor
and smooth transitions; (4) a combined room for atmosphere and performance with a panel open.
Each room needs save/reopen, editor and player parity and named captures.
