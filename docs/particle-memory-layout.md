# Particles: Memory Layout and Processing

**Document ID:** GLOBAL-PARTICLE-MEMORY-LAYOUT

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** the particle's 40-byte layout, how pools are stepped and drawn, and the open directions for dynamic particles and SIMD

**Surface:** guide

**Boundary:** runtime

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## 1. Status and purpose

Written 2026-10-26 as a handoff to the agent taking on dynamic particles, after
the particle work of PIXIE-TODO-044 to 047 ([records](archive/README.md),
PIXIE-ARCHIVE-081 to 084). It describes what exists and why, so the next step
starts from facts rather than from re-reading the code.

**Nothing here is settled for the next step.** The layout, the split into
streams, the SIMD plan and the profile-table idea in sections 5 and 6 are
proposals from one discussion with the owner. Disagree with any of them, and
propose another design, with your reasons; the owner decides. Only section 2's
invariants describe behavior the engine and its scenes rely on today, and even
those can change through a promoted TODO.

The [runtime reference](modules/runtime/REFERENCE.md#particles) remains the
contract; this guide adds the reasoning and the numbers behind it.

## 2. What exists

Particles are not entities. One `ParticleSystem` component owns a contiguous
`std::vector<Particle>` (the pool), up to `capacity`, with a draw `style` and
the affectors added in code. Everything else is a component on an entity:

| Component | Where | Role |
|---|---|---|
| `ParticleSystem` | its own entity | the pool, `capacity`, the default look (`color`, `size`, `shape`) and the pool-wide draw settings (`additive`, `fade`, `minPixels`) |
| `ParticleSpawner` | its own entity, with a `Transform2D` | emits into the system named by `system`: rate, lifetime, speed, emission shape, `jitter`, direction mode, and optionally its own look (`ownLook`, `particleColor`, `particleSize`, `particleShape`) |
| `ParticleAttraction` | the system's entity | the pool is pulled by every `Attractor`, times `scale` (negative pushes) |
| `ParticleInteraction` | the system's entity | the pool's particles closer than `radius` push each other apart |
| `Attractor` | its own entity, with a `Transform2D` | a source of pull; `killOnContact` removes particles inside its body |

All of these are described in `DescribeRuntimeComponents`, so scenes save them
and the editor edits them; a project without code authors the whole effect.
Every World a `ProjectSession` builds has the particle step (`AddParticleStep`,
installed once per World).

Invariants today:

- **A particle is born complete.** Every path builds it through `MakeParticle`
  (in `engine/include/pixie/Particle.hpp`): `ParticleSystem::Emit` with the
  system's look or a look given, and the spawner through `Emit`.
- **Its look and kind are fixed for life.** Only its motion and age change.
  Editing a system's style affects particles born afterwards.
- **Pool order is not stable.** A particle that dies is replaced by the last one
  (swap-remove). Nothing may keep an index into a pool across steps.
- **`previous` is where the particle was drawn from.** The GPU interpolates
  between `previous` and `position`; a newborn's `previous` is its birth point.
- **Affectors change velocities and read positions**, so their result does not
  depend on particle order. They may kill a particle (`KillParticle`), which the
  step removes before anything moves.

## 3. The 40-byte layout

`Particle` is the CPU's pool element and, byte for byte, the GPU's per-instance
vertex format. `static_assert` pins it at 40 bytes and trivially copyable.

| Offset | Size | Field | Type | Shader input |
|---|---|---|---|---|
| 0 | 8 | `position` | 2 × float, world units | `location 0`, `vec2` |
| 8 | 8 | `previous` | 2 × float | `location 1`, `vec2` |
| 16 | 8 | `velocity` | 2 × float, units per second | `location 4`, `vec2` (Streak direction) |
| 24 | 4 | `age` | float, seconds | `location 2` |
| 28 | 4 | `lifetime` | float, seconds, 0 is forever | `location 3` |
| 32 | 4 | `color` | RGBA8 | `location 5`, normalized `vec4` |
| 36 | 2 | `size` | half float, radius in world units (`ToHalf`, `FromHalf`, `Size()`) | `location 6`, `GL_HALF_FLOAT` |
| 38 | 1 | `flags` | bits 0-1 shape (`kShapeBits`), bit 2 `kDynamic` (reserved, unused), bits 3-7 free | `location 7`, integer `uint` |
| 39 | 1 | `spare` | free | not bound |

Free today, without growing: **5 bits** of `flags` and the **8 bits** of `spare`,
13 bits, beside the reserved dynamic bit. Shapes take 2 bits for 4 shapes and up to
7 bits for 128, all inside `flags`.

Why each choice:

- **float, not double**, for motion: the pool is walked several times per step
  and uploaded every frame; components around it (`Transform2D`, spawner,
  attractor) stay double, and emission computes in double before writing floats.
- **Half float for size**: 11 significant bits (about 0.05%) are plenty for a
  radius, and the GPU reads it natively. `ToHalf` rounds to nearest, ties to even.
- **The look in the particle, not the system**: spawners of different looks
  share one pool and one draw call.
- **32 to 40 bytes cost**, measured (section 7): about 0.1 ms more per frame at
  100,000 particles, from the larger upload.

Do not use a C++ `union` to give the same bytes different meanings: reading an
inactive member is undefined behavior. The design note in
[ROADMAP.md](../ROADMAP.md#accepted-direction-expansion-required) proposes a fixed
payload slot read through `std::bit_cast` accessors, its meaning chosen by flag
bits, which the shader decodes the same way.

## 4. How a pool is processed

`AddParticleStep` adds one system, `particles.step`, to `PostUpdate`, after the
Behaviours and the World's integration, so spawners and attractors are where this
step left them. For each `ParticleSystem`, in storage order:

1. **Age and expire.** `previous = position`; `age += dt`; a particle past a
   non-zero lifetime is swap-removed.
2. **Spawn.** Every `ParticleSpawner` whose `system` is this one emits what it
   owes (`rate · dt` plus its carried fraction). Each particle gets its own
   moment within the step, when its share came due: it starts back along its
   velocity by that much, its `previous` is its birth point, and its age is the
   part of the step it lived, so a fast stream comes out evenly spaced. `jitter`
   offsets each start by a Gaussian. A full pool drops the rest of the step.
3. **Affectors added in code**, in the order added (`ParticleAffector::Apply`
   receives `std::span<Particle>`).
4. **Scene affectors**, from components on the system's entity: attraction, then
   interaction. Their working buffers (the interaction's spatial grid) live in
   the registry's context, keyed by system entity, never in the scene.
5. **Remove the killed** (swap-remove), so they are never drawn again.
6. **Move.** `position += velocity · dt`.

The pool is therefore walked about five times per step, each pass touching the
whole 40-byte element. At 1,000,000 particles (40 MB, past the cache) the step
cost about 15 ms, roughly 1.7 times a linear extrapolation from 100,000: the step
is memory-bound, not compute-bound.

Drawing (`DrawParticles`, `ParticleRenderer`): one instanced draw per system;
the whole pool is uploaded every frame; each instance is a quad sized by its own
radius (never below `minPixels` on screen), stretched behind a Streak by
`speed · kStreakSeconds` (0.05 s); the fragment shader covers a disc, a square, a
ring (hole at 0.6 of the radius) or a capsule fading toward its tail. Per-system
uniforms: view-projection, interpolation alpha, minimum radius, fade.

## 5. Dynamic particles: the open design

The roadmap direction is: a spawner marks its particles dynamic; static and
dynamic particles share one pool; only the dynamic ones feel gravity and
colliders, so the cost of collision follows their number, not the pool's, and is
reported separately in telemetry rather than capped. Collider responses (bounce,
stop, die, pass), the payload, and the cost report are yours to design.

Ideas from the discussion, for you to accept, change or replace:

- **Profile index instead of values.** The `spare` byte indexes up to 256
  *dynamic profiles* held by the system or the spawner (restitution, friction,
  gravity scale, layer mask, response). One byte per particle carries any number
  of parameters, shared by every particle of the same behavior.
- **Small discrete state in the free bits**: a response (2 bits), a contact count
  (4 bits), a layer index (5 bits).
- **Continuous per-particle state** (rotation, angular velocity, last contact
  normal, timers) does not fit in 13 bits. Rare data goes to a side table for the
  dynamic particles only, kept in step with swap-removal; data for mutually
  exclusive kinds shares a payload slot.
- **Keep `previous` for dynamic particles.** Deriving it from velocity breaks
  exactly after a collision or a teleport, when the last step's motion is not the
  velocity.
- **The pool's order is not stable**, so a side table must follow swap-removal,
  or index by something stable.

## 6. Streams and SIMD: the open direction

If dynamic particles need more than the free bits and a profile, the discussion
favoured splitting the pool rather than growing the struct to 64 bytes:

| Stream | Content | Read by | Changes |
|---|---|---|---|
| Motion | x, y, previous, velocity, age, lifetime | CPU step, GPU | every step |
| Look | color, size, flags | GPU | at birth (and on swap-removal) |
| Dynamic | whatever collision needs | CPU | dynamic particles only |

A single 64-byte struct would upload CPU-only data every frame and make every pass
pull whole cache lines for 16 bytes of position and velocity. Streams let each
consumer read its slice: OpenGL vertex arrays read attributes from several
buffers in one draw at no cost, and the dynamic stream never reaches the GPU.

For SIMD, the useful batch is one field of eight particles, not one 64-byte
particle in two registers: per-field arrays (`x[]`, `y[]`, `vx[]`, `vy[]`, ...)
let AVX2 run `x += vx · dt` on eight particles in one fused multiply-add, with
plain aligned loads. The owner's machine (Ryzen 7 5700X, Zen 3) has AVX2, FMA,
SSE4.1, BMI2, ABM and F16C (read by CPUID on 2026-10-26), and no AVX-512; F16C
converts eight half floats per instruction, which the size's `ToHalf` and
`FromHalf` could use instead of their portable bit work. **Compile for AVX2**:
on the owner's decision of 2026-10-26, every engine target and every game built
with `pixie_game` compiles with `/arch:AVX2` (`-mavx2 -mfma -mf16c` outside MSVC)
through `cmake/PixieTargetDefaults.cmake`, so AVX2, FMA and F16C intrinsics and the
auto-vectorizer's 256-bit code are available in particle loops; `-DPIXIE_AVX2=OFF`
exists only to build for a CPU without AVX2. No loop uses intrinsics yet.

Costs to weigh: the affector API (from `std::span<Particle>` to a view of
streams, for both runtime affectors and any game's), swap-removal touching every
array, padding arrays to multiples of eight, and masks instead of branches for
kills. Merging the step's passes into one loop is independent of all this and
attacks the memory traffic directly; moving the simulation to a GPU compute
shader is the path to millions, at the price of how affectors are written.

## 7. Measurements and how to reproduce them

Your tests will differ from the ones written here; the numbers below are a
reference from one machine, not targets. All were taken on the owner's machine
(Ryzen 7 5700X, Radeon RX 7600, Windows 11), Release, vsync off, from the
particles sample's log (`particles step: ... ms average` and the frame rate). The
32- and 40-byte columns were built for the x64 baseline (SSE2), before the switch
to AVX2; the last column is the same 40-byte code compiled with `/arch:AVX2`.

| Scenario | 32-byte particle | 40-byte particle | 40-byte, AVX2 |
|---|---|---|---|
| 100,000 particles, 1 attractor, no interaction: step | 0.71-0.84 ms | 0.84-0.88 ms | 0.81-0.85 ms |
| same: frames per second | about 2,230 | about 1,840 | about 1,850-1,900 |
| 10,000 particles with interaction: step | 2.4-2.8 ms | 2.47-2.54 ms | 2.42-2.48 ms |
| 1,000,000 particles, 1 attractor, no interaction: step | | 14-16.5 ms (frames fall to about 5 per second as the fixed steps take the whole second) | |

AVX2 alone gains 3 to 5%: the step is memory-bound, and in the array of 40-byte
structs the compiler finds little to vectorize; the gain section 6 expects needs
per-field arrays.

A dense spawner (3,000 per second, speed 40 to 80, interaction radius 4), mean
speed after 1.5 s: 760 with a step's particles stacked on one point, 539 with the
staggered births, 204 with `jitter` 4.

To reproduce with the repository alone: copy `samples/particles/`'s
`particles.pixie`, `scenes/` and `fonts/` to a scratch folder; in the copy's
`scenes/main.scene`, set the `scenario` entity's `ParticlesScenario.count` and
`interaction`, and the `particles` entity's `ParticleSystem.capacity` when the
count exceeds 200,000; run `bin/Release/particles --project <copy> --vsync off
--quit-after 12`. The sample's real scene stays untouched.

## 8. Behavior covered by tests today

In `tests/runtime/ParticleTests.cpp` and `SceneTests.cpp`; replace them with your
own as you see fit, but the behaviors they pin are what scenes and samples rely on:

- emission rate, aging and expiry; capacity; disabled affectors not running;
  several spawners feeding one pool;
- the layout's size and the half float's rounding; a particle's look from its
  system or its spawner, kept for life;
- staggered births spaced along their path with matching ages; `jitter`'s
  spread; a dense interacting stream calmed by `jitter`;
- each emission shape and direction mode;
- circular orbits keeping their radius; interaction independent of particle
  order; kill on contact only with the flag, growing with the body's scale;
- scene attraction (enabled, scale, reversed), scene interaction, and their Add
  component guard;
- every particle component round-tripping through a scene; a scene's spawner
  emitting once per step with the step installed twice.

## 9. Where to look

| File | What |
|---|---|
| `engine/include/pixie/Particle.hpp` | `Particle`, `ParticleShape`, `ToHalf`, `FromHalf`, `MakeParticle` |
| `engine/src/ParticleRenderer.cpp` | vertex format, shaders, per-system uniforms |
| `runtime/include/pixie/runtime/ParticleSystem.hpp` | the pool, `Emit` |
| `runtime/src/ParticleSpawner.cpp` | emission shapes, direction, staggered births, `jitter` |
| `runtime/src/ParticleStep.cpp` | the step and the scene affectors |
| `runtime/src/AttractorAffector.cpp`, `InteractionAffector.cpp` | the two affectors |
| `runtime/src/RuntimeComponents.cpp` | the particle descriptors |
| `samples/particles/` | the sample, its scene, and the benchmark's starting point |
