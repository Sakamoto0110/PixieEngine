# Movement Laboratory Project

**Document ID:** SAMPLE-MOVEMENT-LAB-README

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** editable no-code collision and character scene

**Surface:** introduction

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

Open this folder through the editor's File/Open project, or run:

```text
editor/PixieEditor/bin/Release/net10.0/PixieEditor.exe --project samples/movement-lab
```

It uses the generic version-9 scene library, with no project C++ code. Four
static Collider2D entities form the floor, ramp, wall and ceiling. The character
has CharacterMotor2D and KeyboardControl2D. Shape components are placeholder art;
Edit gizmos show actual collision geometry.

Move/configure entities in Edit, Save, then Play. Focus the viewport for A/D or
arrows and Space. Stop restores the pre-play authored scene. Collider/Character
buttons and viewport creation menu entries add new objects as single undo steps.
To experiment without changing this sample, copy the project folder first.

The native equivalent runs through:

```text
bin/Release/pixie-player.exe --project samples/movement-lab
```

See [authoring and runtime reuse](../../docs/editor-movement.md) for field rules,
diagnostics, ownership and the separate future in-game laboratory UI.
