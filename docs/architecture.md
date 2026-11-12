# PixieEngine architecture

**Document ID:** GLOBAL-ENGINE-ARCHITECTURE
**Schema version:** 1
**Kind:** guide
**Lifecycle:** current
**Subject:** native host, World, rendering and delivery boundaries
**Surface:** guide
**Boundary:** global
**Authority role:** non-normative
**Mutation:** authored
**Indexing:** include

A game supplies a Project and its component types. The Runtime runs the World.
A native player drives it through App and Platform. An embedded host drives
a native session through the C API.

## Host and player

    Native player -> Run -> Platform Window + Input
                             -> ProjectApp / ProjectSession -> World
    Embedded host -> current GL context + HostSurface
                  -> C API session -> ProjectSession -> World

Core provides CPU timing, logs, images and telemetry. Platform owns the
player window/context, input and frame limiting. Render owns GPU wrappers,
framebuffers and drawing. In the native player, App orders fixed steps,
drawing and presentation. Game GPU resources are released through
`App::Stop` before the player window destroys its GL context. In an embedded
session the host supplies the current context and releases it after native
GPU resources have been released.

[Core timing](modules/core/REFERENCE.md#time-and-framebuffer-dimensions) ·
[Window and host contexts](modules/platform/REFERENCE.md#window-and-host-contexts) ·
[GPU ownership](modules/render/REFERENCE.md#context-and-gpu-ownership) ·
[Player loop and teardown](modules/app/REFERENCE.md#player-loop-and-teardown)

## World and authored content

    Project manifest + scene -> candidate World -> Project::Build
                             -> install scene services -> publish session
    Published World -> PreUpdate -> Behaviours -> Physics -> PostUpdate
                    -> deferred destruction/actions -> advance clocks

`ProjectSession` owns the published World. Play builds from an authored snapshot
and Stop rebuilds from it. A new World invalidates borrowed entities,
component pointers and physics handles. Resolve the new live entity by
`SceneId` when it still exists. A logical Scene scope is a separate identity.
The step contract covers hierarchy refreshes and phase failures. On first
dispatch, Behaviour `Start` runs before eligible input callbacks and Update.

[Projects and sessions](modules/runtime/REFERENCE.md#projects-and-sessions) ·
[Entity identity](modules/runtime/REFERENCE.md#entity-identity-and-lifetime) ·
[World step](modules/runtime/REFERENCE.md#the-step) ·
[Behaviour lifecycle](modules/runtime/REFERENCE.md#behaviours)

## Motion and presentation

An ordinary visual entity combines `Transform2D` with Shape, Sprite, Text
or Path. For collision geometry, choose Collider2D or CompoundCollider2D.
An optional RigidBody2D owns body motion. Use CharacterMotor2D with its own
capsule and a command source for an upright character. The normal velocity
integrator leaves body- and motor-owned poses to the scene movement service.
Named layers describe the two-sided category/mask filter. Attached child poses
are local to their parent and have component restrictions. Authored links
use v3. v1 and v2 retain their own scene families.

The Runtime composes drawing, lighting and particles before Render submits
work to the current GL context. A particle spawner needs a target particle
pool, and installed scene services advance supported effects. The world
camera maps world units to viewport pixels. Use the screen layer for
HUD content in viewport pixels.

[Authored scene movement](modules/runtime/REFERENCE.md#authored-scene-movement) ·
[Parenting](modules/runtime/REFERENCE.md#2d-parenting) ·
[Scene families](modules/runtime/REFERENCE.md#scene-families) ·
[Drawing and lighting](modules/runtime/REFERENCE.md#drawing) ·
[Particles](modules/runtime/REFERENCE.md#particles) ·
[Render coordinates](modules/render/REFERENCE.md#view-and-coordinates)

## Reflection and delivery

    Project::Describe -> component descriptors -> scene codec and C API
                                              -> Editor Inspector fields
    Installed authoring SDK -> generated game module -> Editor session
    Paired shipping profile -> exporter -> standalone player and assets

The Engine owns type metadata and field access through the native boundary.
The Editor owns how it constructs and rebinds its Inspector. An installed
authoring SDK supplies the game build and module contract. Export uses its
paired shipping runtime and exporter. Use the selected SDK's identity and
installed declarations when checking compatibility.

[Reflection](modules/runtime/REFERENCE.md#reflection) ·
[C API identity and profiles](modules/capi/REFERENCE.md#sdk-identity-and-profiles) ·
[Installed SDK](development.md#installed-sdk-and-local-rehearsal) ·
[C++ generation](entity-input-and-code-components.md) ·
[Native export](game-export.md#command-and-build-separation) ·
[Editor C++ authoring](https://github.com/Sakamoto0110/PixieEditor/blob/d1760e14549da826fecddec3550244123f9ea715/docs/code-authoring.md)

## Contributor entry points

Use the [filesystem map](../FILESYSTEM.md) to find source ownership and tests.
Use the [public API map](handbook/api-map.md) to find installed declarations.
