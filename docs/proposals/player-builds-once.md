# The Player Builds a Game Once

**Document ID:** GLOBAL-PROPOSAL-PLAYER-BUILDS-ONCE

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** starting a game in the player with one build of its project, in play mode, rather than an edit-mode build followed at once by a play-mode build from a scene written out of the first

**Surface:** proposal

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## PIXIE-PROPOSAL-012

**State:** idea

**Idea:** The player starts a game in two builds. `Run.cpp` calls `ProjectSession::Start`, which
builds the project in edit mode (`Build(true)`: a new World, the start scene read, and
`Project::Build`), and then `ProjectSession::Play`, which writes that World out as a scene
(`WriteScene`) and builds again from it (`Build(false)`), with `Project::Build` a second time. The
first build is thrown away unused. Anything a game does in `Build` is paid twice, and whatever it
adds to the World is also written to scene text and read back.

The logic sandbox measured it on 2026-10-18: each build took some 13 s with its large libraries,
some 26 s before the window could show anything; and the circuit it reopens in `Build` was loaded in
both builds, a part saved last loading twice over itself. It worked around this in its own code: it
now reopens on its first `BeforeStep`, which only play mode runs. Every game that loads something in
`Build` meets the same thing.

- The player builds once, straight into play mode: `Start` takes whether to play, or `Play` skips
  the scene round trip and the second build when the World is the one just built from the start
  scene and nothing has been edited since.
- The editor keeps what it does now: edit mode builds, and Play still writes the edited World to a
  scene and builds from it, so Stop returns to the edited scene.

**Rationale:** A game's start costs one build; a game needs no workaround to load once; and nothing
large is written to scene text only to be read back.

**Constraints:** Edit mode, Play, and Stop in the editor behave as they do now; `Project::Build` is
still called with a World read from the start scene; `--quit-after` and `--screenshot` keep working.

**Promotion target:** `TODO.md`, when the owner decides it.

**Source:** the owner's request of 2026-10-18 to record it, after the logic sandbox's startup was
measured.

**Outcome:** none yet.
