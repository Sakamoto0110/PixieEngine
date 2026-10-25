# Body Dynamics Laboratory

**Document ID:** SAMPLE-PHYSICS-LAB-README

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** initial no-code room for rigid body rotation, contacts and world gravity

**Surface:** introduction

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

Open this folder in the editor, or run from the engine repository:

```text
editor/PixieEditor/bin/Release/net10.0/PixieEditor.exe --project samples/physics-lab
```

Press Play to drop the orange box onto the ramp, settle four green boxes and
move the purple kinematic platform. Stop restores the authored room. Select
world physics to change gravity; select a box for RigidBody2D and Collider2D
properties. Copy the folder before experimenting. There is no project C++ code.

The generic native player uses the same service:

```text
bin/Release/pixie-player.exe --project samples/physics-lab
```

This is an initial D1 demonstration, not the complete stress/acceptance room.
It deliberately has no character: box pushing, platform carrying and safe
crushing behavior remain separate implementation gates. The platform has
constant velocity, not a finished elevator controller. See the
[body dynamics guide](../../docs/body-dynamics.md) for ownership, units,
compatibility and remaining editor workflow limits.
