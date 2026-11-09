# Consumer report template

**Document ID:** GLOBAL-CONSUMER-REPORT-TEMPLATE
**Schema version:** 1
**Kind:** guide
**Lifecycle:** current
**Subject:** reporting a game built with the public SDK and Editor
**Surface:** guide
**Boundary:** global
**Authority role:** non-normative
**Mutation:** authored
**Indexing:** include

[Handbook](README.md) · [Export and troubleshooting](export-and-troubleshooting.md)

Copy the template below to `CONSUMER-REPORT.md` in your game folder. Keep it
proportional to the game. Replace the bracketed prompts and omit unused optional
rows; do not claim checks merely because they appear in the template. A short
report with explicit gaps is more useful than an unqualified "everything works."

```markdown
# [Game name] — consumer report

Date: [date and machine/account context; no credentials]
Scope/build: [requested revision and what is playable]

## Inputs and outputs

- Project folder:
- Editor executable/version:
- Selected SDK root, version, profile/configuration and SDK/catalog identity:
- Export folder and executable:
- Relevant artifact hashes, if measured:

## Work performed

- Actual Computer Use routes: [create/open, edit, history, code generation,
  Build/reopen, Play/Stop, save/reopen, export; state what was actually done]
- External source/asset work: [files edited/copied and tools used]
- Commands actually run: [or "Build/export used the UI; no terminal equivalent
  was executed"]
- Public APIs/examples used or adapted:
- SDK selection or other local setting changes:
- Producer source inspected, if any: [paths and reason; distinguish reading
  from a build dependency or implementation change]
- Output preparation: [manual asset/license copies, if any]

## Acceptance observations

Use one row per requested behavior, including controls and persistence/export.
Evidence kind is Direct observation, Code inspection, User report or Unverified.
Outcome is Pass, Fail, Inconclusive or Not checked. Name the build and whether
it was the Editor or exported player; do not inherit r1 checks as r2 observations.

| Behavior | Build/surface | Evidence kind | Outcome | What happened / evidence |
|---|---|---|---|---|
| [requested behavior] | [...] | [...] | [...] | [...] |

## Documentation friction and workarounds

- Missing/ambiguous instruction and the route actually taken:
- Error text and recovery, if any:
- Synthetic input versus manual comparison, if relevant:
- Assumptions/tuning chosen where the scope was silent:

## Remaining limits

- Unchecked or inconclusive flows:
- Known failures/blockers:
- Extra changes outside the game, if any: [never imply none without checking]
- Evidence locations: [real saved logs/screenshots or conversation captures;
  do not claim a screenshot archive that was not saved]
```
