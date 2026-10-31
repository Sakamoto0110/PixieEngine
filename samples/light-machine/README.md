# Light Machine

**Document ID:** SAMPLE-LIGHT-MACHINE-README

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** lighting rooms 3 and 4: a machine with emissive indicators that follow its power, sparks that glow and light the floor, dust that does neither, and the optional effects

**Surface:** introduction

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

Run the player, or open this folder in the editor and press Play:

```text
bin/Release/light-machine.exe
editor/PixieEditor/bin/Release/net10.0/PixieEditor.exe --project samples/light-machine
```

Room 3 of the [lighting work](../../docs/lighting-requirements.md#10-proposed-delivery-gates-and-rooms):
a dark room with a machine. A and D move the character, Space jumps, **P** switches the machine's
power and **F** turns the optional effects on and off.

- The cabinet's indicators are shapes with `Emission2D`: a powered green lamp and a busy amber one
  at full intensity, and an unpowered red one faded to 0.15, beside a glowing status strip. They show
  in darkness, unlit.
- A grinder throws sparks from a pool with `ParticleLight2D`: emissive, so they glow at their own
  color, and illuminating, so each lights the floor and the cabinet around it, unshadowed, at most
  160 at a time.
- Dust drifts from the ceiling in a pool without `ParticleLight2D`: it neither glows nor lights.
- A ceiling spot lights the right side, and the cabinet and the character cast its shadows.

The cabinet is a sprite, drawn before the shapes, so the lamp shapes show in front of it.

The power is the room's game code (`src/LightMachineProject.cpp`). `PowerSwitch2D`, the room's scenario on
the `scenario` entity, holds the logical state, which P switches at once; each `PoweredLamp2D` moves its `Emission2D`'s
intensity toward its on or off value at a constant rate, so the fade ends `fadeSeconds` after the
switch, and the faulty red lamp lights up when the power goes; `PoweredSpawner2D` stops the sparks.
The panel at the top shows the logical state, "(lamps fading)" while any lamp has not reached it,
and each lamp's intensity; the gauges `machine.powered` and `machine.lamp_intensity` record the
same in telemetry.

Room 4 is this room with the panel open and **F** pressed: ambient occlusion, height fog and bloom
on `LightingSettings2D`, all off in the authored scene. Their cost is measured by the
`lights-16-effects` room of `lighting-budget`. Look and atmosphere have not been reviewed by the
owner.
