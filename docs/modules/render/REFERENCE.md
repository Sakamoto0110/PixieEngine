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

LightRenderer lights a 2D frame. `BeginWorld` remembers the bound draw framebuffer and viewport and
binds its own RGBA8 albedo target, cleared to the background, which the other 2D renderers draw into
unchanged; `BeginReceivers` switches drawing to the receiver target (per pixel: light layers in red,
unlit in green; cleared to every layer, lit), which SpriteRenderer and ShapeRenderer fill with their
receiver pass (`SetReceiverPass`), a second shader that writes the instance color's receiver where
coverage reaches one half. `Finish` clears an RGBA16F light buffer to the linear ambient, adds
point, spot and directional `LightInstance`s (64 bytes each, colors linear) where their layers meet
the receiver's, then draws `encode(tonemap(exposure * decode(albedo) * light))`, blended back to
the albedo by the unlit weight, into the remembered framebuffer with the albedo's alpha. Lights
with shadow layers are drawn one by one when `Finish` gets `OccluderEdge`s: within the light's
pixel box, every edge of its shadow layers that faces away from it (toward it, for a self-shadowing
occluder) becomes a quad over its umbra and penumbra, whose fragments add the fraction of the
source's angular extent the edge hides into an R32F occlusion mask; the light then multiplies by
one minus that, clamped. Directional lights extrude along their direction with an angular source.
Overlapping penumbrae of separate occluders add, so their crossings are slightly too dark.
`BeginEmission` switches drawing to an RGBA16F emission target of linear radiance, cleared to zero,
which the combine adds to `albedo * light` before exposure. SpriteRenderer and ShapeRenderer choose
their program by `LightingPass` (`SetLightingPass`): color, receivers, or emission, where the
instance color holds an emissive tint and intensity / 16 in alpha; ParticleRenderer's emission
pass (`SetEmissionPass`) writes the particles' color decoded to linear. Each pass is a separate
shader program, so the color pass's shaders are those of unlit drawing. `Finish` writes GPU
timestamps around light accumulation, the shadowed lights and the combine into one of four query
sets in turn, and reads a set only when it is reused and its results have arrived, so it never
waits for the GPU; `GpuTimes` holds the latest arrival, numbered by `sample`. The sRGB
curves are exact (`DecodeSrgb`, `EncodeSrgb`). Without lights and with ambient 1, exposure 1 and no
tone mapping, the result equals the albedo byte for byte on the qualified renderers.

Out-of-line symbols use `PIXIE_RENDER_API`; public headers own exact signatures.

## Packaging and evidence

The internal object target is compiled with its lower-boundary dependencies only. The public consumer facade reaches the shared engine built by the root CMake target pixie_shared (output pixie.dll on Windows); this does not turn every included boundary into a permitted implementation dependency. Native C++ consumers must match toolchain/runtime settings. Core/Platform/Render/App ownership was established by the split; shared packaging is being qualified under [PIXIE-TODO-051](../../../TODO.md#pixie-todo-051--shared-engine-game-export-and-local-mcp-development-channel).

The [qualification matrix](../../shared-engine-export-validation.md) records bounded results and remaining gaps. A Debug build/test result does not establish Release, CI, Linux, export, MCP or final owner acceptance.
