# Consumer handbook: first-consumer follow-up

**Document ID:** GLOBAL-CONSUMER-HANDBOOK-EVIDENCE-2026-11-07
**Schema version:** 1
**Kind:** audit
**Lifecycle:** historical
**Subject:** consumer feedback, SDK path reconciliation and exported-player observation
**Surface:** validation-evidence
**Boundary:** global
**Authority role:** evidence
**Mutation:** snapshot
**Indexing:** include

This follows the [2026-11-06 record](consumer-handbook-2026-11-06.md) and supports
the revised [consumer handbook](../handbook/README.md). It preserves the original
record and consumer report. This was documentation follow-up and a requested
short playthrough, not another 021/022 qualification round, release approval or
clean-machine test. D09 remains frozen. No Engine, Editor or game implementation
was changed, and no SDK selection was changed by this follow-up.

## Inputs and provenance

Artifact locations use placeholders for local evidence roots on the existing Windows account,
not portable installation instructions. Engine HEAD remained
`1f44ae74008f1c2df4ff9b86ee8bb44f8ce8e072`; Editor HEAD remained
`c7cc2bfad332d70674cceb3e4474539ffccf528e`. Both working trees already contained
documentation work, which this update preserved.

| Input | Location / identity |
|---|---|
| Claude's original report | `<consumer-workdir>/CONSUMER-REPORT.md` |
| Report SHA-256 | `e5bb559caae83c1efa01e037fe5868bb3cdf75faa6f98d777219b1302248e707` |
| Game project | `<workspace>/PixieEngine_ConsumerTests/Weapons-Lab` |
| Exported player | `<workspace>/PixieEngine_ConsumerTests/Weapons-Lab-export-r2/WeaponsLab.exe` |
| Player SHA-256 | `cd54021a08771d5b4ceafe37f7a1b3f32f86d6889d2600b0e1ba8b5c11811a1e` |
| Editor executable | `<workspace>/PixieEditor/src/bin/Release/net10.0/PixieEditor.exe` |
| Editor SHA-256 | `d8a68d84aa87bee10b0d7f63c5e4769926d25970f98dfda084ccf7ed42fa7f97` |
| SDK class/profile | Public `0.3.0.0`, authoring Release; paired shipping Release for export |
| SDK ID | `ebbab809d3abb339e8753986dcc099a85323b78c2bde4db9d77ecc4301920938` |
| Catalog SHA-256 | `99c9f6789a1fd79f018f15e53f53c897619fde289454e938629d83bc8b85089a` |
| Exported `pixie.dll` SHA-256 | `e42b94d3c4074704bebecbb3b0f943fa78c23ef565f1383e43edcc31b3d9e3d9`, equal to the selected SDK shipping DLL |

The report is consumer-provided evidence. Its UI claims are not silently promoted
to independently observed results. Claude reported using the installed SDK for
build/export without modifying producer implementation. It also read Editor
source to investigate synthetic keyboard input. Consequently this is not evidence
that the consumer never consulted producer source, nor a source-isolation test.

## SDK path reconciliation: directly checked

The r1 handoff supplied this logical path:

`<local-app-data>/Pixie/SDKs/0.3.0.0-99c9f678`

Claude reported that its independently launched Editor could not resolve that
path and selected this physical path through the UI:

`<codex-local-cache>/Local/Pixie/SDKs/0.3.0.0-99c9f678`

Claude disclosed the selection change before writing the report. Subsequent
read-only checks from Codex confirmed both paths accessible, the directories
and catalogs identifying the same respective NTFS objects, and identical catalog
hashes. The current Editor settings already stored the physical `SdkRoot`, the
catalog hash above, authoring profile and Release configuration. Those settings
were left unchanged. The r2 handoff records this physical path and marks it as
machine-specific; it is not a general requirement to install Codex to use Pixie.

## Consumer-reported workflow and friction

Claude reported create/open, Inspector edits, save/history, code/component
generation, Build and reopen, Play/Pause/Step/Stop and menu export through the
actual public Editor. User-owned C++ and asset files were edited outside that
UI, as permitted by the handoff. The report identifies which game revision each
acceptance observation covered.

The documentation update incorporates these bounded findings:

- Launch permissions and native ImGui input differed between Computer Use tools.
  Move-before-click and real button-down/up actions helped that consumer;
  a fixed delay is not a guaranteed cross-tool remedy.
- An authored Text reference exported the HUD font. Its adjacent license text
  was not discovered automatically; Claude copied it into each output. The r1
  guide already mentioned keeping font licensing, but lacked this concrete
  export step.
- Inspector text appeared stale after Undo of a custom field while the game value
  reverted. This remains a consumer report, not an independently reproduced defect.
- Export log text was clipped. Claude checked produced files, paired runtime
  identity and a running player instead of claiming it read a success line.
- New reflected fields were absent from the unchanged saved scene and used C++
  defaults. Code rebuild alone did not rewrite the saved scene.
- The handoff prompt could be read as requiring particles/physics even when the
  game scope did not. Examples are now explicitly optional. This consumer used
  game-owned interactions and did not validate scene physics events, particle
  systems or lighting.

## Keyboard correction: distinguish manual and synthetic input

The user manually tested Space in the Editor viewport and reported successful
capture. Keyboard rotation was not bound by the game; HUD direction buttons
did not imply keyboard shortcuts. Claude's unsuccessful synthetic Space attempts
remain automation observations with an undetermined cause, not a confirmed
Editor keyboard defect. Its successful player Space observation covered r1,
not r2. This clarification takes precedence over interpreting earlier notes as
proof that Editor keyboard input was broken; the original records remain intact.

## Independent Computer Use observation: exported r2 player

On 2026-11-07, Codex launched the existing r2 executable and operated its actual
window using the Windows Computer Use plugin. Direct observations:

- The player opened with its world view, Manual mode, direction controls, mode
  button, red FIRE button, RPM slider and actual-RPM text.
- Switching to Semi-auto dimmed the manual direction buttons. Moving the cursor
  into the upper-right, then upper-left play area produced visible turns,
  intermediate nonzero RPM readings and later zero RPM at rest.
- Clicking FIRE in Semi-auto produced a visible yellow projectile aligned with
  the barrel. Moving the pointer to the HUD retained the previous aim direction.
- Switching to Auto produced changing aim and projectiles without further aiming
  or firing input. Successive frames showed a projectile near a target and later
  that target absent; the exact contact frame was not isolated.
- Red triangular targets appeared with varying orientations and paths toward
  the lower area. The requested destination-band boundaries were not measured.
- Sampled title counters were approximately 100 FPS / 60 UPS. These are displayed
  counters, not a benchmark.
- The player was returned to Manual and closed normally. A later window inventory
  confirmed its window was gone.

One synthetic Space action in Manual produced no projectile in the following
capture. That observation is inconclusive, not a failure finding. Screenshots
are in the conversation's Computer Use results; no separate screenshot archive
is claimed. A local observation note is retained with the handbook working files.

## Coverage and revision limits

The playthrough did not measure cadence, nearest-target selection, wrap-boundary
behavior, the slider, held manual controls or resize behavior. It did not repeat
Editor save/reopen/export or establish complete game acceptance. The original
ControlPanel helper was not independently exercised visually by this playthrough;
the consumer's adapted HUD was. The earlier example compilation/headless checks
retain their original scope.

The documentation revision adds a portable report template and a new r2 handoff
snapshot, with corrected local setup and evidence distinctions. It leaves the
r1 folder and original evidence/report intact. Documentation links, metadata,
bundle contents/checksums and preservation are checked separately; no C++ build,
runtime test or UI session is repeated merely for these text changes. No commit,
push or public release is part of the update.

Documentation verification covered 198 Engine documents and 10 Editor documents
with zero structural problems, plus diff whitespace checks. The r2 snapshot's
43 local Markdown links, 13 manifest entries and 14 ZIP members passed checks;
archive bytes matched the folder. Comparison with the 813-file pre-edit ledger
found only the seven intended existing-document changes; the 11 r1 files,
original report, initial evidence and six example sources were unchanged.
Two new canonical documents were added: this record and the report template.
A fresh reader answered seven handoff questions without a blocking gap. Its
small prompt ambiguity about writing an export outside the game folder was
corrected. These are documentation checks, not additional product validation.
