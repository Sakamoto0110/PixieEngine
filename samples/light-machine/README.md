# Light Machine

**Document ID:** SAMPLE-LIGHT-MACHINE-README

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** no-code lighting room 3: a machine with emissive indicators, sparks that glow and light the floor, and dust that does neither

**Surface:** introduction

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

Open this folder in the editor, or run from the engine repository:

```text
editor/PixieEditor/bin/Release/net10.0/PixieEditor.exe --project samples/light-machine
bin/Release/pixie-player.exe --project samples/light-machine
```

Room 3 of the [lighting work](../../docs/lighting-requirements.md#10-proposed-delivery-gates-and-rooms):
a dark room with a machine. Press Play; A and D move the character, Space jumps.

- The cabinet's indicators are shapes with `Emission2D`: a powered green lamp and a busy amber one
  at full intensity, and an unpowered red one faded to 0.15, beside a glowing status strip. They show
  in darkness, unlit.
- A grinder throws sparks from a pool with `ParticleLight2D`: emissive, so they glow at their own
  color, and illuminating, so each lights the floor and the cabinet around it, unshadowed, at most
  160 at a time.
- Dust drifts from the ceiling in a pool without `ParticleLight2D`: it neither glows nor lights.
- A ceiling spot lights the right side, and the cabinet and the character cast its shadows.

The cabinet is a sprite, drawn before the shapes, so the lamp shapes show in front of it. Powering
the machine on and off needs game code, which this room has none of: the fade of a lamp with its
logical state is covered by the `EmissionFollowsItsIntensity` case of `lighting-tests`. Look and
atmosphere have not been reviewed by the owner.
