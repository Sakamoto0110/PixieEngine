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

Render owns GL loading and wrappers, framebuffers, readback, `View` and 2D drawing. It uses Core, GLEW, OpenGL and private stb_truetype. Platform and GLFW are outside its dependency boundary. The host supplies context activation and presentation. The root `pixie/render/` and `pixie/render/gl/` headers are independent of `Camera2D`; camera and 2D drawing live under `pixie/render/2d/`. Out-of-line symbols use `PIXIE_RENDER_API`; declarations own exact signatures.

## Context and GPU ownership

Make an OpenGL 4.5 core context current on the calling thread, then call `LoadGl` before any Render GL operation. It returns false on GLEW initialization failure, logs driver identity on success, and installs GL logging for debug contexts. GLEW pointer state is embedded and exported once by the shared engine. Consumers use its headers and import declarations. Include `Gl.hpp` before other GL headers when using GL directly.

Create, use and destroy GPU resources with the appropriate context current. `GlBuffer`, `GlVertexArray`, `ShaderProgram` and `Texture` own their GL names. Their destructors assert that explicit teardown happened. Moving transfers the GL name, and move assignment requires an empty destination. Raw IDs are borrowed for the life of the current resource. If a renderer's multi-stage `Create` returns false, destroy it on the GL thread to clean up any earlier stages. In a player, release application GPU resources in `App::Stop`. In an embedded session, release them before giving up the host context. See [App teardown](../app/REFERENCE.md#player-loop-and-teardown) and [host context ownership](../platform/REFERENCE.md#window-and-host-contexts).

`GlBuffer::Upload` replaces the full store from bytes or a typed span. `GlVertexArray` associates byte stride and offset with per-vertex or per-instance bindings. `Attribute` feeds float inputs, and `IntegerAttribute` feeds integer inputs. `ShaderProgram::Create` compiles and links vertex/fragment sources and logs shader/link failures. `Texture::Create` copies a tightly packed CPU `Image` into RGBA8 with a full mip chain. Upload assumes a complete RGBA8 buffer and default pixel-unpack row/skip state with no unpack buffer bound. The sprite renderer selects sampling.

## Targets and readback

`RenderTarget` owns one RGBA8 texture and its framebuffer. It has no depth attachment. `Create` replaces existing storage and returns false for a nonpositive size or incomplete framebuffer. `Resize` reuses a valid same-size target and otherwise replaces it. Creation failure leaves no valid target. A successful resize to a different size invalidates cached texture/framebuffer IDs.

`Bind` selects the target for reads and draws and sets its viewport. `BindDefault` selects framebuffer zero for both and leaves the viewport as-is. `SetViewport` sets an origin-zero viewport in framebuffer pixels. `ClearColor` clears the color buffer on the current draw target. The FBO texture starts at the GL bottom row. Flip V when displaying it in a top-row-first UI. CPU-uploaded sprite images start at their top row, at V=0.

`SaveFramebufferPng` reads RGBA from the current read framebuffer at origin zero. Call it after drawing and before swapping, with positive pixel dimensions and client-memory pack state (no pixel-pack buffer and default row length/skip). It changes pack alignment to 1, reverses the row order and writes opaque alpha. An empty framebuffer or file-write failure is logged and returns false. The caller selects the intended read framebuffer.

## View and coordinates

`View` stores double-precision view and projection matrices, their projection-times-view product and a physical viewport size. The matrix getters borrow from the value. It is a snapshot, so rebuild it after camera or viewport changes. It carries no GL ownership. Renderers narrow matrices to floats for uniforms. The current 2D renderers use the orthographic path; pixel-size expansion assumes a uniform scale. Perspective footprints, depth and 3D sorting are outside that path.

`Camera2D` maps its world-space center to the middle of its viewport. Screen coordinates start at the framebuffer's top left, Y down, and the 2D world is Y down. A positive rotation from +X toward +Y looks clockwise. `Zoom` is framebuffer pixels per world unit (default 1, clamped to [1e-4, 1e4] for finite input). `ScreenToWorld` and `WorldToScreen` convert for picking and presentation. `VisibleWorld` is the axis-aligned world rectangle for culling. `Pan` takes pixel motion and moves the view so the world follows the cursor. `ZoomAt` changes zoom while preserving the cursor's world anchor. Set a positive drawable viewport before producing a `View` with `MakeView`.

## 2D drawing and batching

The host binds the destination and selects the viewport. The world, camera and authored component state belong to the caller. Render consumes views, spans and instance values. Draw passes can change GL state, so compose them in the intended draw order and reestablish caller-specific state before the next pass.

| Family | Contract |
| --- | --- |
| `ShapeRenderer` | Draw an immediate span of one `ShapeKind`, or submit copied instances and flush by kind in enum order. Positions are in world units, rotation is radians and scale gives half-extents. A ring uses a screen-space outline. `Flush` consumes the queued instances. `Destroy` releases the GL resources and leaves queued CPU instances alone. |
| `SpriteRenderer` | `Submit` copies the instance and borrows the texture name through flush. Sprites use full world-space size, radians, normalized pivot and UVs, and a straight-alpha tint on a premultiplied texture. Flush keeps ascending layer order and insertion order within a layer. Only adjacent compatible texture/filter entries batch together. `belowLayer` is exclusive, including the default `INT_MAX`. Higher/equal entries stay queued, so use lower layer values for ordinary drawing. |
| `PathRenderer` | Submit a polyline with full width in world units and a fill fraction clamped to [0, 1]. It copies per-segment capsule data, preserving end points and the distance along the line for the fill. Flush consumes the queue. `RoundCorners` and `PolylineLength` are CPU geometry helpers. Minimum visible width assumes the 2D view scale. |
| `ParticleRenderer` | Draw uploads a span of 40-byte `Particle` values and interpolates previous/current position on the GPU using alpha. Position and radius use world units, velocity uses world units per second, and age/lifetime use seconds. Lifetime zero is indefinite. Blend, fade and minimum screen size apply to the pool. The particles carry birth-time color, radius and shape. Simulation and physics are caller-owned. |
| `Font` and `TextRenderer` | Load TrueType outlines into CPU-owned bytes. UTF-8 layout uses cached em-size atlases and supports U+0020 through U+00FF plus the replacement glyph. Atlas sizes clamp to 4 through 256 pixels. Text uses a Y-down top-left anchor and borrowed glyph pointers into the atlas. `TextRenderer` uploads the atlas as a texture and submits glyph sprites into the caller's batch. Keep queued textures alive through flush before calling `Forget` or `Destroy`. World text uses a size ladder to bound atlas churn during zoom. |
| `OverlayVertex` and `DrawOverlay` | App owns overlay policy and geometry. Render draws top-left pixel-space triangles to default draw framebuffer zero, restoring the state it changes. The caller owns the program, buffer and vertex array created by `CreateOverlayDrawing`. |

## Lighting passes

With a created `LightRenderer`, pass a positive size to `BeginWorld` and use the same drawable dimensions for `View`. Select the corresponding renderer pass before drawing each attachment. Return sprite/shape drawing to `LightingPass::Color` and turn off particle emission when drawing the next ordinary pass. Call `Finish` before using the composed result. It binds the saved draw target for both reading and drawing and restores its viewport. Destroy the lighting resources while the context is current.

LightRenderer lights a 2D frame. `BeginWorld` remembers the bound draw framebuffer and viewport and
binds its own RGBA8 albedo target, cleared to the background, which the other 2D renderers draw into
unchanged; `BeginReceivers` switches drawing to the receiver target (per pixel: light layers in red,
unlit in green; cleared to every layer, lit), which SpriteRenderer and ShapeRenderer fill with their
receiver pass (`SetLightingPass(LightingPass::Receivers)`), a separate shader that writes the instance color's receiver where
coverage reaches one half. `Finish` clears an RGBA16F light buffer to the linear ambient, adds
point, spot and directional `LightInstance`s (64 bytes each, colors linear) where their layers meet
the receiver's, then draws `encode(tonemap(exposure * decode(albedo) * light))`, blended back to
the albedo by the unlit weight, into the remembered framebuffer with the albedo's alpha. Lights
with shadow layers are drawn one by one when `Finish` gets `OccluderEdge`s: within the light's
pixel box, every edge of its shadow layers that faces away from it (toward it, for a self-shadowing
occluder) becomes a quad over its umbra and penumbra, whose fragments add the fraction of the
source's angular extent the edge hides into an R32F occlusion mask; the light then multiplies by
one minus that, clamped. Directional lights extrude along their direction with an angular source. With
`halfResolutionShadows` the masks are drawn into a texture of half the size and sampled with
linear filtering, clamped at its edges, which costs less and softens shadow edges by a pixel.
Overlapping penumbrae of separate occluders add, so their crossings are slightly too dark.
`BeginEmission` switches drawing to an RGBA16F emission target of linear radiance, cleared to zero,
which the combine adds to `albedo * light` before exposure. SpriteRenderer and ShapeRenderer choose
their program by `LightingPass` (`SetLightingPass`): color, receivers, or emission, where the
instance color holds an emissive tint and intensity / 16 in alpha; ParticleRenderer's emission
pass (`SetEmissionPass`) writes the particles' color decoded to linear. Each pass is a separate
shader program, so the color pass's shaders are those of unlit drawing.

Optional effects default off. Ambient occlusion draws a quad around each occluder edge whose
fragments keep, with max
blending, `1 - smoothstep(0, radius, distance)` on the edge's outer side, and the combine removes that
share of the ambient; fog blends toward its color by world y; bloom writes the linear radiance,
keeps what exceeds the threshold at half size, blurs it with two 9-tap Gaussian passes and adds it
before tone mapping; normal maps come from a fourth world target (`BeginNormals`, the sprite
renderer's `LightingPass::Normals`) and weight each light by the angle it arrives from at
`normalHeight`. `Finish` writes GPU
timestamps around light accumulation, the shadowed lights and the combine into one of four query
sets in turn, and reads a set only when it is reused and its results have arrived, so it never
waits for the GPU; `GpuTimes` holds the latest arrival, numbered by `sample`. The sRGB
curves are exact (`DecodeSrgb`, `EncodeSrgb`). With no lights or optional effects, ambient 1,
exposure 1 and no tone mapping, the result equals the albedo byte for byte on the qualified renderers.

## Packaging and evidence

The internal object target is compiled with its lower-boundary dependencies only. The public consumer facade reaches the shared engine built by the root CMake target pixie_shared (output pixie.dll on Windows); this does not turn every included boundary into a permitted implementation dependency. Native C++ consumers must match toolchain/runtime settings. Core/Platform/Render/App ownership was established by the split; shared packaging is being qualified under [PIXIE-TODO-051](../../archive/todo-051-shared-engine-export-mcp-2026-11-01.md#pixie-todo-051--shared-engine-game-export-and-local-mcp-development-channel).

The [qualification matrix](../../shared-engine-export-validation.md) records bounded results and remaining gaps. A Debug build/test result does not establish Release, CI, Linux, export, MCP or final owner acceptance.
