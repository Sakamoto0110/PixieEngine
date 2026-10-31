# Light Laboratory

**Document ID:** SAMPLE-LIGHT-LAB-README

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** lighting rooms 1 and 2: a dark room with a movable lamp, and several lights over moving occluders and a sliding door

**Surface:** introduction

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

Run the player, or open this folder in the editor and press Play:

```text
bin/Release/light-lab.exe
editor/PixieEditor/bin/Release/net10.0/PixieEditor.exe --project samples/light-lab
```

Two rooms of the [lighting work](../../docs/lighting-requirements.md#10-proposed-delivery-gates-and-rooms)
stand side by side, behind a wall with a doorway at the floor. Press Play; A and D move the
character, Space jumps.

- **Room 1, left:** a dark room lit by one movable light, a lamp box the character pushes, with a
  pillar and a block as static occluders and a crate as a physical box. The character casts a
  shadow through its capsule (`Occluder2D` on it).
- **Room 2, right:** a ceiling spot and two colored point lights over a rotating platform, a
  spinning bar, a box that falls onto the ramp and a door that slides up and down, all casting soft
  shadows at their interpolated poses.

`LightingSettings2D` on the "lighting" entity sets a low ambient, exposure and the Aces curve.
Walls, floor and platforms are occluders cast from their colliders; the back walls are sprites
that receive the light. The door is the room's only game code: `Door2D` (`src/LightLabProject.cpp`)
drives a kinematic body whose velocity reverses every `period` seconds of simulated time, and the
door is an `Occluder2D` like any other; `LightLabScenario` on the `scenario` entity scales the
doors' speed. Look and atmosphere have not been reviewed by the owner.
