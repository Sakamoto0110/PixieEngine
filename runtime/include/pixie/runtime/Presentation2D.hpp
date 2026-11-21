#pragma once

#include "pixie/runtime/Export.hpp"
#include <pixie/pxColorRgba.hpp>
#include <cstdint>
#include <optional>
#include <string>

namespace pixie {
class World;
struct LightingSettings2D;

enum class LightingState2D { Absent, Disabled, Configured };
struct EnvironmentLighting2D {
    pxColorRgba ambient{255, 255, 255};
    double ambientIntensity = 1;
    bool fog = false;
    pxColorRgba fogColor{110, 120, 140};
    double fogDensity = 0.4;
    double fogTop = 0;
    double fogBottom = 400;
    bool operator==(const EnvironmentLighting2D&) const = default;
};
struct SceneEnvironment2D {
    bool overrideWorld = false;
    EnvironmentLighting2D value;
    bool operator==(const SceneEnvironment2D&) const = default;
};
enum class ViewToneMapping2D { None, Aces };
struct ViewLighting2D {
    double exposure = 1;
    ViewToneMapping2D toneMapping = ViewToneMapping2D::None;
    bool bloom = false;
    double bloomThreshold = 1;
    double bloomIntensity = 0.5;
    bool operator==(const ViewLighting2D&) const = default;
};
enum class RendererQuality2D { Low, Medium, High };
struct RendererLighting2D {
    RendererQuality2D quality = RendererQuality2D::Medium;
    int maxParticleLights = 2048;
    bool operator==(const RendererLighting2D&) const = default;
};
// Retained compatibility values. Their final ownership is not yet approved; new UI
// controls do not edit them. They remain active exactly as in the v1 adapter.
struct LegacyLightingEffects2D {
    bool ambientOcclusion = false;
    double occlusionRadius = 24;
    double occlusionStrength = 0.6;
    bool normalMaps = false;
    double normalHeight = 48;
    bool operator==(const LegacyLightingEffects2D&) const = default;
};
// Authored defaults for the primary view and renderer live in the same document,
// but remain separate domains. Presence never implicitly enables lighting.
struct WorldPresentation2D {
    LightingState2D lighting = LightingState2D::Absent;
    EnvironmentLighting2D environment;
    ViewLighting2D view;
    RendererLighting2D renderer;
    LegacyLightingEffects2D legacyEffects;
    std::uint64_t camera = 0; // Stable entity SceneId; zero means session fallback.
    bool operator==(const WorldPresentation2D&) const = default;
};
// An authored camera's center comes from Transform2D.position. Rotation/scale have
// no camera meaning; unsupported projection policies are not inferred.
struct CameraComponent2D {
    double zoom = 1;
    bool enabled = true;
};

PIXIE_RUNTIME_API std::string ValidatePresentation2D(const WorldPresentation2D& value);
PIXIE_RUNTIME_API std::string ValidateEnvironment2D(const EnvironmentLighting2D& value);
PIXIE_RUNTIME_API WorldPresentation2D ReadPresentation2D(const World& world);
PIXIE_RUNTIME_API LightingSettings2D ResolveLightingSettings2D(const World& world, std::uint64_t scene);
// Small owned read surface for editor bindings; never the complete entity snapshot.
PIXIE_RUNTIME_API std::string PresentationSnapshot2D(const World& world);
} // namespace pixie
