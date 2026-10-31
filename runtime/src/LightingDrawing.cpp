#include "pixie/runtime/LightingDrawing.hpp"

#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

#include <algorithm>
#include <cmath>

namespace pixie {

namespace {

std::array<float, 3> Linear(pxColorRgba color, double intensity) {
    const auto scale = static_cast<float>(intensity);
    return {DecodeSrgb(color.r / 255.0f) * scale, DecodeSrgb(color.g / 255.0f) * scale,
            DecodeSrgb(color.b / 255.0f) * scale};
}

bool Reaches(const LightInstance& light, const pxRect& view) {
    if (light.kind == LightKind::Directional) {
        return true;
    }
    const double x = std::clamp(static_cast<double>(light.x), view.x, view.x + view.width);
    const double y = std::clamp(static_cast<double>(light.y), view.y, view.y + view.height);
    const double dx = x - light.x;
    const double dy = y - light.y;
    return dx * dx + dy * dy < static_cast<double>(light.range) * light.range;
}

} // namespace

const LightingSettings2D* FindLightingSettings(World& world) {
    const auto settings = world.Registry().view<const LightingSettings2D>();
    return settings.begin() == settings.end() ? nullptr : &settings.get<const LightingSettings2D>(*settings.begin());
}

LightingParameters MakeLightingParameters(const LightingSettings2D& settings) {
    LightingParameters parameters;
    parameters.ambient = Linear(settings.ambient, std::max(settings.ambientIntensity, 0.0));
    parameters.exposure = static_cast<float>(std::max(settings.exposure, 0.0));
    parameters.toneMapping =
        settings.toneMapping == LightToneMapping2D::Aces ? ToneMapping::Aces : ToneMapping::None;
    return parameters;
}

LightInstance MakeLightInstance(const Light2D& light, pxPoint position, double rotation) {
    LightInstance instance;
    instance.x = static_cast<float>(position.x);
    instance.y = static_cast<float>(position.y);
    instance.range = static_cast<float>(std::max(light.range, 0.0));
    instance.falloff = static_cast<float>(std::max(light.falloff, 0.0));
    instance.color = Linear(light.color, std::max(light.intensity, 0.0));
    instance.kind = light.type == LightType2D::Spot          ? LightKind::Spot
                    : light.type == LightType2D::Directional ? LightKind::Directional
                                                             : LightKind::Point;
    instance.directionX = static_cast<float>(std::cos(rotation));
    instance.directionY = static_cast<float>(std::sin(rotation));
    const double half = std::clamp(light.coneAngle, 0.0, 2.0 * std::numbers::pi) * 0.5;
    const double softness = std::clamp(light.coneSoftness, 0.0, 1.0);
    instance.cosOuter = static_cast<float>(std::cos(half));
    instance.cosInner = static_cast<float>(std::cos(half * (1.0 - softness)));
    instance.layers = static_cast<std::uint32_t>(light.lightLayers) & 0xFFu;
    return instance;
}

void GatherLights(World& world, double alpha, const pxRect* view, std::vector<LightInstance>& lights) {
    lights.clear();
    entt::registry& registry = world.Registry();
    for (auto [id, light, transform] : registry.view<const Light2D, const Transform2D>().each()) {
        if (!light.enabled || !std::isfinite(light.range) || !std::isfinite(light.intensity)) {
            continue;
        }
        const auto* previous = registry.try_get<const PreviousTransform2D>(id);
        const Transform2D drawn = previous != nullptr ? Interpolate(previous->value, transform, alpha) : transform;
        const LightInstance instance = MakeLightInstance(light, drawn.position, drawn.rotation);
        if (view == nullptr || Reaches(instance, *view)) {
            lights.push_back(instance);
        }
    }
    // EnTT iterates newest first; the World's order is oldest first.
    std::reverse(lights.begin(), lights.end());
}

bool HasLightReceivers(World& world) {
    const auto receivers = world.Registry().view<const LightReceiver2D>();
    return receivers.begin() != receivers.end();
}

pxColorRgba ReceiverColor(const LightReceiver2D* receiver, std::uint8_t alpha) {
    if (receiver == nullptr) {
        return pxColorRgba(255, 0, alpha, 255);
    }
    return pxColorRgba(static_cast<std::uint8_t>(receiver->lightLayers & 0xFF),
                       static_cast<std::uint8_t>(receiver->unlit ? 255 : 0), alpha, 255);
}

} // namespace pixie
