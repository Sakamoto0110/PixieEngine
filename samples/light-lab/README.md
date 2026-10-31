# Light Laboratory

**Document ID:** SAMPLE-LIGHT-LAB-README

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** no-code lighting rooms 1 and 2: a dark room with a movable lamp, and several lights over moving occluders

**Surface:** introduction

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

Open this folder in the editor, or run from the engine repository:

```text
editor/PixieEditor/bin/Release/net10.0/PixieEditor.exe --project samples/light-lab
bin/Release/pixie-player.exe --project samples/light-lab
```

Two rooms of the [lighting work](../../docs/lighting-requirements.md#10-proposed-delivery-gates-and-rooms)
stand side by side, behind a wall with a doorway at the floor. Press Play; A and D move the
character, Space jumps.

- **Room 1, left:** a dark room lit by one movable light, a lamp box the character pushes, with a
  pillar and a block as static occluders and a crate as a physical box. The character casts a
  shadow through its capsule (`Occluder2D` on it).
- **Room 2, right:** a ceiling spot and two colored point lights over a rotating platform, a
  spinning bar and a box that falls onto the ramp, all casting soft shadows at their interpolated
  poses.

`LightingSettings2D` on the "lighting" entity sets a low ambient, exposure and the Aces curve.
Walls, floor and platforms are occluders cast from their colliders; the back walls are sprites
that receive the light. There is no project C++ code. Room 2 has no door: a door that opens and
closes needs game code, so its shadow under motion is covered by the interpolated-pose test of
`lighting-tests` instead. Look and atmosphere have not been reviewed by the owner.
