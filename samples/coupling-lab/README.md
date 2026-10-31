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

Use **A/D or the arrows** to move the yellow player (E) and **Space** to jump. The panel at
the top lists every character: what supports it (air, static, kinematic or dynamic), its
completed world velocity, the motion carried by its support this tick, the push impulse and
support load it applied this tick, and its placement status.

| Zone | What it shows |
|---|---|
| A, translating platform | A rides a platform that reverses every 2.4 s. The post stops the rider, not the platform: **carry obstructed**, then the rider is left behind and drops. |
| B, rotating platform | B stands off center on a platform that turns 23° in 2 s and turns back, so its support point also moves sideways. |
| C, elevator | C rides up and down 100 units; jump from it to feel the inherited velocity. |
| D, crusher | The platform rises into a ceiling: **UNSAFE placement** while there is no room, and it clears when the platform reverses. The engine reports the failure; it does not stop the platform, damage or respawn D. |
| E, player | Push the 1 kg, 10 kg and compound boxes, stand on them (support load), jump against them and walk onto the platforms. The ram at the right shoves the 2 kg box back toward you. |

The platforms and the ram are kinematic bodies with a `Shuttle` component: the sample's code
reverses their velocity every period of simulated time. That reversal is game policy, here in
the sample, not an engine feature. Select one in the editor to change its velocity, angular
velocity or period; select a character for its `CharacterMotor2D` tuning, including the push,
load, inherited-speed and carry limits. Stop restores the authored room. Copy the folder before
saving experiments.

## Owner review

This room prepares the owner's movement-feel and debug-view review for PIXIE-TODO-043; it does
not record that review. Things to judge while playing:

1. Riding A, B and C: does the rider stay planted, without sliding, jitter or a lag behind
   the platform, including at each reversal?
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

Native checks of the same mechanics are in
[CharacterCouplingTests.cpp](../../tests/runtime/CharacterCouplingTests.cpp) and the mixed
`coupled` workload; they do not replace this review.
