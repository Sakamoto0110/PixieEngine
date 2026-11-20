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

Choose **Open project** on the Editor start screen and select this folder, or run:

```text
C:/tools/PixieEditor/PixieEditor.exe --sdk C:/sdk/public --project samples/movement-lab
```

It uses the selected SDK's built-in authoring runtime without project C++ code. Four
static Collider2D entities form the floor, ramp, wall and ceiling. The character
has CharacterMotor2D and KeyboardControl2D. Shape components are placeholder art;
Edit gizmos show actual collision geometry.

Move and configure entities in Editing, save, then Play. Select **Game input**
and focus the viewport for A/D or arrows and Space. Stop restores the pre-play
authored scene. **Entities > + Create** and the viewport creation menu add
presets as single history steps. The sample's saved character uses the legacy
KeyboardControl2D mapping. The current **Character motor starter** uses generic
KeyboardControl and needs game code to supply motor commands.
To experiment without changing this sample, copy the project folder first.

The native equivalent runs through:

```text
bin/Release/pixie-player.exe --project samples/movement-lab
```

See [authoring and runtime reuse](../../docs/editor-movement.md) for field rules,
diagnostics, ownership and the separate future in-game laboratory UI.
