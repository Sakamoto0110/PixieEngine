#pragma once

#include "pixie/runtime/Export.hpp"

#include "pixie/render/2d/LightRenderer.hpp"
#include "pixie/runtime/Lighting2D.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/Transform2D.hpp"

#include <pixie/pxColorRgba.hpp>
#include <pixie/pxRect.hpp>

#include <cstdint>
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

// Replaces `edges` with the edges of every enabled Occluder2D of an entity with a Transform2D,
// interpolated `alpha` of the way from its previous transform to its current one.
PIXIE_RUNTIME_API void GatherOccluders(World& world, double alpha, std::vector<OccluderEdge>& edges);

// Whether any entity has a LightReceiver2D, so a frame needs the receiver pass.
PIXIE_RUNTIME_API bool HasLightReceivers(World& world);

// The color a receiver pass draws for `receiver` (null for the default: every layer, lit), with the
// drawn thing's own alpha: red its layers, green whether it is unlit, blue that alpha.
PIXIE_RUNTIME_API pxColorRgba ReceiverColor(const LightReceiver2D* receiver, std::uint8_t alpha);

} // namespace pixie
