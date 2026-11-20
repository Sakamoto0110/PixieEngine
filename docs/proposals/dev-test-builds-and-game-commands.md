# Private Test Builds and Game Commands over MCP

**Document ID:** PIXIE-PROPOSAL-024

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** package a playable local development build and expose project-owned player and test commands through MCP

**Surface:** proposal

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## PIXIE-PROPOSAL-024

**State:** promoted on 2026-11-12. The private CLI and MCP launch are in use
for local qualification. The disposable Weapons-Lab project implements the
26 player commands and 16 developer commands listed below. All 42 bindings
were exercised over MCP on the privately exported Windows x64 Release player.

**Idea:** Add a private test-build export path. A developer can hand a playable
package to another agent, which launches the game locally through `PixieMcp`,
discovers its player and test commands, and checks the resulting game state.

**Rationale:** The existing [Dev Channel](dev-channel.md) can step the simulation,
inject input, and inspect the World. A game can expose player actions and test
fixtures through the same connection. This gives a test agent stable controls for
repeatable scenarios and leaves mouse and keyboard injection available for
testing the HUD itself.

**Constraints:** Use the private `local-development` profile and its opt-in,
owner-only, single-client local channel. The existing [shipping export](../game-export.md)
continues to enforce its shipping profile and development-code exclusions.

**Promotion:** Workspace `PXW-ROADMAP-003` and `PXW-TODO-005`.

### Test-build handoff

The private CLI stages a standalone Windows x64 Release player, saved project,
scenes, assets, app-local native DLLs and SDK notices. `pixie-test-build.json`
records the selected private SDK catalog and build identities and the size and
SHA-256 of each staged file. The receiving machine supplies the matching
installed private SDK and a local build of `PixieMcp` separately. Only the
machine exporting the bundle needs the C++ build tools. Use bundles from a
trusted producer. The inventory checks byte consistency and SDK pairing.

Run the CLI with Python 3.11 or later, an installed private SDK with a
`local-development/Release` tuple and the matching Visual Studio 2026 x64
toolchain. For code projects, explicitly adopt that private SDK in a disposable
project copy through its SDK requirement before exporting. The CLI uses
`pixie_game` to select the player's target and builds into a temporary folder.
Choose an output directory that does not exist. The CLI includes built-in scene
asset references and CMake-declared assets. Repeat `--asset` for any additional
project-relative data or notices that the game accesses directly.

For an atlas-backed Sprite, use a private SDK that supports the
[sprite atlas contract](../modules/runtime/REFERENCE.md#sprite-atlas-contract).
The CLI stages the descriptor and its PNG. It checks the frame references,
descriptor fields used for rendering, and PNG header dimensions. The runtime
decodes the image when it loads the scene. A Sprite with both atlas fields
empty still stages its direct PNG.

```powershell
python tools/test-build/export_test_build.py --sdk $PrivateSdk --cmake $CMake `
  --asset assets/inter-license.txt $ProjectCopy $Bundle
dotnet build tools/pixie-mcp/PixieMcp.csproj -c Release -p:RestoreLockedMode=true
```

Configure the MCP client for stdio with .NET 10 and the following arguments.
Use absolute paths and choose a capture directory outside the verified bundle.
The bridge checks the inventory and SDK before launching the bundled player
with `--dev-channel`. Direct standalone launch keeps the channel closed unless
explicitly enabled. The public shipping exporter retains its existing CLI.

```powershell
dotnet $PixieMcpDll --test-build $TestBuildManifest --sdk $PrivateSdk --captures $Captures
```

### Game command registration

Projects explicitly register their commands for the active development session.
`game.player.*` runs through the same action handlers and rules as local
controls. Player queries use operator-visible state and ordinary sensor rules.
`game.dev.*` can prepare artificial test conditions and read internal state.
Use the current game model as the authority for both namespaces. Keep the HUD
and gameplay mechanics as they are. The Weapons-Lab target inventory has 26
player bindings and 16 developer bindings. The active build can expose a subset.

The private player accepts a Project that also implements `GameCommandProvider`
from `pixie/dev/GameCommands.hpp`. Register methods with `GameCommands::Add`.
Keep handlers tied to the active session and reacquire entities from its World
when called. Use `CommandArgumentError` for dynamic argument checks before any
mutation. The dispatcher adds the simulation tick, and the bridge advertises
the input and output schemas after the private test bundle handshake. The
bridge refreshes the catalog on `tools/list`. The provider can release held
controls when the channel disconnects or the World is replaced.

The disposable Weapons-Lab copy registers the game commands. The registry
supplies `game.dev.describe`. Fixture reset reloads the exported authored
scene in memory, starts paused Play at tick zero, and scopes new contact IDs
and event cursors to that fixture. Fixture patches use reflected authored
scalar fields and their declared ranges. `configure_spawner` covers live
spawn controls and one distance band per call.

Registration supplies the method name, short description, input schema and
result schema. The bridge advertises the bindings registered by the connected
game for that session. The Engine dispatches on the session owner thread and
expires registrations when the session closes or is replaced. The bridge
refreshes the available tools when the session changes. `game.dev.describe`
reports the IDs, options, units and limits supported by the running session.
Advertise `set_cadence`, `set_cycle_mode` and `set_cooling_route` when the
corresponding game control and handler are available. Until then, omit the
binding from the tool list and `describe`, along with any unavailable
observations. Publish optional result fields in the command's result schema.

The tables give named parameters inside the MCP `parameters` object, carried
as JSON-RPC `params`. Pass `{}` for a command with no parameters. IDs are
opaque strings drawn from the current session. Numeric arguments must be
finite. Validate types, required fields, enums, bounds and references before
mutation. Duplicate registrations, unknown methods and invalid parameters
produce structured errors. A gameplay refusal reports a reason and preserves
the existing control or state where the game declines the requested change.

Every game result includes `tick`, the fixed simulation tick at which its
values were observed. A query returns its observed values in `value`. Actions
use the same `value` field for applied values and observed outcomes. Their
`status` is `refused` when gameplay declines the action, `pending` when the
action was accepted and its outcome is still outstanding, or `completed` when
the action was accepted and its outcome is known. Refused and pending results
include `reason`. Every valid action request also returns an `action_id`
scoped to its session. Advance through `world.step` and use
`game.dev.read_events` to correlate events with that ID. A pending action
publishes its terminal outcome with the same ID if it settles in that session.
Reset or session replacement invalidates outstanding action IDs and cursors.
A completed input action can yield zero projectiles under the normal game
rules. The tick alone cannot identify an action when several requests share it.

#### Engine operations

Use the existing Engine commands directly for these six operations. Pause and
resume share one operation and use two methods. `world.stats`
and the existing world/entity queries can provide lifecycle readback. Capture
uses the active host's existing capability and capture directory.

| Operation | Existing MCP call | Parameters and readback |
| --- | --- | --- |
| Start Play | `world.play` | `{}`. Start through the Engine lifecycle and read session state. |
| Stop Play | `world.stop` | `{}`. Stop through the Engine lifecycle and inspect restored state as needed. |
| Pause or resume | `world.pause` or `world.resume` | `{}` for either method. Read the effective pause state and tick. |
| Step | `world.step` | `{ "count": integer }`. Step a paused Play session. Read `steps_run`, `stats.steps` and `stats.elapsed`. |
| Change time scale | `world.time_scale` | `{ "value": number }`. Read the applied `stats.time_scale`. |
| Capture | `world.capture` | `{ "name": string }`. Supply a fresh basename. Read the capture path and simulation step reported by the host. |

#### Weapons-Lab player commands

| Method | Parameters | Behavior and observation |
| --- | --- | --- |
| `game.player.get_status` | Optional `subsystem` ID | Read operator-visible modes, requested and achieved values, readiness, temperatures, energy, transmission condition, cooling distribution and limiting reasons. Include available cannon, gear and cooling-route IDs and allowed control values. |
| `game.player.list_contacts` | Optional `filter` of `local`, `radar` or `all`. Default `all`. | List known contacts with ID, observation source, observed position, observation age and selection state. For a contact outside current observation, report only its last known observation when available. |
| `game.player.set_movement_mode` | `mode` of `manual`, `locked`, `semi_automatic` or `automatic` | Operate the movement selector and report the effective mode, including the fallback to `manual` when Auto Target is off. |
| `game.player.rotate` | `direction` of `left`, `right` or `stop` | Apply or release manual rotation input. Report the applied command, actual angle and RPM, and limiting reason. Rotation follows the motor, transmission and inertia. |
| `game.player.aim` | `point` with world-space `x` and `y` | Update the semi-automatic aim point. Report the requested point, actual direction and alignment error as the assembly turns under the normal simulation. |
| `game.player.select_target` | `contact_id` from known contacts | Use ordinary selection rules and report the selected contact ID and source, or the refusal reason. |
| `game.player.lock_target` | `target_id` from the same operator-visible IDs returned by `list_contacts` | Perform the normal Locked selection action and report the locked ID and tracking state. Require current visibility under the same rules as the Locked UI gesture. A stale contact alone does not qualify. |
| `game.player.fire` | `input` of `press`, `release` or `tap` | Operate the normal firing input. Report firing events, projectile count and any pending or refusal reason. Respect cadence, energy, temperature, recovery, jams and salvo rules. `tap` submits a single input action. |
| `game.player.release_controls` | None | Release MCP-held fire and manual rotation. Confirm both inputs are clear and preserve automatic modes. |
| `game.player.set_auto_fire` | `mode` of `off`, `local` or `local_and_radar` | Operate the Auto Fire selector and report effective mode and firing eligibility. |
| `game.player.set_auto_target` | `mode` of `off`, `local`, `local_alt` or `radar` | Operate the Auto Target selector. Report effective mode, selection and range restriction. Apply the R1 automatic-acquisition limit. |
| `game.player.set_salvo_mode` | `mode` of `all` or `alt` | Select simultaneous or alternating firing events. Report the effective mode and participating barrels as firing events occur. |
| `game.player.set_cadence` | `events_per_minute`, with zero pausing cycling | Set desired cadence and report requested and achieved event rates and limiting reason. Count an All salvo as one event regardless of projectile count. Preserve nominal reload and recovery settings. |
| `game.player.set_cycle_mode` | `mode` of `balanced` or `overload` | Apply the cycle effort policy and report effective mode, stress and per-barrel jam state. Respect energy and physical recovery constraints. |
| `game.player.set_motor_power` | `fraction` in the normal 0.30–1.00 range | Operate the power lever and report requested versus available power during ramping. |
| `game.player.set_requested_rpm` | `rpm` in the session's supported range | Operate the assembly RPM request control and report requested RPM, actual RPM and limiting reason. |
| `game.player.set_regulation` | `mode` of `off`, `power_balance` or `overload` | Apply the regulation policy and report effective regulation and motor response. `off` leaves the motor running with regulation disabled. |
| `game.player.set_shift_policy` | `policy` of `economy`, `responsiveness` or `preservation` | Set the automatic transmission policy and report the policy, selected gear and shift or blocking state. |
| `game.player.set_gear_enabled` | `gear_id`, `enabled` boolean | Change eligibility for automatic gear selection and report the actual permission and transmission state. Preserve gear wear and damage. |
| `game.player.disconnect_transmission` | None | Request normal disconnection. Report coupling state or the refusal reason. |
| `game.player.reconnect_transmission` | None | Request normal reconnection under existing repair and jam restrictions. Report coupling and engagement state or the refusal reason. |
| `game.player.start_repair` | None | Start the existing transmission repair procedure. Report its chosen target, progress or blocking prerequisites. Repair progresses through the normal simulation. |
| `game.player.set_radar_range` | `range` of `R1` through `R5` | Change display range and report the selected band and displayed distance boundaries. Preserve detection power and the R1 auto-target cap. |
| `game.player.set_sweep_interval` | `seconds` per sweep within the current control limits | Operate the sweep interval control and report the applied interval and subsequent contact-update times. |
| `game.player.set_radar_expanded` | `expanded` boolean | Expand or collapse the existing radar view and report its effective presentation state. |
| `game.player.set_cooling_route` | `route_id` from session discovery | Select Auto, Shared absorber, Turntable, Anchor or a cannon rear. Report the accepted route, branch flow and relevant temperatures. Manual routing prioritizes a branch within the available flow. Auto restores automatic prioritization. Invalid IDs leave the previous route applied. |

#### Weapons-Lab developer commands

| Method | Parameters | Behavior and observation |
| --- | --- | --- |
| `game.dev.describe` | Optional `command` name or `subsystem` ID | Report registered bindings for this build and session, their schemas and current options, valid IDs, units and limits. With neither filter, describe the full game command surface. |
| `game.dev.list_fixtures` | None | List available fixture IDs and short descriptions. |
| `game.dev.reset_fixture` | `fixture_id`, optional integer `seed` | Rebuild the test session from the fixture baseline, including runtime state and authored settings. Report the loaded fixture, effective seed, fresh IDs, reset tick and resulting play/pause state. Keep changes in memory and refresh discovery after reset. |
| `game.dev.inspect` | Entity or subsystem `id`, optional `fields` array | Read configuration, internal runtime state, derived values and limiting reasons. Diagnostic access includes hidden targets. |
| `game.dev.configure_fixture` | Component or entity `id`, `patch` object of authored field values | Validate the whole patch before applying it in memory. Report applied values, derived changes and whether Play must restart. Cover projectile settings, geometry and material, nominal timing, energy, cooling, radar and transmission where supported. Read-only runtime fields are ineligible. |
| `game.dev.spawn_target` | World-space `position`, `movement`, `size` and `health` from the current target schema | Create a passive test target. Report the new target ID and actual initial state. |
| `game.dev.remove_target` | `target_id` | Remove the test target and report removal and normal selection/contact cleanup. |
| `game.dev.clear_targets` | None | Remove all test targets. Report remaining count and normal selection/contact cleanup. |
| `game.dev.set_spawning` | `enabled` boolean | Enable or disable the automatic spawner and report its applied state. Preserve targets already present. |
| `game.dev.configure_spawner` | `patch` with spawn band, rate, population limit and other supported scenario fields | Validate the patch against the current spawner schema. Report applied settings and whether Play must restart. |
| `game.dev.set_energy` | `stored_energy` in the declared unit | Inject reserve energy within capacity bounds and report actual stored energy and capacity. Preserve capacity. |
| `game.dev.set_temperature` | `reservoir_id`, `temperature` in the declared unit | Update the chosen thermal reservoir consistently. Report actual temperature and subsequent protection or cooling response. |
| `game.dev.set_gear_health` | `gear_id`, `fraction` in `[0, 1]` | Inject gear wear and report health, condition and automatic-selection response. |
| `game.dev.inject_fault` | Discovered fault `type`, `component_id` | Inject a supported barrel or transmission jam. Report the affected state and operational restrictions. |
| `game.dev.clear_fault` | Discovered fault `type`, `component_id` | Clear the injected fault through a consistent test reset. Report restored state and any remaining independent restrictions. |
| `game.dev.read_events` | Optional `cursor` and `filters`, including `action_id` | Read events in order with ticks and a continuation cursor. Include action acceptance, refusal and completion, firing outcomes, jams, shifts, repairs and other available diagnostic events. Correlate action events with their `action_id`. An omitted cursor starts at the oldest retained event. Report a stale cursor explicitly when history was reset or is unavailable. |

After a fixture reset, obtain fresh IDs and an event cursor and use the
reported mode to decide whether to call `world.play` or `world.pause`.
Apply fixture configuration that requires a restart in Edit before entering
Play. Prepare runtime-only test conditions after Play initialization.
Release MCP-held inputs on reset, client disconnect and session replacement.

### Weapons-Lab acceptance

Use a dedicated copy of Weapons-Lab and record its source baseline and the exact
test bundle and private SDK identities. The [previous consumer trial](../evidence/native-parenting-weapons-lab-2026-11-07.md)
records the project's provenance. Discover the bindings before running the
acceptance cases and record which game features are available in that build.
For lifecycle readback, bring the host to Edit with `world.stop` if it starts
in Play. Use `world.play`, then pause before any fixed-step assertions.

| Scenario | Commands | Check |
| --- | --- | --- |
| Fire | `game.player.fire`, `world.step`, `game.dev.read_events` | Tap with a ready cannon and correlate the shot by action ID. Use a repeat tap during cooldown or prepare overheat with `game.dev.set_temperature`. Confirm the limiting state and check the result against the local firing rules. |
| HUD controls | `game.player.set_requested_rpm`, `game.player.set_motor_power`, `game.player.get_status` | Read applied requests and observe their effect during a controlled turn. On a host that routes HUD input, exercise the gesture separately using `world.cursor` and `world.mouse_button`. |
| Rotation | `game.player.rotate`, `world.step`, `game.player.release_controls`, `game.player.get_status` | Hold left or right for a fixed number of ticks. Release, step again and read angle and RPM to verify direction, the configured limit and braking. |
| Observation and setup | `game.dev.reset_fixture`, `game.dev.spawn_target`, `game.player.list_contacts`, `game.dev.inspect` | Start from a named fixture and inspect a spawned target outside current observation. Verify the player sees it only when allowed by the sensor rules and receives the last observation once current observation is lost. |
| Discovery and validation | `game.dev.describe`, `game.player.set_cooling_route` when available | Confirm availability follows the running build. Read the route before and after an invalid route ID and check that the error preserves the previous route. |
| Engine reuse | `world.play`, `world.stop`, `world.pause`, `world.resume`, `world.step`, `world.time_scale`, `world.capture` | Check the Edit and Play transitions and effective pause and time scale through Engine readback. Compare the paused step count with the capture receipt and confirm the PNG is returned. |

The agent should be able to open the prepared package through MCP, discover
the available commands, execute the sequence from a known starting state, and
save a receipt with arguments, tick counts, and observed results. Verify that a
launch without channel opt-in leaves the pipe closed and that the existing
shipping verifier still rejects development payloads.
