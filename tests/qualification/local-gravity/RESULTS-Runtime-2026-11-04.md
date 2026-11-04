# Runtime scope-gravity qualification — 2026-11-04

This report extends the isolated Box2D comparisons with actual runtime Worlds,
Scene scopes, deferred membership and consumer paths. It does not authorize or
activate local body gravity. The production body solver remains globally driven;
no vendored solver or normal body integration was modified.

## Implemented runtime consumers

- Upright CharacterMotor2D resolves its owning entity's World/Scene gravity,
  then applies its existing gravityScale once. Downward and zero vectors work;
  unsupported lateral/upward vectors produce per-entity diagnostics. Other valid
  motors and globally driven bodies continue; changing the override to supported
  gravity recovers the affected motor.
- Dynamic particles resolve gravity per pool entity. Existing per-pool gravityScale
  is applied once by the existing integrator. World-only pools retain their legacy
  backend/default path. Scene resolution does not install a collision backend.
  Ordinary particles remain ballistic; physics still precedes particle stepping,
  and a transfer requested during Physics changes gravity on the next tick.
- No local body compensation was installed in production. A qualification-only
  PreUpdate system applies forces after normal sync and before the complete solve,
  using actual World::ResolveGravity and public CollisionWorld2D APIs. It skips
  sleepers and explicitly wakes an affected body once when its resolved value changes.

## Qualification

The isolated worktree uses dirty Engine baseline ae598b612583f299271ec83f1cf089ce1aa97232,
MSVC19.51 x64 and Ninja Multi-Config. UnitsPerMeter=1 for the body probe, so its
position/velocity values are backend meters/meters-per-second. No graphics context
is created. Normal runtime consumer regressions use the default units.

| Check | Debug | Release |
|---|---:|---:|
|Runtime tests after six added regressions|301, zero failed checks|301, zero failed checks|
|Actual runtime body comparisons|14 PASS|14 PASS|
|Local motor/pool normal production checks|3 PASS|3 PASS|
|Loaded project-scene inheritance controls|4 PASS|4 PASS|
|Non-collinear interacting Scene observation|6 contact steps, NOT-COMPARABLE|6 contact steps, NOT-COMPARABLE|
|Omit compensation negative control|14 failed body cases, exit1|14 failed body cases, exit1|
|Wake sleepers every tick negative control|9 failed body cases, exit1|9 failed body cases, exit1|

Before the runtime consumer fix, the six added tests produced301 tests with41 failed
checks. The same tests then passed in Debug and Release. This is directed red/green
regression evidence, not an unexplained successful rerun. Existing legacy tests passed.

The14 body comparisons cover World zero, effective zero, lateral override,
gravityScale0.5, mass override change, gravityScale change during simulation,
resting contact, downward and lateral CCD against thin barriers, two actual Scenes
with equal or different collinear gravities in contact, actual deferred
World::SetScope transfer, unchanged sleep, and sleeping transfer. Each compares
samples at1,60 and600 steps; contact pairs, events and sleep state are compared
at every step. Equal-gravity cross-Scene contact existed for596 ticks, different
collinear contact for582. Deferred transfer occurred at tick60; sleeping transfer
at121. Each affected transfer had one explicit wake operation.

The largest observed position drift was0.0000610351562m at step600 in the
changing-gravityScale case. No numeric600-step position threshold is approved;
that drift is reported rather than presented as an equivalence guarantee. The
largest60-step drift was0.00000781170093m (lateral CCD); the1-step position drift
was zero. Both stay within approved1/60-step limits based on linearSlop0.005m.
The largest linear-velocity error was0.0196335805m/s at the first fast lateral CCD
sample; its native speed is approximately100m/s, so it satisfies the0.1% relative
limit. The absolute0.001 floor is used only when larger than that relative bound.
Angular errors were zero. Contact-pair, event and sleep mismatch counts were zero;
no additional full barrier crossing was detected. Per-case values are in the two
RESULTS-Runtime-<configuration>-2026-11-04.csv files.

The same public force API always requests wake. The successful test adapter's
sleep guard and single-change wake are therefore material: removing the guard
fails the negative controls. Neither negative result came from a crash; each
reached the explicit 'runtime experimental criteria: FAIL' receipt.

Four project scene files (LightMachine, LightLab, PhysicsLab, CouplingLab) were loaded
without rewriting them and advanced600 headless runtime ticks. The scoped versions
used Inherit/Multiply1, retained inactive override vectors and matched the World-only
reference at1/60/600 with zero position/velocity/contact/event differences. This is
runtime integration evidence, not a player Build/render/UI or owner visual acceptance.
Synthetic local consumer regressions separately compare motors and two particle
pools to equivalent native/global references at1/60/600, with and without a backend.

## Explicit limits and approval boundary

### Expanded experiment: rejected

The integrated probe was extended with `--changing-world-base`. The native
reference retains global gravity (0, 10), while the experimental body's Scene
override remains (0, 10). After both bodies sleep, only the experimental World's
base gravity changes eight times. Effective gravity, body identity and shape
identity remain unchanged. Both Debug and Release return exit 1 with **204 sleep
mismatch steps**. Contact/event mismatch and extra tunneling counts remain zero;
maximum position error across all steps is only 0.00000163165822 m. Small position
error does not excuse incorrect sleep under the owner's acceptance criteria.

This is a failed technique qualification, despite the original fourteen passing
cases. Production force compensation is rejected. The owner's previously approved
fallback applies: bodies retain World/global gravity, logical Scenes remain
functional, and qualified local motor/pool consumers remain available. No solver
modification or local-body activation is included. Another physical technique would
require a separate design and qualification, not relaxed sleep tolerances.

Expanded raw CSVs accompany this report. Root command logs/receipts are preserved in
`.local/validation/todo057-run11/changing-base-{Debug,Release}*`. The serial CTest
`local-gravity-world-base-rejection` validates the completed rejection reason,
normal comparison passes and stable-override/identity preconditions. Its procedural
PASS means the experiment is still correctly rejected, not that body compensation
passed qualification. It does not accept a crash or any arbitrary nonzero exit.

- Non-collinear different gravity in interacting bodies has no equivalent
  one-World global-gravity reference. Actual runtime Scenes recorded six contact
  steps, but no comparative PASS or general physical equivalence is claimed.
- The deferred test invokes World::SetScope from the runtime Physics phase after
  its solver system, while the World is stepping. The older standalone probe
  separately covers a request from a native pre-solve callback. They are distinct
  observations; this test does not expose a new production pre-solve API.
- CCD checks contact pairs and additional complete crossing at tick boundaries;
  it does not reconstruct the full continuous internal solver trajectory.
- Body identity, pose and velocity are checked around transfer; normal body
  reconstruction caused by an explicit mass/configuration edit retains its
  existing semantics rather than promising a backend handle never changes.
- Existing strict authoring candidate validation may reject an authored lateral
  motor configuration before publication. Runtime loading/synchronization diagnoses
  and skips the affected motor while valid peers continue. No aggregate movement
  validation was weakened by this change.
- The original passing subset does not authorize body activation; the expanded
  failure above determines the current production fallback.

The target is registered as local-gravity-runtime-qualification in CTest, serial,
with a120-second timeout and physics/experimental labels. Sensitivity controls remain
explicit commands with validated failure reasons, rather than WILL_FAIL crash oracles.
The CTest registration was added after the equivalent direct Debug/Release runs;
root integration must configure and exercise that registration in its final matrix.

Raw commands/logs, receipts, red/green results, initial/final source hashes, scene
hashes and parsed summaries are preserved in the isolated worktree at
.local/validation/gravity-runtime-20261104/. The first configure with samples OFF
was rejected because the normal snapshot registration requires LightLab. The cause
was diagnosed; samples ON configured successfully while building only requested
qualification targets. No product source workaround was made for that setup error.
