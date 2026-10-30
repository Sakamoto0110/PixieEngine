# Render Technical Reference

**Document ID:** RENDER-REFERENCE

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** render ownership, dependencies and lifecycle

**Surface:** technical-reference

**Boundary:** render

**Authority role:** normative

**Mutation:** authored

**Indexing:** include

Render owns GL loading/wrappers, render targets, readback, View and the current 2D renderers. It depends on Core and graphics/image/font dependencies, never Platform or GLFW. It receives a current OpenGL 4.5 core context; context ownership, creation and presentation remain outside Render.

The root `pixie/render/` and `pixie/render/gl/` paths have no Camera2D dependency. View stores double-precision view/projection matrices, their projection-times-view product and viewport size. Camera2D lives under `pixie/render/2d/` and produces a View; 2D renderer Draw/Flush paths take that value. Picking/culling continue to use the 2D camera. Pixel-size expansion in the 2D path assumes an orthographic uniform-scale view; it is not a perspective footprint algorithm.

RenderTarget owns an RGBA8 color texture/FBO, not a Surface or context. Destroy GPU resources while their creating context is current. Bind sets the target viewport; BindDefault selects framebuffer zero and does not restore a previous viewport. ClearColor and SetViewport operate on current context/bindings. Screenshot readback produces top-row-first PNG output; the FBO texture's GL orientation is separate.

GLEW's loader/pointer state is embedded and exported once by the shared engine; consumers receive its headers/import declarations rather than a second static loader instance. The 2D overlay drawing helper belongs here, while console policy/text preparation remains in App. This split implements neither 2.5D nor 3D; depth attachment, sorting and perspective behavior need explicit future scope.

Out-of-line symbols use `PIXIE_RENDER_API`; public headers own exact signatures.

## Packaging and evidence

The internal object target is compiled with its lower-boundary dependencies only. The public consumer facade reaches the shared engine built by the root CMake target pixie_shared (output pixie.dll on Windows); this does not turn every included boundary into a permitted implementation dependency. Native C++ consumers must match toolchain/runtime settings. Core/Platform/Render/App ownership was established by the split; shared packaging is being qualified under [PIXIE-TODO-051](../../../TODO.md#pixie-todo-051--shared-engine-game-export-and-local-mcp-development-channel).

The [qualification matrix](../../shared-engine-export-validation.md) records bounded results and remaining gaps. A Debug build/test result does not establish Release, CI, Linux, export, MCP or final owner acceptance.
