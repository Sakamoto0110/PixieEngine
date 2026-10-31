#pragma once

#include "pixie/runtime/Export.hpp"

#include "pixie/render/2d/Camera2D.hpp"
#include "pixie/render/2d/LightRenderer.hpp"
#include "pixie/render/2d/ParticleRenderer.hpp"
#include "pixie/render/2d/ShapeRenderer.hpp"
#include "pixie/runtime/Lighting2D.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/Sprite.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

#include <pixie/pxColorRgba.hpp>
#include <pixie/pxRect.hpp>

#include <cstdint>
#include <filesystem>
#include <map>
#include <span>
#include <string>
#include <vector>

namespace pixie {

class World;

// The scene's LightingSettings2D (one per scene); null when it has none, which leaves it unlit.
PIXIE_RUNTIME_API const LightingSettings2D* FindLightingSettings(World& world);

// What the LightRenderer combines with for these settings: the ambient decoded from sRGB and
// multiplied by its intensity, the exposure and the tone mapping.
PIXIE_RUNTIME_API LightingParameters MakeLightingParameters(const LightingSettings2D& settings);

// The LightRenderer's form of one light at `transform`: color decoded from sRGB and multiplied by
// the intensity, direction from the rotation, cone as cosines, layers kept to bits 0 to 7.
PIXIE_RUNTIME_API LightInstance MakeLightInstance(const Light2D& light, pxPoint position, double rotation);

// Replaces `lights` with every enabled Light2D of an entity with a Transform2D, interpolated `alpha`
// of the way from its previous transform to its current one, in the World's order; with `view`,
// only the point and spot lights whose range reaches it.
PIXIE_RUNTIME_API void GatherLights(World& world, double alpha, const pxRect* view, std::vector<LightInstance>& lights);

// The convex pieces an occluder casts from at `transform`, in world units, each winding with a
// positive signed area: its polygon, or with the Collider source the enabled, solid shapes of
// `collider` or `compound`, or the upright capsule of `motor`. Circles and capsule caps become
// polygons whose chords stay within 0.5 world units of the curve (8 to 32 segments per circle).
PIXIE_RUNTIME_API std::vector<std::vector<pxPoint>> OccluderPolygons(const Occluder2D& occluder,
                                                                    const Transform2D& transform,
                                                                    const Collider2D* collider,
                                                                    const CompoundCollider2D* compound,
                                                                    const CharacterMotor2D* motor);

// One edge of a sprite's alpha outline, in the sprite's own units (0 to 1 across its drawn
// rectangle, Y down), wound so that its outward normal is (b - a) turned to (dy, -dx).
struct OutlineEdge {
    pxPoint a;
    pxPoint b;
};

// The alpha outlines of sprite images, traced once per image, region and threshold and kept. Images
// are read from the project folder; one that cannot be read has no outline. A changed file is read
// again only after Clear.
class SpriteAlphaOutlines {
public:
    void SetFolder(const std::filesystem::path& folder) {
        folder_ = folder;
        Clear();
    }
    void Clear() { traced_.clear(); }
    // The outline of `sprite`'s image (its region, when it has one) where alpha reaches
    // `threshold`, and the drawn size in image pixels.
    PIXIE_RUNTIME_API const std::vector<OutlineEdge>& Edges(const Sprite& sprite, double threshold, pxVec2& pixelSize);

    // The outline of an RGBA image of `width` x `height`, first row at the top, on a grid of at most
    // `cells` cells a side: the edges between solid and empty cells, merged along rows and columns.
    PIXIE_RUNTIME_API static std::vector<OutlineEdge> Trace(int width, int height, std::span<const std::uint8_t> rgba,
                                                            double threshold, int cells = 64);

private:
    struct Traced {
        pxVec2 pixelSize;
        std::vector<OutlineEdge> edges;
    };
    std::filesystem::path folder_;
    std::map<std::string, Traced> traced_;
};

// Replaces `edges` with the edges of every enabled Occluder2D of an entity with a Transform2D,
// interpolated `alpha` of the way from its previous transform to its current one. SpriteAlpha
// occluders need `outlines`; without it they cast nothing.
PIXIE_RUNTIME_API void GatherOccluders(World& world, double alpha, std::vector<OccluderEdge>& edges,
                                       SpriteAlphaOutlines* outlines = nullptr);

// Appends a light splat for each live particle of every pool whose ParticleLight2D illuminates,
// interpolated `alpha` of the way between steps: unshadowed point lights of the particle's color,
// times its alpha (faded with age when the pool fades) and the pool's intensity. Each pool casts at
// most its lightCap, in pool order; all pools together at most `limit`. With `view`, only splats that
// reach it count. Returns how many were left out by the caps.
PIXIE_RUNTIME_API int GatherParticleLights(World& world, double alpha, const pxRect* view, int limit,
                                           std::vector<LightInstance>& lights);

// The color an emission pass draws for `emission` (null for none, which still covers what glows
// below): its tint, with intensity / 16 in alpha.
PIXIE_RUNTIME_API pxColorRgba EmissionColor(const Emission2D* emission);

// What entity `id` draws in a receiver or emission pass (ReceiverColor, EmissionColor), with its own
// alpha `alpha`.
PIXIE_RUNTIME_API pxColorRgba PassColor(const entt::registry& registry, entt::entity id, std::uint8_t alpha,
                                        LightingPass pass);

// Whether any entity has an Emission2D or any pool is emissive, so a frame needs the emission pass.
PIXIE_RUNTIME_API bool HasEmission(World& world);

// Draws the ParticleSystem pools whose ParticleLight2D is `emissive` (pools without one are not).
PIXIE_RUNTIME_API void DrawParticlePools(World& world, ParticleRenderer& renderer, const Camera2D& camera,
                                         double alpha, bool emissive);

// The most shadow-casting lights a frame draws at `quality`: 8, 32 or 64.
PIXIE_RUNTIME_API int ShadowedLightCap(LightQuality2D quality);

// Keeps at most `cap` shadow-casting lights in `lights`, those that contribute most (their summed
// color times the area they cover in `view`; directional lights cover all of it), and removes the
// others, keeping the order of the rest. Returns how many it removed.
PIXIE_RUNTIME_API int CapShadowedLights(std::vector<LightInstance>& lights, int cap, const pxRect& view);

// Debug drawing for lighting, over the frame and never lit: light bounds (a ring at each point or
// spot light's range, a spot's cone edges, a directional light's direction) and occluder edges.
inline constexpr std::uint32_t kLightingDebugLights2D = 1u;
inline constexpr std::uint32_t kLightingDebugOccluders2D = 2u;
PIXIE_RUNTIME_API void DrawLightingDebug2D(std::span<const LightInstance> lights, std::span<const OccluderEdge> occluders,
                                           ShapeRenderer& renderer, const Camera2D& camera, std::uint32_t flags);

// The display names of light layer slots 1 to 8 (index 0 to 7): the scene's LightLayers2D names,
// "Default" for slot 1 without one, "Light layer N" for the rest. A duplicate configuration or slot
// leaves the affected names unnamed.
PIXIE_RUNTIME_API std::array<std::string, 8> LightLayerNames(const World& world);

// Whether any entity has a LightReceiver2D, so a frame needs the receiver pass.
PIXIE_RUNTIME_API bool HasLightReceivers(World& world);

// The color a receiver pass draws for `receiver` (null for the default: every layer, lit), with the
// drawn thing's own alpha: red its layers, green whether it is unlit, blue that alpha.
PIXIE_RUNTIME_API pxColorRgba ReceiverColor(const LightReceiver2D* receiver, std::uint8_t alpha);

} // namespace pixie
