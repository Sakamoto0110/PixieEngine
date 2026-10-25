# Movement Room

**Document ID:** SAMPLE-MOVEMENT-README

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** neutral native and headless collision/controller demonstration

**Surface:** introduction

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

Build target `movement-demo`. Run `bin/Release/movement-demo.exe`:
A/D or arrows move, Space requests a jump, R resets, F3 shows existing native
metrics. Green means grounded; amber means airborne. The capsule moves over a
floor and ramp, against a thin wall and below a ceiling. This is a mechanical
test room, not the game's atmosphere or an accepted movement-feel baseline.

`movement-demo --headless on --autoplay on` runs 600 fixed updates without a
window and writes CPU metrics/state gauges as CSV to stdout. Final position
and grounding are written to stderr. `--telemetry off` disables collection.

For repeatable native observation use `--autoplay on --quit-after 5 --screenshot
movement.png`. Screenshot and interactive input evidence must be reported
separately: autoplay does not establish a keyboard/mouse playtest.

See the [movement guide](../../docs/collision-movement.md) for ownership, units,
integration and the explicit limitations. The sample owns its collision world
and controller; it does not add a reflected component or exercise editor authoring.
