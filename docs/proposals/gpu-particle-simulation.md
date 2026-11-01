# Particle Simulation on the GPU

**Document ID:** GLOBAL-PROPOSAL-GPU-PARTICLE-SIMULATION

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** moving the particle simulation (spawning, stepping, affectors and dynamic-particle collision) from the CPU to the GPU, for counts the CPU cannot step within a frame

**Surface:** proposal

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## PIXIE-PROPOSAL-015

**State:** idea

**Idea:** Particles are simulated on the CPU and only drawn on the GPU. Spawners
(`runtime/src/ParticleSpawner.cpp`) create them, the step and affectors
(`runtime/src/ParticleStep.cpp`) evolve them in the 40-byte layout delivered by
PIXIE-TODO-047, and dynamic particles collide with the scene's colliders through
`CollisionWorld2D::CircleCast` (`runtime/src/ParticlePhysics2D.cpp`). Light-casting pools are
gathered on the CPU into point lights each frame (PIXIE-TODO-052). A GPU path would keep the pool
in GPU memory and step it with compute shaders (OpenGL 4.5 core has them, and so does Mesa
llvmpipe), so the CPU no longer touches each particle and drawing reads the pool in place.

**Rationale:** the gain appears only at high counts, roughly hundreds of thousands of particles,
where stepping on the CPU no longer fits a frame. At today's counts it is not a bottleneck: with
2000 light-casting particles on the owner's RX 7600 at 1920 × 1080, gathering their lights costs
about 0.13 ms of CPU, and moving only that gathering to the GPU would save little, since the
0.33 ms of GPU light accumulation stays the same. No measurement of the simulation's own cost at
high counts exists yet; that measurement is the first step, and it decides whether this idea is
worth pursuing.

**Constraints:**

- Dynamic particles collide with colliders that live in the CPU collision world (Box2D-backed).
  The GPU needs a copy of the collision geometry kept in sync each step, or collision results
  read back, or dynamic particles stay on the CPU while static ones move.
- Fixed-step determinism and seeded captures must hold, including on llvmpipe in CI; GPU float
  and ordering differences must not make captures unrepeatable.
- The development channel, telemetry (counts, collision deaths) and game code read particle state;
  reading it back from the GPU costs a sync, so what stays readable must be chosen.
- Light-casting pools would then gather their lights on the GPU too.
- Native engine code never depends on the editor; C API changes follow the usual versioning.

**Promotion target:** owner decision required

**Source:** owner request on 2026-10-31, after the PIXIE-TODO-052 discussion of where particle light is gathered
