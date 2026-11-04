# Local gravity qualification probe

This standalone executable compares two copies of the vendored Box2D solver in
physical units. The reference World uses the desired gravity globally. The
experimental World keeps its base gravity and, before each complete step,
applies `current mass * gravityScale * (desired - base)` to each awake dynamic
body with Box2D's `wake=false` force call. This is an experiment, not a runtime
implementation or approval of force compensation.

Run it only when the shared build is free. Configure this directory in a
separate build directory, then build and run `local-gravity-probe` in both Debug
and Release. It prints a CSV table and exits nonzero if a measured case violates
the current criteria. Preserve the complete output and exit status; a single
passing summary does not replace the individual measurements.

The limits come from PIXIE-PROPOSAL-018: position error at step 1 is at most
`0.1 * B2_LINEAR_SLOP`, and at step 60 at most `0.5 * B2_LINEAR_SLOP`.
Linear and angular velocity error at steps 1, 60 and 600 is at most 0.1% of
the reference magnitude, with an absolute floor of 0.001 in the respective
units. The tool records step-600 position drift without assigning it a limit.
It compares the logical touching pair set, Box2D contact count, and cumulative
begin/end event counts every step; any difference fails. It also records whether each World
crossed the far side of a floor, and whether the experimental
World tunneled when the reference did not. Repeated waking of a sleeping body
fails. If the sleep fixture never sleeps, that case fails as unqualified.

The `scope` numbers in this probe are labels in the test fixture. The
`cross-scope-contact` case has bodies with distinct labels and the same
effective gravity. The `cross-scope-different-collinear` case compares two
different collinear effective gravities: the reference uses one native gravity
and different `gravityScale` values to produce the same accelerations. A
separate observational fixture uses non-collinear `(0,10)` and `(4,0)` in two
labeled scopes and reports cross-scope touching without a comparative claim.
The transfer cases have one dynamic body and a World-only floor, so their
global reference gravity can change at the same boundary without affecting
another dynamic body. The sleeping-transfer case checks one explicit wake
call. The deferred case requests transfer in Box2D pre-solve and applies it
after the complete step; it does not exercise the runtime's `World::SetScope`
API. The CCD check detects a full crossing; it is not a complete swept-trajectory
proof. No motor, particle, editor, persistence or actual project scene is
exercised by this tool.

Results from the expanded 2026-11-03 run are in `RESULTS-2026-11-03.md` and
the matching dated Debug and Release CSV files. The 2026-11-02 files remain
available as the original ten-case evidence.
