# Core

**Document ID:** CORE-README

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** introduction to the core native boundary

**Surface:** introduction

**Boundary:** core

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

Core owns logging, explicit frame time and fixed stepping, rate sampling, CPU telemetry, and CPU image decoding/encoding. It does not own a window, input routing, GL context, GPU resource or simulation World.

Read the [manifest](MANIFEST.md) for source/build routes and the [technical reference](REFERENCE.md) for ownership and lifecycle. Public declarations remain in `engine/core/include/pixie/core/`.

