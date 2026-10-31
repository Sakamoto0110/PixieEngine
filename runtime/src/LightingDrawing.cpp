#include "pixie/runtime/LightingDrawing.hpp"

#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <utility>

namespace pixie {

namespace {

std::array<float, 3> Linear(pxColorRgba color, double intensity) {
    const auto scale = static_cast<float>(intensity);
    return {DecodeSrgb(color.r / 255.0f) * scale, DecodeSrgb(color.g / 255.0f) * scale,
            DecodeSrgb(color.b / 255.0f) * scale};
}

// Segments for a circle of `radius` world units whose chords stay within 0.5 of the curve.
int CircleSegments(double radius) {
    if (!(radius > 0.5)) {
        return 8;
    }
    return std::clamp(static_cast<int>(std::ceil(std::numbers::pi / std::acos(1.0 - 0.5 / radius))), 8, 32);
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
    instance.shadowLayers = light.castsShadows ? static_cast<std::uint32_t>(light.shadowLayers) & 0xFFu : 0u;
    const double source = light.type == LightType2D::Directional ? light.sourceAngle : light.sourceRadius;
    instance.source = static_cast<float>(std::isfinite(source) ? std::max(source, 0.0) : 0.0);
    return instance;
}

std::vector<std::vector<pxPoint>> OccluderPolygons(const Occluder2D& occluder, const Transform2D& transform,
                                                   const Collider2D* collider, const CompoundCollider2D* compound,
                                                   const CharacterMotor2D* motor) {
    std::vector<std::vector<pxPoint>> polygons;
    const double c = std::cos(transform.rotation);
    const double s = std::sin(transform.rotation);
    const auto place = [&](pxPoint local) {
        const double x = local.x * transform.scale.x;
        const double y = local.y * transform.scale.y;
        return pxPoint(transform.position.x + c * x - s * y, transform.position.y + s * x + c * y);
    };
    const double largestScale = std::max(std::abs(transform.scale.x), std::abs(transform.scale.y));
    const auto add = [&](std::vector<pxPoint> points) {
        if (points.size() < 3) {
            return;
        }
        double area = 0.0;
        for (std::size_t i = 0; i < points.size(); ++i) {
            const pxPoint& a = points[i];
            const pxPoint& b = points[(i + 1) % points.size()];
            area += a.x * b.y - b.x * a.y;
        }
        if (!std::isfinite(area) || area == 0.0) {
            return;
        }
        if (area < 0.0) {
            std::reverse(points.begin(), points.end());
        }
        polygons.push_back(std::move(points));
    };
    const auto addPiece = [&](const Collider2D& piece) {
        if (!piece.enabled || piece.sensor) {
            return;
        }
        const double sc = std::cos(piece.rotation);
        const double ss = std::sin(piece.rotation);
        const auto local = [&](double x, double y) {
            return place(pxPoint(piece.offset.x + sc * x - ss * y, piece.offset.y + ss * x + sc * y));
        };
        std::vector<pxPoint> points;
        switch (piece.kind) {
        case ColliderKind2D::Box: {
            constexpr std::array<std::pair<double, double>, 4> corners{{{-1.0, -1.0}, {1.0, -1.0}, {1.0, 1.0}, {-1.0, 1.0}}};
            for (const auto& [x, y] : corners) {
                points.push_back(local(x * piece.halfSize.x, y * piece.halfSize.y));
            }
            break;
        }
        case ColliderKind2D::Circle: {
            const int n = CircleSegments(piece.radius * largestScale);
            for (int i = 0; i < n; ++i) {
                const double a = 2.0 * std::numbers::pi * i / n;
                points.push_back(local(piece.radius * std::cos(a), piece.radius * std::sin(a)));
            }
            break;
        }
        case ColliderKind2D::Polygon:
            for (const pxPoint& p : piece.points) {
                points.push_back(local(p.x, p.y));
            }
            break;
        }
        add(std::move(points));
    };
    if (occluder.source == OccluderSource2D::Polygon) {
        std::vector<pxPoint> points;
        for (const pxPoint& p : occluder.polygon) {
            points.push_back(place(p));
        }
        add(std::move(points));
    } else if (motor != nullptr) {
        // The motor's capsule stays upright at the entity's position, in world units.
        // An even count per cap puts a vertex on its top and bottom.
        const int half = std::max(4, (CircleSegments(motor->radius) + 3) / 4 * 2);
        std::vector<pxPoint> points;
        for (int i = 0; i <= half; ++i) {
            const double a = std::numbers::pi + std::numbers::pi * i / half;
            points.emplace_back(transform.position.x + motor->radius * std::cos(a),
                                transform.position.y - motor->halfSegment + motor->radius * std::sin(a));
        }
        for (int i = 0; i <= half; ++i) {
            const double a = std::numbers::pi * i / half;
            points.emplace_back(transform.position.x + motor->radius * std::cos(a),
                                transform.position.y + motor->halfSegment + motor->radius * std::sin(a));
        }
        add(std::move(points));
    } else if (compound != nullptr) {
        if (compound->enabled) {
            for (const ComponentRecord<Collider2D>& record : compound->shapes) {
                addPiece(record.value);
            }
        }
    } else if (collider != nullptr) {
        addPiece(*collider);
    }
    return polygons;
}

void GatherOccluders(World& world, double alpha, std::vector<OccluderEdge>& edges) {
    edges.clear();
    entt::registry& registry = world.Registry();
    for (auto [id, occluder, transform] : registry.view<const Occluder2D, const Transform2D>().each()) {
        if (!occluder.enabled) {
            continue;
        }
        const auto* previous = registry.try_get<const PreviousTransform2D>(id);
        const Transform2D drawn = previous != nullptr ? Interpolate(previous->value, transform, alpha) : transform;
        const auto polygons = OccluderPolygons(occluder, drawn, registry.try_get<const Collider2D>(id),
                                               registry.try_get<const CompoundCollider2D>(id),
                                               registry.try_get<const CharacterMotor2D>(id));
        for (const auto& polygon : polygons) {
            for (std::size_t i = 0; i < polygon.size(); ++i) {
                const pxPoint& a = polygon[i];
                const pxPoint& b = polygon[(i + 1) % polygon.size()];
                OccluderEdge edge;
                edge.ax = static_cast<float>(a.x);
                edge.ay = static_cast<float>(a.y);
                edge.bx = static_cast<float>(b.x);
                edge.by = static_cast<float>(b.y);
                edge.layers = static_cast<std::uint32_t>(occluder.layers) & 0xFFu;
                edge.flags = occluder.selfShadow ? 1u : 0u;
                edges.push_back(edge);
            }
        }
    }
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
