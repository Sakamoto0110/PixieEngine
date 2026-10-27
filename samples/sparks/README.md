# Dynamic Sparks Laboratory

**Document ID:** SAMPLE-SPARKS-README

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** click sparks, player-pushed floor sparks and R room reset

**Surface:** introduction

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

Run `bin/Release/sparks.exe`, or open this folder in the editor. The sample has
project code; build it with the engine, or let the editor build the project.

- Right click empty space to fire a burst. Sparks fall under world gravity and
  bounce from the floor and box using native circle sweeps.
- Use **A/D or arrows** to move the yellow player and **Space** to jump. Push
  the blue dynamic box. All boxes emit sparks on native impact/sliding contacts.
- Press **R** to clear all sparks, return player and box to their starting poses, clear
  their motion and restart the random sequence. This resets room contents; it does not
  reset the host's elapsed-time or telemetry counters.

The title bar repeats the controls. The pool's ParticlePhysics2D exposes gravity
scale, collision radius, bounce, friction, bounce threshold and collision filter.
The floor's material participates in contact response. Click burst count is in
SparksScenario. Particle appearance stays fixed at birth. The 40-byte layout is
unchanged and no particle owns a rigid body.

The box is a dynamic rigid body pushed through the native character motor;
left-dragging applies a bounded native spring force at the grabbed point. The player uses swept capsule movement while dragged. Query-only character capsules do not block particles
in this first increment. The demonstration is a first bounded particle dynamics
case, not a 100,000-dynamic-particle performance claim. See the
[algorithm and limits](../../docs/particle-dynamics.md).

## Rotating rectangle and mass test

Three green rectangles have unequal extents, nonzero initial angles and freely
rotating native rigid bodies (1, 2 and 3 kg). They fall, tip and settle against the
same world as the player and the original box. The original blue push box retains
its rotation lock for comparison.

Left click/drag a dynamic box to select it; the single **Mass (kg)** textbox
shows that body's backend mass. Click the field, type a decimal number, then
press Enter to apply. The initial value is selected for replacement; Backspace,
Ctrl+A and Escape are supported, and comma is accepted as the decimal point.
The valid test range is 0.01 to 1000 kg. Invalid/empty input leaves the body
unchanged. Editing suppresses player keyboard movement. Mass changes recalculate
native inertia while preserving the body's velocity. R outside the field restores
every box's authored pose, rotation and mass. The field is for mass, not force in
newtons; world gravity is unchanged.

## Ramp and box-contact room

Two ramps, a raised drop platform and two orange stack/drop boxes extend the
room. Lift a box with the left button and release it over a ramp or another box.
Emission reads native manifold points and impulses, independently of entity name.
An increase in normal impulse creates a mass-weighted impact burst; actual relative tangential
speed under friction creates sliding sparks. Steady sleeping stacks do not emit.
Births probe near contact edges to avoid starting inside solids. FX emission is
bounded to 32 particles per contact point and 256 per tick into the same pool.

Particles stay lightweight circle queries, never rigid bodies. Their collision
pass is bounded separately from the native contact emitter. The emitter has its
own sample.sparks.contacts timing and contact-point/emitted gauges, separate from
particles.physics. Dynamic population has no new simulation cap or 100k claim.
Standalone particle-only Worlds no longer install scene movement. Invalid physical
pool settings surface native per-entity diagnostics and fall back ballistically
until fixed, so an Inspector edit cannot leave World::IsStepping stuck true.
