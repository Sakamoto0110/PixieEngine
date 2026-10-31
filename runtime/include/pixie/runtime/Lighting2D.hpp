#pragma once

#include "pixie/runtime/ComponentTypes.hpp"

#include <pixie/pxColorRgba.hpp>
#include <pixie/pxPoint.hpp>

#include <cstdint>
#include <numbers>
#include <string>
#include <vector>

namespace pixie {

// How the lit image is mapped to the display.
enum class LightToneMapping2D : std::uint8_t {
    // Clamped: values above 1 clip. With exposure 1, an unlit albedo comes out unchanged.
    None,
    // A filmic curve (Narkowicz's fit of ACES) that rolls bright light off instead of clipping.
    Aces,
};

// How much lighting work a frame may do: the most shadow-casting lights in view, 8, 32 or 64. Over
// the cap, the lights that contribute least are left out (drawing them without shadows would light
// through walls) and counted in telemetry.
enum class LightQuality2D : std::uint8_t { Low, Medium, High };

// Lighting for a scene: one per scene, on any entity, like PhysicsSettings2D. Without one, the scene
// is drawn unlit, exactly as before lighting existed, and its lights do nothing. Its defaults (white
// ambient at 1, exposure 1, no tone mapping) leave an unlit image unchanged; lower the ambient for
// darkness. See docs/lighting-l0-design.md.
struct LightingSettings2D {
    // Light every receiver gets, whatever its layers; authored in sRGB.
    pxColorRgba ambient{255, 255, 255};
    // Linear multiplier of the ambient color.
    double ambientIntensity = 1.0;
    // Linear multiplier of all light, before tone mapping.
    double exposure = 1.0;
    LightToneMapping2D toneMapping = LightToneMapping2D::None;
    // At most this many particle light splats a frame, over all pools, after each pool's cap.
    int maxParticleLights = 2048;
    LightQuality2D quality = LightQuality2D::Medium;
    // Optional effects (LIT-013), off by default. Ambient occlusion darkens the ambient light within
    // `occlusionRadius` world units outside occluder edges, by up to `occlusionStrength`.
    bool ambientOcclusion = false;
    double occlusionRadius = 24.0;
    double occlusionStrength = 0.6;
    // Fog toward `fogColor` (sRGB): none at world y `fogTop`, `fogDensity` at `fogBottom` and below
    // (Y down), applied before tone mapping.
    bool fog = false;
    pxColorRgba fogColor{110, 120, 140};
    double fogDensity = 0.4;
    double fogTop = 0.0;
    double fogBottom = 400.0;
    // Bloom: light above `bloomThreshold` spreads into a glow at `bloomIntensity`.
    bool bloom = false;
    double bloomThreshold = 1.0;
    double bloomIntensity = 0.5;
    // Normal maps (NormalMap2D) shade sprites by their relief, with lights `normalHeight` world units
    // above the plane.
    bool normalMaps = false;
    double normalHeight = 48.0;
    bool operator==(const LightingSettings2D&) const = default;
};

// Makes the entity's Sprite, Text or Shape glow: its image (a shape's flat area) times `color` times
// `intensity`, added unlit after the light, so it shows in darkness. Game code drives `intensity`
// for fades; the entity's logical state stays its own.
struct Emission2D {
    // Authored in sRGB.
    pxColorRgba color{255, 255, 255};
    // Linear multiplier, 0 to 16 (in steps of 1/16 of the 0-255 range).
    double intensity = 1.0;
    bool operator==(const Emission2D&) const = default;
};

// How a ParticleSystem's pool takes part in lighting, like ParticlePhysics2D for physics.
struct ParticleLight2D {
    // Drawn unlit, at the particles' own color, instead of lit like the world.
    bool emissive = false;
    // Each live particle lights its surroundings as an unshadowed point light of the particle's
    // color and alpha.
    bool illuminates = false;
    // World units.
    double lightRadius = 24.0;
    double lightIntensity = 1.0;
    // At most this many particles of the pool cast light, the first in the pool's order.
    int lightCap = 256;
    bool operator==(const ParticleLight2D&) const = default;
};

enum class LightType2D : std::uint8_t {
    // From the entity's position in every direction, out to its range.
    Point,
    // A point light limited to a cone around the entity's rotation.
    Spot,
    // The same light over the whole view, as from the sun or a window; its direction (the entity's
    // rotation) matters for shadows.
    Directional,
};

// A light at the entity's Transform2D, interpolated like drawing. Intensity is a linear multiplier:
// a white light of intensity 1 shows an albedo at its own brightness where it is strongest.
struct Light2D {
    LightType2D type = LightType2D::Point;
    // Authored in sRGB.
    pxColorRgba color{255, 255, 255};
    double intensity = 1.0;
    // World units; the light falls to exactly 0 there (point and spot).
    double range = 300.0;
    // The exponent p of the falloff saturate(1 - (d / range)^2)^p: 0 is flat to the edge, higher is
    // tighter around the center.
    double falloff = 2.0;
    // The cone's full angle, in radians (spot).
    double coneAngle = std::numbers::pi / 3.0;
    // The fraction of the cone's half-angle, from its edge inward, over which the light fades in.
    double coneSoftness = 0.2;
    // The light layers (bits 0 to 7) of the receivers this light reaches.
    int lightLayers = 0xFF;
    bool enabled = true;
    // Whether occluders of its shadow layers cast shadows from it.
    bool castsShadows = true;
    // The occluder layers (bits 0 to 7) that shadow it.
    int shadowLayers = 0xFF;
    // The light's size, which sets how soft its shadows are: a radius in world units (point and
    // spot); 0 gives hard shadows.
    double sourceRadius = 8.0;
    // The same for a directional light: its angular radius, in radians.
    double sourceAngle = 0.5 * std::numbers::pi / 180.0;
    bool operator==(const Light2D&) const = default;
};

enum class OccluderSource2D : std::uint8_t {
    // The entity's Collider2D or CompoundCollider2D shapes, or a CharacterMotor2D's capsule, so
    // shadows and physics agree.
    Collider,
    // `polygon`, in the entity's local units.
    Polygon,
    // The outline of the entity's Sprite where its image's alpha reaches `alphaThreshold`, traced
    // on a grid of at most 64 cells a side.
    SpriteAlpha,
};

// Makes the entity block light: its shape casts shadows from every light whose shadow layers
// include its layers, at the entity's interpolated transform. An occluder does not shadow its own
// interior, so its sprite is lit as if it were not there (other occluders still shadow it), unless
// `selfShadow` is set; then only a soft rim on the light's side is lit.
struct Occluder2D {
    OccluderSource2D source = OccluderSource2D::Collider;
    // A convex polygon in the entity's local units, before its scale, rotation and position.
    std::vector<pxPoint> polygon{{-16, -16}, {16, -16}, {16, 16}, {-16, 16}};
    // For the SpriteAlpha source: the alpha, 0 to 1, from which a cell of the image counts as solid.
    double alphaThreshold = 0.5;
    // The occluder layers (bits 0 to 7) it belongs to.
    int layers = 1;
    bool selfShadow = false;
    bool enabled = true;
    bool operator==(const Occluder2D&) const = default;
};

// How the entity's Sprite, Text or Shape receives light. Without one, it is in every light layer
// and lit.
struct LightReceiver2D {
    // The light layers (bits 0 to 7) it belongs to: a light reaches it when they share one. Ambient
    // light reaches it whatever its layers.
    int lightLayers = 0xFF;
    // Shown at its own color, ignoring light, exposure and tone mapping.
    bool unlit = false;
    bool operator==(const LightReceiver2D&) const = default;
};

// A normal map for the entity's Sprite: a PNG of the same size and region, in tangent space with green
// toward the image's top. With LightingSettings2D::normalMaps, lights shade the sprite by its relief,
// turning with the sprite; without a map, a surface keeps its light.
struct NormalMap2D {
    std::string image;
    // 0 ignores the map, 1 applies it fully.
    double strength = 1.0;
    bool operator==(const NormalMap2D&) const = default;
};

// One light layer's name; `slot` is the stable 1-based bit (1 to 8), never a list index.
struct LightLayer2D {
    int slot = 1;
    std::string name;
    bool operator==(const LightLayer2D&) const = default;
};

// Names for the scene's 8 light layers, one per scene, as CollisionLayers2D names collision layers.
// Names are for display only: renaming one changes no mask.
struct LightLayers2D {
    std::vector<ComponentRecord<LightLayer2D>> layers{{{1, "Default"}, {}}};
    bool operator==(const LightLayers2D&) const = default;
};

} // namespace pixie
