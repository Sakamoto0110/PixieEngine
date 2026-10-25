# Native Player Debug Controls

**Document ID:** GLOBAL-PLAYER-DEBUG-GUIDE

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** read-only rates and runtime display commands in the native player

**Surface:** guide

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

Games driven by `pixie::Run` inherit these controls after rebuilding against the engine.
The C# editor and C API hosts own their loops and do not receive these shortcuts.
The implementation is in `engine/src/Run.cpp`, `DebugConsole.cpp`, and `DebugOverlay.cpp`.

| Shortcut | Action |
|---|---|
| F3 | Toggle read-only FPS/UPS and registered CPU telemetry |
| Hold F3, press C | Toggle the command bar across the top of the window |
| Enter | Execute the current command, keeping the bar open |
| Escape | Close the command bar |
| Left/Right, Home/End | Move the text caret |
| Backspace/Delete | Edit the command |
| Up/Down | Recall the previous command / clear the line |
| Ctrl+V | Paste text into the line |

The F3+C chord preserves the rates panel's previous visibility. The command bar draws after
the game's rendering, on the window framebuffer. It captures keyboard, mouse buttons, movement,
and scroll while open. Opening it cancels held game controls; closing it requires a fresh press
before a control becomes active again. Game updates continue while typing. The line accepts up
to 200 printable ASCII characters; pasted control characters are ignored and never submit a line.

| Command | Effect |
|---|---|
| `-vsync true` | Request synchronization with the display |
| `-vsync false` | Disable that request |
| `-fps-cap true 60` | Limit rendering to 60 FPS |
| `-fps-cap true 15` | Set the minimum accepted command rate, 15 FPS |
| `-fps-cap false` | Remove the software frame limit |
| `-telemetry true` / `-telemetry false` | Enable/disable host and exposed-app CPU collection independently of display |

The rate must be a whole number of at least 15 that fits a signed 32-bit integer.
Invalid commands show usage and preserve the existing settings. VSync and the software cap
apply together: disabling the software cap does not disable VSync. Display drivers can override
the requested swap interval; a cap is a ceiling, not a promise that the game can reach that rate.

Changes apply on the render thread at the next input poll. They last for the current run and
do not rewrite the game's configuration. Neither command changes the configured fixed update
rate. UPS counts actual `App::Update` calls; FPS counts rendered frames. Both use the existing
one-second rate meter, so the initial display is zero until the first sample. There is no entity
count unless the app registers one: the native host does not own a game's World. Collection,
CPU timing semantics, CSV and `--telemetry on|off` are described in the [telemetry guide](telemetry.md).

Hosts that implement other modal UI can use `Input::ReleaseAll()` to release held keys/buttons
once, discard pending presses, scroll, and movement, and retain the cursor position. The native
host also uses it on focus loss.
