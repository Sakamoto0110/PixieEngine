# Character/Body Coupling Laboratory

**Document ID:** SAMPLE-COUPLING-LAB-README

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** review room for platform carrying, pushing, support load, obstruction and crushing

**Surface:** introduction

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

The room for reviewing the [character/body coupling](../../docs/character-body-coupling.md)
by playing it. Run the player with native debug outlines, or open this folder in the editor
and press Play:

```text
bin/Release/coupling-lab.exe --physics-debug 127
editor/PixieEditor/bin/Release/net10.0/PixieEditor.exe --project samples/coupling-lab
```

Use **A/D or the arrows** to move the yellow player (E) and **Space** to jump. **R** resets the
room: bodies, characters, platforms, particles and dropped objects. **Left drag** pulls a box or
the ball with a bounded spring; **right click** drops a runtime object, chosen with **1** (1 kg
box), **2** (10 kg box), **3** (compound box) or **4** (1 kg ball). Dropped objects are never
saved. The panel at the top shows the controls and lists every character: what supports it (air, static, kinematic or dynamic), its
completed world velocity, the motion carried by its support this tick, the push impulse and
support load it applied this tick, and its placement status.

| Zone | What it shows |
|---|---|
| A, translating platform | A rides a platform that reverses every 2.4 s. The post stops the rider, not the platform: **carry obstructed**, then the rider is left behind and drops. |
| B, rotating platform | B stands off center on a platform that turns 23° in 2 s and turns back. Within the 45° walkable limit the rider stays put on the platform while its support point moves sideways. |
| F, steep platform | Upper left: the platform turns to 55°, past the walkable limit, so F slides off. |
| Ramps | The left ramp (25°) is walkable; the right ramp (55°) is not, so the player slides back down it. |
| C, elevator | C rides up and down 100 units; jump from it to feel the inherited velocity. |
| D, crusher | The platform rises into a ceiling and crushes D: **UNSAFE placement**. Its `reverseOnCrush` sends it back at once (game policy). Turn it off in the editor to see the engine's own behavior: it keeps the last valid pose and reports the failure, and the platform moves on through the capsule. |
| E, player | Push the ball and the 1 kg, 10 kg and compound boxes, stand on them (support load), jump against them and walk onto the platforms. The ram at the right shoves the 2 kg box into a stack of four boxes. Boxes thrown, dragged or shoved at a character stop against it and push it aside, within its push speed limit. |
| Particles | Dynamic particles rain on B and C and bounce off the bodies, the platforms and the characters. The rain over the stack and the ram comes from a second pool whose collision layers leave out "characters", so it falls through them. |

The platforms and the ram are kinematic bodies with a `Shuttle` component: the sample's code
reverses their velocity every period of simulated time, and, with `reverseOnCrush`, as soon as a
character it was carrying reports unsafe placement. Both are game policy, here in the sample, not
engine features. Select one in the editor to change its velocity, angular velocity, period or
crush policy; select a character for its `CharacterMotor2D` tuning, including the push,
load, inherited-speed and carry limits. Stop restores the authored room. Copy the folder before
saving experiments.

## Owner review

The owner reviewed this room for PIXIE-TODO-043 and accepted the physics delivery on
2026-10-31 (see its [completion record](../../docs/archive/todo-043-rigid-body-dynamics-2026-10-31.md)).
The list below is what that review judged; use it again when coupling behavior changes:

1. Riding A, B and C: does the rider stay planted, without sliding, jitter or a lag behind
   the platform, including at each reversal? And on the ramps: standing still on 25°, sliding
   off 55° and off F's platform.
2. Jumping from C while it rises and while it sinks, and from A while it moves: does the
   inherited velocity feel right, and is it applied once?
3. Walking off a platform and landing back on it: no stale carry after leaving.
4. Pushing the light, heavy and compound boxes: does the difference in mass read clearly,
   and does the push cap feel too strong or too weak? Jumping against a box must not launch it.
5. Standing on a box, and being hit by the box the ram shoves: no catapult, no persistent overlap.
6. D's crush and A's obstruction: are the reported statuses the ones a game would want to
   react to?
7. With `--physics-debug 127`: are outlines, centers, velocities, sleep state and contact
   normals readable, and do they match what moves?
8. Dragging and dropping objects, and the particles: do bodies and particles react as expected
   to the platforms, the ram and each other?

Native checks of the same mechanics are in
[CharacterCouplingTests.cpp](../../tests/runtime/CharacterCouplingTests.cpp) and the mixed
`coupled` workload; they do not replace this review.
