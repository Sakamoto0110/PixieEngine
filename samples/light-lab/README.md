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

Run the native player:

```text
bin/Release/light-lab.exe
```

This sample's C++ code uses the Engine source build. In [independent PixieEditor](../../editor/README.md),
choose **Open without code** to inspect the authored scene. Use the native
sample player to exercise its game code.

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
doors' speed. The owner reviewed and accepted this room on 2026-10-31.

## Shared-engine acceptance scenario

TODO-051 uses this authored room for a real editor/MCP workflow. The official-client
qualification opens the project, selects `cyan light`, changes `Light2D.intensity`
through the Inspector, checks native storage and one history entry, and captures
the changed viewport. Undo restores the intensity and the viewport within isolated
one-channel-level raster rounding (at most 0.001% of the compared pixels);
redo reapplies the edit. Play/pause followed by 120 additional fixed steps and Stop
exercise the room's game module without writing the authored scene.

The original run used the nested source Editor's default 1600x900 layout and
compared the scene rectangle (360,250)-(1150,700). It also verified restoration
of the authored state. For current cross-product checks, use the
[explicit-artifact MCP driver](../../tests/mcp/qualify_mcp.py) with an admitted
SDK project and disposable capture paths.

The approved player snapshot in `snapshots/scenarios.json` fixes the total
at 120 steps, seed 1 and 960x540 pixels. On the qualified machine, two captures were
identical on the RX 7600 and two were identical on Mesa 26.2.4 llvmpipe. Across those
renderers the maximum channel difference was 1, affecting 12.83% of pixels. The
approved tolerance permits a difference of 1 in up to 15% of pixels; it is specific
to this scenario. On 2026-10-31 the owner approved the presented llvmpipe image and
this tolerance to establish the concrete TODO-051 regression baseline. The exact
approved image is `snapshots/rooms-running.png`, SHA-256
`d64e5bfb536dac2ab821e9f4c31772352c7255f3b39caef3edb4277e9ba1e39d`.
The manifest registers `snapshot-light-lab-rooms-running` in CTest. Room appearance
acceptance and this reference/tolerance approval were separate decisions.
