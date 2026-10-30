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

Platform owns the player window, context creation/currentness/presentation, input data and frame limiting. It may depend on Core and GLFW; it must not include or link the Render implementation.

Read the [manifest](MANIFEST.md) for source/build routes and the [technical reference](REFERENCE.md) for ownership and lifecycle. Public declarations remain in `engine/platform/include/pixie/platform/`.
