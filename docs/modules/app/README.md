# App

**Document ID:** APP-README

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** introduction to the app native boundary

**Surface:** introduction

**Boundary:** app

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

App composes Core timing/telemetry, Platform context/input/presentation and Render drawing. It owns App, Run, run configuration/options, the native debug console and overlay policy. Lower boundaries never depend on App.

Read the [manifest](MANIFEST.md) for source/build routes and the [technical reference](REFERENCE.md) for ownership and lifecycle. Public declarations remain in `engine/app/include/pixie/app/`.

