# Scene Movement Authoring and Runtime Reuse

**Document ID:** RUNTIME-SCENE-MOVEMENT-GUIDE

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** authoring collision/character components, session lifecycle and in-game laboratory boundaries

**Surface:** portal

**Boundary:** runtime

**Authority role:** portal

**Mutation:** authored

**Indexing:** pointer-only

**Canonical:** docs/editor-movement.md

**Canonical scope:** pixieeditor

This portal preserves Engine links. The independent Editor owns the
[published baseline guide](https://github.com/Sakamoto0110/PixieEditor/blob/d1760e14549da826fecddec3550244123f9ea715/docs/editor-movement.md).

## Using the editor

See [the canonical Editor section](https://github.com/Sakamoto0110/PixieEditor/blob/d1760e14549da826fecddec3550244123f9ea715/docs/editor-movement.md#using-the-editor).

## Physics debug views

See [the canonical Editor section](https://github.com/Sakamoto0110/PixieEditor/blob/d1760e14549da826fecddec3550244123f9ea715/docs/editor-movement.md#physics-debug-views).

## Viewport collider handles

See [the canonical Editor section](https://github.com/Sakamoto0110/PixieEditor/blob/d1760e14549da826fecddec3550244123f9ea715/docs/editor-movement.md#viewport-collider-handles).

## Compound bodies

In the retained v3 diagnostic, **Make compound** on a selected static
Collider2D parent detached an unselected child. Undo restored the original
collider and left the child detached. The child kept its stable ID and world pose.

The [published Editor baseline section](https://github.com/Sakamoto0110/PixieEditor/blob/d1760e14549da826fecddec3550244123f9ea715/docs/editor-movement.md#compound-bodies)
predates this warning.

## Native lifecycle and ownership

The scene movement service synchronizes colliders in `PreUpdate` for gameplay
queries. In `Physics`, it refreshes geometry after legacy velocity integration
and completes body and motor movement before `PostUpdate`.
[Authored movement contract](modules/runtime/REFERENCE.md#authored-scene-movement)

The [published Editor baseline section](https://github.com/Sakamoto0110/PixieEditor/blob/d1760e14549da826fecddec3550244123f9ea715/docs/editor-movement.md#native-lifecycle-and-ownership)
predates this timing correction.

## Named collision layers

See [the canonical Editor section](https://github.com/Sakamoto0110/PixieEditor/blob/d1760e14549da826fecddec3550244123f9ea715/docs/editor-movement.md#named-collision-layers).

## An editor inside the game

See [the canonical Editor section](https://github.com/Sakamoto0110/PixieEditor/blob/d1760e14549da826fecddec3550244123f9ea715/docs/editor-movement.md#an-editor-inside-the-game).
