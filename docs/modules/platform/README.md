# Platform

**Document ID:** PLATFORM-README

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** introduction to the platform native boundary

**Surface:** introduction

**Boundary:** platform

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

Use Platform when a host needs a player window or needs to feed physical input into a fixed update. A player-owned `Window` creates and presents its GLFW context. A `HostSurface` carries a viewport size for a context managed by the caller.

- [Technical reference](REFERENCE.md) for contracts and lifecycle.
- [Manifest](MANIFEST.md) for targets, source and test routes.
