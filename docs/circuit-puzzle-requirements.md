# Circuit Puzzle Consumer Requirements

**Document ID:** GLOBAL-CIRCUIT-PUZZLE-REQUIREMENTS

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** consumer design requirements informing engine direction

**Surface:** guide

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## Scope and status

This document owns the consumer requirements analysis used as input to engine
planning. It distinguishes owner-confirmed direction from proposed acceptance
scenarios, priorities and performance references. It is not an engine technical
contract or work scheduler. [ROADMAP.md](../ROADMAP.md) owns engine direction;
[TODO.md](../TODO.md) owns promoted execution.

The existing LogicSandbox is the circuit consumer. Any remaining repository
question in the analysis concerns a separate side-scroller project, not the
already-resolved replacement of the older sandbox. Delivery status of extracted
engine facilities belongs in the scheduler and completion records.

The [body dynamics design](rigid-body-physics-requirements.md) expands physics
ownership, character/platform interactions, editor workflow and proposed acceptance
gates. It is design input, not a claim that the current movement foundation meets
all requirements below.

The analysis below retains its 2026-10-22 design baseline. It was imported from
the same-date circuit-puzzle handoff; no external workspace is needed to read it.
Original source SHA-256:
`926ae3f257f3b5cafdc81b334ca204401d3c9a0db10f5d8ef87a05f01cf700c3`.

---

# 2D side-scrolling circuit puzzle game requirements

Date: 2026-10-22. Status: design handoff; no new game implementation authorized or performed here.

## 1. Confirmed direction and proposed scope

Confirmed by the owner:

- A 2D side-scrolling world that the player explores and interacts with.
- World interactions can involve power cables, doors, bridges and other mechanisms.
- Circuit panels open for fine tuning, repairs and puzzle solving.
- Atmosphere, lighting and fluid movement must be excellent.

Proposed design: observe a world problem, inspect its panel, modify a real circuit, then verify the
result in the world. Reasoning is the main challenge; ordinary execution of a known solution should
feel responsive and forgiving. This is a proposed quality direction, not a finalized genre contract.

The existing LogicSandbox is a source of reusable simulation/editor behavior, not the finished game
interface. This concept does not implicitly include the separate tower/base-defense idea. Combat,
survival, crafting, procedural levels, pixel-perfect jumps and a specific art style are not confirmed.

Priority notation: P0 is required for the first representative vertical slice; P1 is for progression
after the slice proves the loop. Optional features are explicitly conditional.

## 2. Functional requirements

| ID | Priority | Requirement and observable result |
|---|---|---|
| G01 | P0 | The player can traverse a small connected environment, reach mechanisms and interact without fighting the controls |
| G02 | P0 | A reachable panel opens a readable circuit workspace and closes back into the same world situation, without losing edits or duplicating an interaction |
| G03 | P0 | Sensors, switches, circuit ports and actuators have explicit bindings: the circuit state actually drives the door/bridge/elevator, with visible feedback |
| G04 | P0 | World feedback distinguishes a command from physical completion: a door may be commanded open while still moving or obstructed; its sensor reports the defined physical state |
| G05 | P0 | Circuit inspection/editing supports selecting, connecting, disconnecting, moving and examining the permitted pieces, with visible signal state and a consistent timing model |
| G06 | P0 | Reset/undo or a clear local recovery action makes experimentation safe; recovery restores the relevant circuit, mechanism and player state consistently |
| G07 | P0 | Opening panels has a defined input/time policy. Panel clicks must not move the character or trigger actions behind the panel; closing cannot leak a held jump or interaction |
| G08 | P0 | Power cable sources, compatible sockets, reach and connection state are understandable. If removable cable reuse is in the slice, disconnection has a consistent effect and cannot strand progress permanently |
| G09 | P0 | Checkpoint/reload preserves the intended puzzle, connection and actuator state, and places the player safely; transient UI highlights/layout changes are excluded |
| G10 | P0 | Offscreen/hidden panels keep the simulation activity required by their contract. Camera visibility and panel expansion do not change puzzle outcomes |
| G11 | P1 | Reusable devices and nested circuits support authoring without forcing the player to inspect every internal gate |
| G12 | P1 | Observation tools and optional hints help locate missing power, wrong signals or blocked mechanisms without immediately giving the solution |

G03 must use the same authoritative simulation as the panel. A decorative circuit that merely forwards
unrelated numeric commands does not satisfy it. Define command, power and sensor meanings separately;
do not silently assume that a Boolean logic signal models electrical current or power capacity.

Recommended G07 default for the first slice: freeze player locomotion while a safely accessed panel
is open, keep the world/circuit live, and show the relevant mechanism or another immediate feedback
channel. World pausing, single-step diagnostic mode and unsafe mid-action panel use remain decisions
to review. If pause is chosen, specify exactly which clocks and simulations pause and how they resume.

## 3. Atmosphere: quality-critical requirements

| ID | Priority | Requirement | Review evidence |
|---|---|---|---|
| A01 | P0 | A room has intentional foreground, playable space and background composition; traversable surfaces, panels and destinations remain distinguishable | Owner review of stills and traversal footage at gameplay camera scale |
| A02 | P0 | Ambient sound and local machine/interaction sounds respond coherently to power and mechanism changes, with smooth fades and no audible seams | Headphone/speaker recording of power loss, restoration, movement and panel interaction |
| A03 | P0 | Motion, particles, sound and light reinforce the same world state. Decorative movement has a clear visual hierarchy and does not obscure circuit or traversal cues | Playtest identifying which machine is active and where attention is needed |
| A04 | P0 | Opening a panel preserves a sense of place through framing, background visibility, spatial sound or a deliberate transition | Side-by-side world/panel transition review; feedback remains available during edits |
| A05 | P0 | Screen shake, flashes, heavy effects and sound levels have practical controls; reduced effects retain necessary information | Review with effects reduced and sound muted |
| A06 | P1 | Environmental details and changes suggest what mechanisms do before explicit explanation | First-time player observation rather than a developer explaining the scene |

Atmosphere is an authored experience supported by rendering, audio and camera tools. A particle system
or a dark color palette alone is not acceptance. Art direction, mood and sound references still need
owner selection; an industrial setting is an example, not a confirmed theme.

## 4. Lighting: quality-critical requirements

| ID | Priority | Requirement | Acceptance scenario |
|---|---|---|---|
| L01 | P0 | Local lights and ambient illumination produce coherent depth while keeping player silhouettes, floor edges, sockets and panel text readable | Traverse the same room powered, unpowered and with reduced effects |
| L02 | P0 | Walls and appropriate foreground objects occlude light; there is no distracting leakage through closed geometry or flickering at corners | Move a light/camera across wall corners and open/close an occluding door |
| L03 | P0 | Powered lamps/emissive indicators respond to actual simulation state; light presentation can fade while the underlying logical state remains unambiguous | Record command, lamp state and visual transition together |
| L04 | P0 | Lights, shadows, sprites, text and panel overlays compose in a stable order without seams, camera jitter or UI contamination | Pan/zoom at supported resolutions and open panels near bright sources |
| L05 | P0 | Light count, shadow cost, overdraw and quality settings have declared budgets and useful telemetry | Repeat a named stress scene with scene counts and CPU/GPU costs recorded separately |
| L06 | P1 | Optional bloom, normal maps or fog improve the chosen art direction and have scalable costs | A/B review justifies the effect; these techniques are not prerequisites by themselves |

No requirement mandates a particular lighting technique. Prefer the smallest solution that passes
the room's visual and performance review. Darkness must not replace puzzle reasoning with searching
for invisible interactables. Color alone must not communicate power or fault state.

## 5. Fluid movement: quality-critical requirements

| ID | Priority | Requirement | Acceptance scenario |
|---|---|---|---|
| M01 | P0 | Acceleration, braking and reversal are tunable and responsive; idle has no unintended drift | Repeated starts/stops, reversals and short positioning tasks near a panel |
| M02 | P0 | Movement uses a defined fixed simulation step and appropriate rendering interpolation; rendering rate does not change traversal reach or collision results | Replay the same scripted input at 30, 60 and 144 render FPS |
| M03 | P0 | Collision handling does not snag on tile seams, permit tunneling at the supported speed or create persistent penetration | Seam corridor, wall/corner approach, low ceiling and high-speed supported movement cases |
| M04 | P0 | Mechanisms that carry or obstruct the player handle contact reliably | Stand on, ride and leave an elevator/bridge; move into an opening/closing door |
| M05 | P0 | Camera following is smooth but responsive, respects room bounds and keeps necessary destination/context visible | Direction reversals, vertical traversal if present, room transitions and moving-platform footage |
| M06 | P0 | Locomotion animation matches movement and interaction; panel transitions do not cause snapping, stuck states or competing control owners | Repeated approach/open/edit/close/leave loop with held and released inputs |
| M07 | Conditional P0 | If jumping is adopted, use tunable jump height, input buffering and coyote time, with predictable head/landing contacts | Early/late jump input, edge departure, low ceilings and platform landing tests plus playtest |
| M08 | P0 | Pause, focus loss and input remapping cannot leave movement or interaction stuck | Alt-tab while moving/interacting, release input, then resume |

Jumping, slopes, ladders, crouching, one-way platforms and a dash are not mandatory abilities. Select
only what the first puzzle needs; add a corresponding collision/feel test for any adopted ability.
If jumping is selected, approximately 80-120 ms coyote time and 100-150 ms buffering are starting
tuning proposals, not approved or universal values. Automatic traversal can also fit this concept.

Acceptance requires both repeatable tests and the owner/new players finding the character pleasant to
control. A controller that passes collision tests but feels sluggish or imprecise is incomplete.

## 6. Puzzle design and readability

- P0: Each introductory puzzle teaches one relation, provides immediate observable feedback, and
  allows experimentation without irreversible failure. Combine concepts after they have been used.
- P0: Circuit complexity follows the mechanism. Start with a few meaningful components, not the
  full calculator or a dense unexplained board. Panel size and allowed components are level data.
- P0: Teach the link between physical exploration and circuit reasoning. At least one puzzle must
  require observing a world condition rather than only solving an isolated gate worksheet.
- P0: Validate actual solvability from checkpoint/reset states, unintended bypasses, softlocks and
  whether alternative solutions satisfy the goal. Decide deliberately which alternatives are valid.
- P0: A solved puzzle changes access or behavior meaningfully, and the world communicates success.
- P1: Repairs, adaptations and construction tasks supply variety; not every room needs all three.

Example only: an elevator runs while a button is held. The player needs to board it. A small panel
allows a latched command that resets on the arrival sensor. It requires a defined latch, sensor and
actuator contract; it is a design candidate, not an already approved level or proof of solvability.

## 7. Performance, persistence and authoring

Proposed initial performance reference: 1920x1080 at 60 FPS on the recorded RX 7600 machine, with
its full CPU/RAM/build/driver details captured when testing. This is a proposed reference, not a
minimum specification or an approved budget. Agree on scene density and hardware before acceptance.

- Record frame-time median/p95/p99 and hitch count, not just average FPS. A 60 FPS target gives a
  16.67 ms frame budget; propose and review subsystem budgets once the representative slice exists.
- Separate circuit simulation, character/collision, world updates, rendering, lighting, panel work,
  audio and telemetry costs. Report loading separately from steady-state samples. Do not force GPU
  synchronization into the ordinary player just to measure it.
- Bound lights, shadow geometry, particles, active mechanisms, circuit entities and visible wires in
  every benchmark. Include a dark-room power transition and repeated panel open/close cycles.
- Preload or stage resources so approaching/opening a panel does not cause a recurring visible hitch.
  Check state/memory stability across repeated room, panel and checkpoint cycles.
- Author geometry, collision, lights/occluders, camera limits, sound zones, panels, port bindings,
  mechanism sensors and checkpoints as inspectable data. Provide views that diagnose broken bindings.
- Preserve the original sandbox saves and package versions. Reuse simulation with explicit timing
  contracts and regression traces; do not assume open/sealed chips have identical timing.

## 8. Proposed vertical slice and review order

1. **Movement room:** a greybox corridor with a moving platform and reachable panel location.
   Validate collision, input, camera and feel before a large content pipeline.
2. **Atmosphere room:** establish a representative powered/unpowered look and soundscape in that
   same space. Validate readability, lighting transitions and the owner's quality bar early.
3. **World/panel loop:** two connected spaces, one repair panel and one adaptation panel controlling
   two mechanisms such as a door and elevator/bridge. Include cable reuse only if it is chosen for
   this slice. Show the consequence of edits while inspecting a panel.
4. **Robustness pass:** reset, save/reload, input capture, repeated expansion/closing, obstruction,
   alternate puzzle solutions and first-time playtest. Then profile the named stress variant.

The slice is accepted through functional evidence, a recorded representative run, owner visual/feel
review and at least a small first-time playtest. Automated checks do not establish atmosphere or fun.

## 9. Decisions to settle during slice planning

- Target mood/art style, sound direction and visual references.
- Walking-only traversal versus jumping/other abilities; keyboard/mouse first versus gamepad parity.
- Panel time policy and safe interaction conditions; camera versus panel split/framing.
- Whether cable handling is a resource puzzle, a routing interaction, or both; power versus logic rules.
- Target machines, resolution, scene density, effect budget and accessibility options for the slice.
- New-game repository/location and which simulator/editor components are shared libraries or consumers.

These are explicit open decisions, not reasons to invent a full game scope or silently start building
one. Engine promotion decisions and game design decisions should reference each other while remaining
owned by their respective projects.
