#include "pixie/runtime/Presentation2D.hpp"
#include "pixie/runtime/Lighting2D.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/World.hpp"
#include "pixie/runtime/Name.hpp"
#include "PresentationJson.hpp"
#include <cmath>

namespace pixie {
std::string ValidateEnvironment2D(const EnvironmentLighting2D& value) {
    if (!std::isfinite(value.ambientIntensity) || value.ambientIntensity < 0 || value.ambientIntensity > 100 ||
        !std::isfinite(value.fogDensity) || value.fogDensity < 0 || value.fogDensity > 1 ||
        !std::isfinite(value.fogTop) || !std::isfinite(value.fogBottom)) return "invalid Environment lighting values";
    return {};
}
std::string ValidatePresentation2D(const WorldPresentation2D& value) {
    if (auto error = ValidateEnvironment2D(value.environment); !error.empty()) return error;
    if (value.lighting != LightingState2D::Absent && value.lighting != LightingState2D::Disabled && value.lighting != LightingState2D::Configured)
        return "invalid lighting capability state";
    if (!std::isfinite(value.view.exposure) || value.view.exposure < 0 || value.view.exposure > 100 ||
        !std::isfinite(value.view.bloomThreshold) || value.view.bloomThreshold < 0 || value.view.bloomThreshold > 100 ||
        !std::isfinite(value.view.bloomIntensity) || value.view.bloomIntensity < 0 || value.view.bloomIntensity > 100 ||
        (value.view.toneMapping != ViewToneMapping2D::None && value.view.toneMapping != ViewToneMapping2D::Aces)) return "invalid view lighting values";
    if (value.renderer.maxParticleLights < 0 || value.renderer.maxParticleLights > 1000000 ||
        (value.renderer.quality != RendererQuality2D::Low && value.renderer.quality != RendererQuality2D::Medium && value.renderer.quality != RendererQuality2D::High)) return "invalid renderer lighting values";
    const auto& effects = value.legacyEffects;
    if (!std::isfinite(effects.occlusionRadius) || effects.occlusionRadius < 0 ||
        !std::isfinite(effects.occlusionStrength) || effects.occlusionStrength < 0 || effects.occlusionStrength > 1 ||
        !std::isfinite(effects.normalHeight) || effects.normalHeight <= 0) return "invalid retained legacy lighting effects";
    return {};
}
WorldPresentation2D ReadPresentation2D(const World& world) {
    if (world.Presentation()) return *world.Presentation();
    WorldPresentation2D result;
    const auto settings = world.Registry().view<const LightingSettings2D>();
    if (settings.begin() == settings.end()) return result;
    const auto& source = settings.get<const LightingSettings2D>(*settings.begin());
    result.lighting = source.enabled ? LightingState2D::Configured : LightingState2D::Disabled;
    result.environment = {source.ambient, source.ambientIntensity, source.fog, source.fogColor, source.fogDensity, source.fogTop, source.fogBottom};
    result.view = {source.exposure, static_cast<ViewToneMapping2D>(source.toneMapping), source.bloom, source.bloomThreshold, source.bloomIntensity};
    result.renderer = {static_cast<RendererQuality2D>(source.quality), source.maxParticleLights};
    result.legacyEffects = {source.ambientOcclusion, source.occlusionRadius, source.occlusionStrength, source.normalMaps, source.normalHeight};
    return result;
}
LightingSettings2D ResolveLightingSettings2D(const World& world, std::uint64_t scene) {
    const auto source = ReadPresentation2D(world);
    auto environment = source.environment;
    for (const auto& scope : world.Scopes()) {
        if (scope.id.value == scene && scope.environment && scope.environment->overrideWorld) environment = scope.environment->value;
    }
    LightingSettings2D result;
    result.enabled = source.lighting == LightingState2D::Configured;
    result.ambient = environment.ambient; result.ambientIntensity = environment.ambientIntensity;
    result.fog = environment.fog; result.fogColor = environment.fogColor; result.fogDensity = environment.fogDensity;
    result.fogTop = environment.fogTop; result.fogBottom = environment.fogBottom;
    result.exposure = source.view.exposure; result.toneMapping = static_cast<LightToneMapping2D>(source.view.toneMapping);
    result.bloom = source.view.bloom; result.bloomThreshold = source.view.bloomThreshold; result.bloomIntensity = source.view.bloomIntensity;
    result.quality = static_cast<LightQuality2D>(source.renderer.quality); result.maxParticleLights = source.renderer.maxParticleLights;
    result.ambientOcclusion = source.legacyEffects.ambientOcclusion; result.occlusionRadius = source.legacyEffects.occlusionRadius;
    result.occlusionStrength = source.legacyEffects.occlusionStrength; result.normalMaps = source.legacyEffects.normalMaps; result.normalHeight = source.legacyEffects.normalHeight;
    return result;
}
std::string PresentationSnapshot2D(const World& world) {
    detail::PresentationJson document;
    document["canonical"] = world.Presentation().has_value();
    document["presentation"] = detail::WritePresentation(ReadPresentation2D(world));
    document["scopes"] = detail::PresentationJson::array();
    for (const auto& scope : world.Scopes()) {
        detail::PresentationJson entry{{"id", detail::CameraJson(scope.id.value)}, {"name", scope.name}};
        detail::WriteScopePresentation(scope, entry);
        const SceneGravity2D authored = scope.gravity.value_or(SceneGravity2D{});
        entry["gravity"] = {{"mode", authored.mode == SceneGravityMode2D::Override ? "override" : authored.mode == SceneGravityMode2D::Multiply ? "multiply" : "inherit"},
            {"value", {authored.value.x, authored.value.y}}, {"multiplier", authored.multiplier}};
        const auto resolved = world.ResolveGravity(scope.id);
        entry["effectiveGravity"] = {{"x", resolved.value.x}, {"y", resolved.value.y}, {"diagnostic", resolved.error}};
        document["scopes"].push_back(std::move(entry));
    }
    document["cameras"] = detail::PresentationJson::array();
    for (const auto [entity, camera, id] : world.Registry().view<const CameraComponent2D, const SceneId>().each()) {
        const auto* name = world.Registry().try_get<Name>(entity);
        document["cameras"].push_back({{"id", detail::CameraJson(id.value)}, {"name", name ? name->value : "Camera"}, {"enabled", camera.enabled}});
    }
    return document.dump();
}
} // namespace pixie
