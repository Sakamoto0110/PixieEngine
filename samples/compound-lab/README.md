# Compound Body Laboratory

**Document ID:** SAMPLE-COMPOUND-LAB-README

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** no-code compound geometry, local shape poses and shared body motion

**Surface:** introduction

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

Open this folder in PixieEditor, or run:

```text
editor/PixieEditor/bin/Release/net10.0/PixieEditor.exe --project samples/compound-lab
bin/Release/pixie-player.exe --project samples/compound-lab
```

This is a separate save derived from physics-lab; that original room is unchanged.
The **compound bracket** has one RigidBody2D and two boxes in CompoundCollider2D.
Its L-shaped Path is a drawable outline matching the initial box union. Play drops
and rotates it against the same room's floor/ramp; Stop restores authored data.

Select the bracket and expand **CompoundCollider2D**, then **Shape 1/2**. Each item
uses the normal Collider2D fields: local offset/rotation, full size, material,
density and two-sided category/mask. Add shape appends an item; the item's close
button removes it. Each action is undoable. A sensor contributes no mass. Overlapping
solid shapes contribute their masses separately; there is no mass union calculation.

The orange Path is authored visual data, independent of physics. Editing collider
geometry updates the blue outlines and actual collision; it does not redraw the
Path automatically. Edit the Path too when changing this sample's visible contour.
In a new project, **Physics... > Compound box** creates a ready dynamic compound.
**Make compound** converts an existing Collider2D while retaining its body and data.

This room does not qualify character pushing/carrying, crushing or accepted game
feel. See the [workflow](../../docs/editor-movement.md) and
[physics requirements](../../docs/rigid-body-physics-requirements.md) for remaining scope.
