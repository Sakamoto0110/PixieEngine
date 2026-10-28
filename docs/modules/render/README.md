# Render

**Document ID:** RENDER-README

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** introduction to the render native boundary

**Surface:** introduction

**Boundary:** render

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

Render owns GL loading/wrappers, render targets, readback, View and the current 2D renderers. It depends on Core and graphics/image/font dependencies, never Platform or GLFW. It receives a current OpenGL 4.5 core context; context ownership, creation and presentation remain outside Render.

Read the [manifest](MANIFEST.md) for source/build routes and the [technical reference](REFERENCE.md) for ownership and lifecycle. Public declarations remain in `engine/render/include/pixie/render/`.

