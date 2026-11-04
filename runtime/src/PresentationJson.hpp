#pragma once
#include "pixie/runtime/World.hpp"
#include <nlohmann/json.hpp>
#include <cmath>
#include <stdexcept>

namespace pixie::detail {
using PresentationJson = nlohmann::ordered_json;
inline void Object(const PresentationJson& value) {
    if (!value.is_object()) throw std::runtime_error("presentation settings must be objects");
}
inline double Number(const PresentationJson& value, const char* name, double fallback) {
    if (!value.contains(name)) return fallback;
    if (!value[name].is_number()) throw std::runtime_error(std::string("invalid presentation number: ") + name);
    const double result = value[name].get<double>();
    if (!std::isfinite(result)) throw std::runtime_error("nonfinite presentation value");
    return result;
}
inline bool Flag(const PresentationJson& value, const char* name, bool fallback) {
    if (!value.contains(name)) return fallback;
    if (!value[name].is_boolean()) throw std::runtime_error(std::string("invalid presentation flag: ") + name);
    return value[name].get<bool>();
}
inline std::uint64_t CameraId(const PresentationJson& value) {
    if (value.is_null()) return 0;
    if (!value.is_string()) throw std::runtime_error("camera references require an entity ID or null");
    const auto text = value.get<std::string>();
    if (text.empty() || text.size() > 16 || text.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)
        throw std::runtime_error("invalid camera entity ID");
    const auto id = std::stoull(text, nullptr, 16);
    if (!id) throw std::runtime_error("zero camera ID is not an entity reference");
    return id;
}
inline PresentationJson CameraJson(std::uint64_t id) {
    if (!id) return nullptr;
    char text[17]{};
    constexpr char digits[] = "0123456789abcdef";
    for (int i = 15; i >= 0; --i) { text[i] = digits[id & 15]; id >>= 4; }
    return std::string(text);
}
inline pxColorRgba Color(const PresentationJson& source, const char* key, pxColorRgba fallback) {
    if (!source.contains(key)) return fallback;
    const auto& value = source[key];
    if (!value.is_array() || (value.size() != 3 && value.size() != 4)) throw std::runtime_error("invalid presentation color");
    unsigned char channels[4]{0, 0, 0, 255};
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (!value[i].is_number_integer() || value[i].get<double>() < 0 || value[i].get<double>() > 255)
            throw std::runtime_error("presentation color channels must be bytes");
        channels[i] = static_cast<unsigned char>(value[i].get<double>());
    }
    return {channels[0], channels[1], channels[2], channels[3]};
}
inline EnvironmentLighting2D ReadEnvironment(const PresentationJson& json) {
    Object(json); EnvironmentLighting2D value;
    value.ambient = Color(json, "ambient", value.ambient);
    value.ambientIntensity = Number(json, "ambientIntensity", value.ambientIntensity);
    value.fog = Flag(json, "fog", value.fog);
    value.fogColor = Color(json, "fogColor", value.fogColor);
    value.fogDensity = Number(json, "fogDensity", value.fogDensity);
    value.fogTop = Number(json, "fogTop", value.fogTop);
    value.fogBottom = Number(json, "fogBottom", value.fogBottom);
    if (auto error = ValidateEnvironment2D(value); !error.empty()) throw std::runtime_error(error);
    return value;
}
inline PresentationJson WriteEnvironment(const EnvironmentLighting2D& value, PresentationJson json = PresentationJson::object()) {
    Object(json);
    json["ambient"] = {value.ambient.r, value.ambient.g, value.ambient.b, value.ambient.a};
    json["ambientIntensity"] = value.ambientIntensity;
    json["fog"] = value.fog;
    json["fogColor"] = {value.fogColor.r, value.fogColor.g, value.fogColor.b, value.fogColor.a};
    json["fogDensity"] = value.fogDensity;
    json["fogTop"] = value.fogTop;
    json["fogBottom"] = value.fogBottom;
    return json;
}
inline WorldPresentation2D ReadPresentation(const PresentationJson& json) {
    Object(json); WorldPresentation2D value;
    if (!json.contains("schemaVersion") || !json["schemaVersion"].is_number_integer() || json["schemaVersion"] != 1)
        throw std::runtime_error("unsupported presentation schema");
    const auto state = json.value("lightingState", std::string("absent"));
    if (state == "absent") value.lighting = LightingState2D::Absent;
    else if (state == "disabled") value.lighting = LightingState2D::Disabled;
    else if (state == "configured") value.lighting = LightingState2D::Configured;
    else throw std::runtime_error("invalid lighting capability state");
    if (json.contains("environment")) value.environment = ReadEnvironment(json["environment"]);
    if (json.contains("view")) {
        const auto& view = json["view"]; Object(view);
        value.view.exposure = Number(view, "exposure", value.view.exposure);
        if (view.contains("toneMapping")) {
            const auto mode = view["toneMapping"].get<std::string>();
            if (mode != "none" && mode != "aces") throw std::runtime_error("invalid tone mapping");
            value.view.toneMapping = mode == "aces" ? ViewToneMapping2D::Aces : ViewToneMapping2D::None;
        }
        value.view.bloom = Flag(view, "bloom", value.view.bloom);
        value.view.bloomThreshold = Number(view, "bloomThreshold", value.view.bloomThreshold);
        value.view.bloomIntensity = Number(view, "bloomIntensity", value.view.bloomIntensity);
    }
    if (json.contains("renderer")) {
        const auto& renderer = json["renderer"]; Object(renderer);
        const auto quality = renderer.value("quality", std::string("medium"));
        if (quality != "low" && quality != "medium" && quality != "high") throw std::runtime_error("invalid renderer quality");
        value.renderer.quality = quality == "low" ? RendererQuality2D::Low : quality == "high" ? RendererQuality2D::High : RendererQuality2D::Medium;
        const double cap = Number(renderer, "maxParticleLights", value.renderer.maxParticleLights);
        if (cap < 0 || cap > 1000000 || std::floor(cap) != cap) throw std::runtime_error("invalid particle light budget");
        value.renderer.maxParticleLights = static_cast<int>(cap);
    }
    if (json.contains("legacyEffects")) {
        const auto& effects = json["legacyEffects"]; Object(effects);
        value.legacyEffects.ambientOcclusion = Flag(effects, "ambientOcclusion", value.legacyEffects.ambientOcclusion);
        value.legacyEffects.occlusionRadius = Number(effects, "occlusionRadius", value.legacyEffects.occlusionRadius);
        value.legacyEffects.occlusionStrength = Number(effects, "occlusionStrength", value.legacyEffects.occlusionStrength);
        value.legacyEffects.normalMaps = Flag(effects, "normalMaps", value.legacyEffects.normalMaps);
        value.legacyEffects.normalHeight = Number(effects, "normalHeight", value.legacyEffects.normalHeight);
    }
    if (json.contains("camera")) value.camera = CameraId(json["camera"]);
    if (auto error = ValidatePresentation2D(value); !error.empty()) throw std::runtime_error(error);
    return value;
}
inline PresentationJson WritePresentation(const WorldPresentation2D& value, PresentationJson json = PresentationJson::object()) {
    Object(json);
    json["schemaVersion"] = 1;
    json["lightingState"] = value.lighting == LightingState2D::Absent ? "absent" : value.lighting == LightingState2D::Disabled ? "disabled" : "configured";
    json["environment"] = WriteEnvironment(value.environment, json.value("environment", PresentationJson::object()));
    auto& view = json["view"]; if (view.is_null()) view = PresentationJson::object(); Object(view);
    view["exposure"] = value.view.exposure; view["toneMapping"] = value.view.toneMapping == ViewToneMapping2D::Aces ? "aces" : "none";
    view["bloom"] = value.view.bloom; view["bloomThreshold"] = value.view.bloomThreshold; view["bloomIntensity"] = value.view.bloomIntensity;
    auto& renderer = json["renderer"]; if (renderer.is_null()) renderer = PresentationJson::object(); Object(renderer);
    renderer["quality"] = value.renderer.quality == RendererQuality2D::Low ? "low" : value.renderer.quality == RendererQuality2D::High ? "high" : "medium";
    renderer["maxParticleLights"] = value.renderer.maxParticleLights;
    auto& effects = json["legacyEffects"]; if (effects.is_null()) effects = PresentationJson::object(); Object(effects);
    effects["ambientOcclusion"] = value.legacyEffects.ambientOcclusion; effects["occlusionRadius"] = value.legacyEffects.occlusionRadius;
    effects["occlusionStrength"] = value.legacyEffects.occlusionStrength; effects["normalMaps"] = value.legacyEffects.normalMaps; effects["normalHeight"] = value.legacyEffects.normalHeight;
    json["camera"] = CameraJson(value.camera);
    return json;
}
inline void ReadScopePresentation(const PresentationJson& json, SceneScope& scope) {
    if (json.contains("environment")) {
        const auto& environment = json["environment"]; Object(environment);
        const auto mode = environment.value("mode", std::string("inherit"));
        if (mode != "inherit" && mode != "override") throw std::runtime_error("invalid Scene Environment mode");
        SceneEnvironment2D value; value.overrideWorld = mode == "override";
        if (environment.contains("value")) value.value = ReadEnvironment(environment["value"]);
        scope.environment = value;
    }
    if (json.contains("camera")) scope.camera = CameraId(json["camera"]);
}
inline void WriteScopePresentation(const SceneScope& scope, PresentationJson& json) {
    if (scope.environment) {
        auto environment = json.value("environment", PresentationJson::object()); Object(environment);
        environment["mode"] = scope.environment->overrideWorld ? "override" : "inherit";
        environment["value"] = WriteEnvironment(scope.environment->value, environment.value("value", PresentationJson::object()));
        json["environment"] = std::move(environment);
    } else json.erase("environment");
    if (scope.camera) json["camera"] = CameraJson(scope.camera); else json.erase("camera");
}
} // namespace pixie::detail
