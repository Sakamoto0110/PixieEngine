# A Development Channel for Agents

**Document ID:** GLOBAL-PROPOSAL-DEV-CHANNEL

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** a local, development-only command channel through which an agent drives and edits a running session (player or editor) without desktop automation

**Surface:** proposal

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## PIXIE-PROPOSAL-013

**State:** promoted

**Idea:** An agent that works on a game today sees it only through `--quit-after` and
`--screenshot`, or through desktop automation (screenshots, clicks and typing in the editor), which
is slow, fragile and blind to state. A development channel lets the agent talk to a running session
directly: list entities, read and write their fields, play, pause, step, inject input and capture a
frame, with structured answers.

The channel has two layers, because they test different things:

- **The World layer** acts on the native World directly. It tests the engine and lets an agent set up
  a situation quickly. A field it writes appears in the inspector on the next frame, but the write
  does not pass through the inspector, so it proves nothing about the inspector.
- **The inspector layer** acts through PixieInspector's node tree, by the same path a view takes when
  the user edits a row: the node's writer, its binding, its report, and the editor's history. It
  tests the inspector and the editor: that a row shows the right value, that an edit reaches the
  World, that a mixed multi-object row writes every object, that a refused write is reported, that
  undo restores the value.

Neither layer clicks ImGui widgets. Whether a widget draws and responds is checked by capture
([PIXIE-PROPOSAL-014](deterministic-capture.md)); widget-level input would need an ImGui test engine,
which is outside this proposal.

### The World layer

- **Where it lives.** A new static library, `devchannel`, over `runtime`. Both hosts already own a
  `ProjectSession`: the C API wraps one (`PixieSession` in `capi/src/Capi.cpp`) and the player runs one
  through `ProjectApp`. The channel attaches to a `ProjectSession`, so the player, the editor and
  tests get the same commands. It is a general engine feature, so the guardrail that the engine
  never depends on the editor holds.
- **How a host opens it.** The native dispatcher takes a JSON request and returns a JSON answer, and
  knows nothing of transports. The player owns a native transport and takes `--dev-channel <name>`.
  The editor owns the transport itself, in C# (see the inspector layer), and passes World commands
  to the dispatcher through one new C API function, `pixie_dev_dispatch(session, request, buffer,
  capacity)`. One process therefore has one channel, and nothing listens without the flag.
- **Transport.** Local IPC only, never TCP: a named pipe (`\\.\pipe\pixie-<name>`) on Windows and a
  Unix domain socket on Linux, owned by the current user. One client at a time. The transport is the
  only OS-specific part and sits with the platform code.
- **Protocol.** JSON-RPC 2.0, one message per line. The first call is `hello`, which returns the
  protocol version, `pixie_api_version()`, the project name and the session mode. Errors carry the
  engine's `PixieResult` and `pixie_last_error` text.
- **Threading and timing.** A transport thread only reads, parses and queues. The session's own thread
  drains the queue at one fixed point per frame, after `Advance` and before `Render`, so a command
  never lands in the middle of a step and the step phases (`PreUpdate`, `Update`, `PostUpdate`) stay
  intact. `step` and `advance` answer after the steps have run. A paused session still drains the
  queue.
- **Addressing.** Entities by scene ID (`pixie_entity_scene_id`), which is stable across play and stop
  and appears in the scene file; runtime entity numbers and names are accepted as conveniences.
  Components and fields by name (`Transform2D.position`), resolved through the field descriptors,
  never by index. Values are JSON, typed by the field kind.

**Commands.** Almost every command is an existing C API function given a JSON face, so the channel
adds dispatch and not behavior:

| Group | Commands | Over |
|---|---|---|
| Session | `hello`, `stats`, `play`, `stop`, `pause`, `resume`, `step {count}`, `advance {seconds}`, `time_scale`, `reset` | `pixie_session_*`, `pixie_session_stats` |
| Schema | `types`, `type {name}` (fields with kind, label, range, units, enum names) | `pixie_type_*`, `pixie_field_*` metadata |
| Entities | `entities`, `entity {id}` (every component and field as JSON), `create`, `destroy`, `rename`, `add_component`, `remove_component` | `pixie_entities`, `pixie_entity_*`, `pixie_component_*` |
| Fields | `get {id, field}`, `set {id, field, value}`, record lists, points and text | `pixie_field_get/set*`, `pixie_record_*` |
| Experiments | `snapshot {ids}` returns a token; `restore {token}` | `pixie_entities_snapshot/restore` |
| Input | `key`, `mouse_button`, `cursor`, `scroll` | `pixie_input_*` |
| View | `pick`, `pick_box`, `view_to_world`, `world_to_view` | `pixie_pick*`, `pixie_view_*` |
| Capture | `capture {name}` writes a PNG of the next rendered frame and returns its path | the render target readback ([PIXIE-PROPOSAL-014](deterministic-capture.md)) |
| Project | `save` | `pixie_project_save` |

`snapshot` and `restore` let an agent try a change and undo it without touching the scene file.
`capture` writes only inside one folder fixed when the channel opens (by default
`build/captures/<name>/`), so the channel can never write anywhere else on disk.

### The inspector layer

- **Half of it exists, as a one-shot script.** The editor already takes `--select <name>`,
  `--set <Type.Property=v>` (through `InspectorPanel.Set`, which parses the text and calls the
  node's `SetValue`, so the edit goes through the inspector and the `EditRecorder` into the history),
  `--do <command>` with some thirty editing commands (`select`, `move`, `add`, `remove`, `duplicate`,
  `delete`, `undo`, `redo`, `play`, `stop`, `save`, collider and shape edits, `wait`), `--seed`,
  `--quit-after` and `--screenshot`. What it lacks: the commands are fixed at launch and timed in
  seconds (`--edit-after`, `wait=`), and nothing reads back. An agent can change the inspector but
  cannot ask it what it shows, nor react to an answer.
- **The channel reuses those handlers.** One implementation of each editing command, with two front
  ends: the command line (unchanged, for one-shot runs and tests) and the channel. The channel adds
  the read side: `inspector.tree` (each node's `Path`, label, value, mixed state, read-only, range and
  units), `inspector.get {path}`, `inspector.report`, `editor.history` and `editor.selection`. Writes
  are `inspector.set {path, value}` (the `--set` path) and `editor.do {command}` (the `--do` commands,
  one name for one gesture).
- **No new PixieInspector API is needed for a first version.** Its nodes already expose a stable,
  member-based `Path`, `GetValue`/`GetValues`, `SetValue`/`SetValues` and the `InspectorReport`, and the
  `Inspector` is enumerable. If a check needs the exact text a view displays (units and formatting
  applied), that formatting moves from the views into the core, as a later PixieInspector release.
- **What a check looks like.** `inspector.select` an entity, `inspector.set` a row, then `world.get`
  the same field through the World layer and compare; `editor.undo`, and compare again. The two
  layers check each other.

Commands are namespaced by layer (`world.*`, `inspector.*`, `editor.*`); the player answers only
`world.*`.

**An MCP server in front.** The agent does not speak the pipe protocol by hand. A small MCP server,
`tools/pixie-mcp/` (C#, .NET 10, beside the editor's toolchain), launches the player or the editor
with `--dev-channel`, connects, and exposes each command as an MCP tool. `capture` returns the PNG as
image content, so the model sees the frame in the same call. The MCP server is a tool: nothing in the
engine depends on it.

**Rationale:** It closes the feedback loop without a human in the middle and without desktop
automation: an agent edits a value, steps, captures and reads the result in seconds. Building it
over `ProjectSession` and the C API's semantics reuses code that is already tested, keeps one way to
change the World, and makes the player and the editor equal hosts. Draining at one point per frame
keeps the engine single-threaded where it matters.

**Constraints:**

- Development only. The channel is compiled when the CMake option `PIXIE_DEV_CHANNEL` is on (default
  on), and a host listens only when given `--dev-channel`. A packaged game must not contain it; how
  the export turns it off belongs to [PIXIE-PROPOSAL-010](shared-engine-and-export.md).
- The C API changes only by `pixie_dev_dispatch`, with `pixie_api_version` raised.
- `world.*` edits do not enter the editor's undo history; `inspector.*` edits do. Both mark the
  project unsaved (`pixie_project_unsaved`), so Save and the editor's warning still see them.
- The command-line options keep working as they are; moving their handlers behind the shared
  implementation must not change what a scripted run does.
- The first version stays inside PixieEngine. PixieInspector changes only if displayed-text checks are
  wanted, as a separate release.
- Verification: the dispatcher is tested in process through an in-memory transport (no OS IPC), one
  test per command group, under `tests/`; one integration test opens a real pipe or socket against a
  hidden-window session, like the existing C API tests.
- Useful to an agent in a Linux sandbox only once the engine builds there; that preset is a separate
  decision.
- Depends on the render/platform split in progress (PIXIE-TODO-050) for where the transport and the
  readback live; it should start after that work closes.

**Open questions:**

1. Which layer comes first? The recommendation is the inspector layer in the editor: most of its
   commands exist, and it is the one desktop automation was standing in for.
2. Should `save` be allowed over the channel, or should only the owner save?
3. Is C# the language for the MCP server, or Python, as with `scripts/`?
4. Should the player get the editor's `--seed`, so a player run and an editor run of the same scene
   start from the same World?

**Promotion target:** [PIXIE-TODO-051](../../TODO.md#pixie-todo-051--shared-engine-game-export-and-local-mcp-development-channel).

**Source:** owner request in conversation, 2026-10-28: a development-only library that agents use
to drive the editor without desktop automation, with a path through the inspector itself so that the
inspector's own behavior can be tested.

**Outcome:** supplied and promoted by the owner on 2026-10-28 within PIXIE-TODO-051, together with
proposals 010 and 014. The owner's explicit requirements include session stepping, native schema
discovery, C API-backed snapshot/restore, input/view operations, Inspector readback and history,
and fixed-folder capture returned to MCP as image content. World and Inspector edits remain
separate paths; the latter must reuse the existing handlers and EditRecorder. Entity addressing
uses scene IDs; runtime entity numbers/names above are proposal conveniences, not a replacement
for the accepted stable addressing contract. The owner's first answer to question 2 kept channel save
disabled, including save reached through `editor.do`. On 2026-10-30 the owner answered it: save
stays available and a build flag removes it. `editor.do save` reuses the editor's save handler in
development builds, `-p:PixieChannelSave=false` leaves it out of an editor build, and `world.*` and
the player never save. Question 1's
recommendation does not bypass the mandatory shared-engine EnTT gate. All development-operation code/artifacts are excluded
from game exports, publish and remote-release outputs. Promotion does not claim implementation,
validation or package delivery; scheduler progress is in TODO/TASKLIST.
