# Assistant handoff

**Document ID:** GLOBAL-CONSUMER-ASSISTANT-HANDOFF
**Schema version:** 1
**Kind:** guide
**Lifecycle:** current
**Subject:** optional assistant-led public Editor walkthrough and evidence
**Surface:** guide
**Boundary:** global
**Authority role:** non-normative
**Mutation:** authored
**Indexing:** include

Use this workflow when the game owner asks an assistant to carry out and observe
the first-game walkthrough through the public Editor UI. The ordinary
[consumer handbook](README.md#read-in-this-order) also works for human readers.

## Handoff inputs

Supply this handbook, its example sources, the companion Editor walkthrough,
the game scope, the complete Editor application, the compatible public SDK and
a writable game folder. Record verified physical paths and SDK identity in the
machine's `LOCAL-SETUP.md`. An Editor process may have different path access
from the assistant's file tools. [Setup and compatibility](setup.md#when-a-path-works-in-one-application-but-not-another)

The game scope specifies rules, bindings, layout dimensions and success
conditions. A sketch supplies layout guidance. Pick the examples that serve
those requirements and record adaptations. For example, a clickable HUD can
use the control panel helper without physics sensors, lighting or particles.

## Computer Use workflow

Use Computer Use for project creation/opening, Inspector edits, component
attachment, Save, Undo/Redo, Play/Pause/Step/Stop, code generation, Build and
reopen, and Export. Report unavailable or interrupted UI access. Classify CLI,
private automation, MCP and direct file changes by their actual route.

1. Launch through the Computer Use tool's supported app route or open the
   supplied folder in an approved file browser and launch the executable.
   Some tools need the app to be running before they can attach. Record any
   preparatory shell launch separately and follow the tool's access rules.
2. Use fresh screenshots and visible labels. Native ImGui may expose few
   accessibility elements. Coordinates vary with size, layout and DPI.
   Observe each result before the next action and rediscover the window after
   rebuild/reopen. For a missed click, move onto the control, allow hover
   processing, then click and observe. A held control requires mouse-down
   followed by mouse-up when supported.
3. Edit user-owned C++ in an allowed external editor or file tool, then build
   from PixieEditor. If the task requires mouse and keyboard for code entry
   too, use Computer Use with an external text editor.
4. Check the bound controls in Play and verify Stop restores authored state.
   Save, reopen and inspect the saved state. Export from the menu, then
   separately launch and exercise the standalone player.

## Reusable prompt

> Read the Pixie consumer handbook and the companion Editor walkthrough. Build
> the game in my supplied scope with the installed public SDK and Editor. Keep
> project files in the game folder and export to a separate destination. Follow
> the Computer Use workflow above and report blocked UI access.
>
> Start with the saved project, code build, Play/Stop and export loop. Create
> components through the Editor and edit their user-owned source. Keep generated
> registries intact. Look up exact declarations in the selected SDK. For
> continuations, reopen the existing game and reuse its Editor cache. Use a
> scratch build for a specific diagnostic and export when the standalone check
> or handoff is ready.
>
> Report missing capabilities and preserve the supplied Engine, SDK and Editor.
> Record compilation, gameplay, save/reopen and export as separate results in
> CONSUMER-REPORT.md. Distinguish direct observations, source review, user
> reports and unchecked items. Keep publication outside this workflow.

Use the [consumer report template](consumer-report-template.md). Record the
actual UI routes, external source edits, diagnostic commands, build identities
and any manual output copies. Evidence from the Editor and standalone player
belongs to the surface where it was observed.
