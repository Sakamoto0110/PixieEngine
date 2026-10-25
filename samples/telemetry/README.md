# Telemetry demo

**Document ID:** SAMPLE-TELEMETRY-README

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** neutral headless and native consumers of generic CPU telemetry

**Surface:** introduction

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

`bin/Release/telemetry-demo.exe` runs 600 fixed steps without a window, then writes CSV to stdout.
The CSV includes named World system timings, an exact update counter and a simulated-time gauge.
`--telemetry off` runs the same steps with collection disabled; registered metrics remain zero.

For the native view run `telemetry-demo --headless off --telemetry on --vsync off --fps 60`.
F3 shows rates and CPU metrics while a square moves. F3+C opens the existing command bar;
`-telemetry false` disables collection without hiding the display, and `-telemetry true` resumes.
`--quit-after 5 --screenshot telemetry.png` captures the last frame. CSV is written on exit.
The sample enables its own collector by default; the native host's collector defaults off unless
overridden. CPU timings are inclusive and do not measure GPU execution. See the
[telemetry guide](../../docs/telemetry.md).
