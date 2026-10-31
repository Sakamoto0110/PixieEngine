#include "pixie/runtime/LightingDrawing.hpp"

#include "pixie/core/Image.hpp"
#include "pixie/runtime/ParticleSystem.hpp"
#include "pixie/runtime/SpriteDrawing.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

#include <format>
#include <optional>

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
    parameters.halfResolutionShadows = settings.quality == LightQuality2D::Low;
    parameters.normalMaps = settings.normalMaps;
    parameters.normalHeight = static_cast<float>(std::isfinite(settings.normalHeight) ? std::max(settings.normalHeight, 0.001) : 48.0);
    const auto finite = [](double value, double fallback) { return std::isfinite(value) ? value : fallback; };
    if (settings.ambientOcclusion) {
        parameters.occlusionStrength = static_cast<float>(std::clamp(finite(settings.occlusionStrength, 0.0), 0.0, 1.0));
        parameters.occlusionRadius = static_cast<float>(std::max(finite(settings.occlusionRadius, 0.0), 0.001));
    }
    if (settings.fog) {
        parameters.fogDensity = static_cast<float>(std::clamp(finite(settings.fogDensity, 0.0), 0.0, 1.0));
        parameters.fogColor = Linear(settings.fogColor, 1.0);
        const double top = finite(settings.fogTop, 0.0);
        parameters.fogTop = static_cast<float>(top);
        // smoothstep needs its edges apart and in order.
        parameters.fogBottom = static_cast<float>(std::max(finite(settings.fogBottom, top + 1.0), top + 0.001));
    }
    if (settings.bloom) {
        parameters.bloomIntensity = static_cast<float>(std::clamp(finite(settings.bloomIntensity, 0.0), 0.0, 16.0));
        parameters.bloomThreshold = static_cast<float>(std::max(finite(settings.bloomThreshold, 1.0), 0.0));
    }
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

std::vector<OutlineEdge> SpriteAlphaOutlines::Trace(int width, int height, std::span<const std::uint8_t> rgba,
                                                     double threshold, int cells) {
    std::vector<OutlineEdge> edges;
    if (width <= 0 || height <= 0 || rgba.size() < static_cast<std::size_t>(width) * height * 4 || cells <= 0) {
        return edges;
    }
    const int cell = std::max(1, (std::max(width, height) + cells - 1) / cells);
    const int nx = (width + cell - 1) / cell;
    const int ny = (height + cell - 1) / cell;
    // A cell is solid when its mean alpha reaches the threshold.
    std::vector<std::uint8_t> solid(static_cast<std::size_t>(nx) * ny, 0);
    for (int cy = 0; cy < ny; ++cy) {
        for (int cx = 0; cx < nx; ++cx) {
            double sum = 0.0;
            int count = 0;
            for (int y = cy * cell; y < std::min(height, (cy + 1) * cell); ++y) {
                for (int x = cx * cell; x < std::min(width, (cx + 1) * cell); ++x) {
                    sum += rgba[(static_cast<std::size_t>(y) * width + x) * 4 + 3];
                    ++count;
                }
            }
            solid[static_cast<std::size_t>(cy) * nx + cx] = sum >= threshold * 255.0 * count ? 1 : 0;
        }
    }
    const auto at = [&](int cx, int cy) {
        return cx >= 0 && cy >= 0 && cx < nx && cy < ny && solid[static_cast<std::size_t>(cy) * nx + cx] != 0;
    };
    const auto u = [&](int cx) { return static_cast<double>(std::min(width, cx * cell)) / width; };
    const auto v = [&](int cy) { return static_cast<double>(std::min(height, cy * cell)) / height; };
    // Rows: top boundaries run left to right, bottom ones right to left, merged where they continue.
    for (int cy = 0; cy < ny; ++cy) {
        for (const int side : {-1, 1}) {
            int start = -1;
            for (int cx = 0; cx <= nx; ++cx) {
                const bool edge = cx < nx && at(cx, cy) && !at(cx, cy + side);
                if (edge && start < 0) {
                    start = cx;
                } else if (!edge && start >= 0) {
                    const double y = v(side < 0 ? cy : cy + 1);
                    edges.push_back(side < 0 ? OutlineEdge{pxPoint(u(start), y), pxPoint(u(cx), y)}
                                             : OutlineEdge{pxPoint(u(cx), y), pxPoint(u(start), y)});
                    start = -1;
                }
            }
        }
    }
    // Columns: right boundaries run downward, left ones upward.
    for (int cx = 0; cx < nx; ++cx) {
        for (const int side : {-1, 1}) {
            int start = -1;
            for (int cy = 0; cy <= ny; ++cy) {
                const bool edge = cy < ny && at(cx, cy) && !at(cx + side, cy);
                if (edge && start < 0) {
                    start = cy;
                } else if (!edge && start >= 0) {
                    const double x = u(side < 0 ? cx : cx + 1);
                    edges.push_back(side > 0 ? OutlineEdge{pxPoint(x, v(start)), pxPoint(x, v(cy))}
                                             : OutlineEdge{pxPoint(x, v(cy)), pxPoint(x, v(start))});
                    start = -1;
                }
            }
        }
    }
    return edges;
}

const std::vector<OutlineEdge>& SpriteAlphaOutlines::Edges(const Sprite& sprite, double threshold, pxVec2& pixelSize) {
    const bool region = sprite.region.width > 0.0 && sprite.region.height > 0.0;
    const std::string key = std::format("{}|{}|{}|{}|{}|{}", sprite.image, sprite.region.x, sprite.region.y,
                                        sprite.region.width, sprite.region.height, threshold);
    auto found = traced_.find(key);
    if (found == traced_.end()) {
        Traced traced;
        std::string error;
        if (const std::optional<Image> image = LoadPngFile(folder_ / sprite.image, error)) {
            int x0 = 0, y0 = 0, w = image->width, h = image->height;
            if (region) {
                x0 = std::clamp(static_cast<int>(sprite.region.x), 0, image->width);
                y0 = std::clamp(static_cast<int>(sprite.region.y), 0, image->height);
                w = std::clamp(static_cast<int>(sprite.region.width), 0, image->width - x0);
                h = std::clamp(static_cast<int>(sprite.region.height), 0, image->height - y0);
            }
            std::vector<std::uint8_t> part(static_cast<std::size_t>(w) * h * 4);
            for (int y = 0; y < h; ++y) {
                std::copy_n(image->pixels.begin() + (static_cast<std::ptrdiff_t>(y0 + y) * image->width + x0) * 4,
                            static_cast<std::ptrdiff_t>(w) * 4,
                            part.begin() + static_cast<std::ptrdiff_t>(y) * w * 4);
            }
            traced.pixelSize = region ? pxVec2(sprite.region.width, sprite.region.height) : pxVec2(w, h);
            traced.edges = Trace(w, h, part, threshold);
        }
        found = traced_.emplace(key, std::move(traced)).first;
    }
    pixelSize = found->second.pixelSize;
    return found->second.edges;
}

void GatherOccluders(World& world, double alpha, std::vector<OccluderEdge>& edges, SpriteAlphaOutlines* outlines) {
    edges.clear();
    entt::registry& registry = world.Registry();
    for (auto [id, occluder, transform] : registry.view<const Occluder2D, const Transform2D>().each()) {
        if (!occluder.enabled) {
            continue;
        }
        const auto* previous = registry.try_get<const PreviousTransform2D>(id);
        const Transform2D drawn = previous != nullptr ? Interpolate(previous->value, transform, alpha) : transform;
        if (occluder.source == OccluderSource2D::SpriteAlpha) {
            const auto* sprite = registry.try_get<const Sprite>(id);
            if (outlines == nullptr || sprite == nullptr) {
                continue;
            }
            pxVec2 pixelSize;
            const std::vector<OutlineEdge>& outline =
                outlines->Edges(*sprite, std::clamp(occluder.alphaThreshold, 0.0, 1.0), pixelSize);
            // A mirrored sprite turns the outline's winding around.
            const bool mirrored = drawn.scale.x * drawn.scale.y < 0.0;
            for (const OutlineEdge& local : outline) {
                const pxPoint a = SpritePoint(*sprite, drawn, pixelSize, mirrored ? local.b : local.a);
                const pxPoint b = SpritePoint(*sprite, drawn, pixelSize, mirrored ? local.a : local.b);
                OccluderEdge edge;
                edge.ax = static_cast<float>(a.x);
                edge.ay = static_cast<float>(a.y);
                edge.bx = static_cast<float>(b.x);
                edge.by = static_cast<float>(b.y);
                edge.layers = static_cast<std::uint32_t>(occluder.layers) & 0xFFu;
                edge.flags = occluder.selfShadow ? 1u : 0u;
                edges.push_back(edge);
            }
            continue;
        }
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

int GatherParticleLights(World& world, double alpha, const pxRect* view, int limit, std::vector<LightInstance>& lights) {
    entt::registry& registry = world.Registry();
    std::vector<std::pair<const ParticleSystem*, const ParticleLight2D*>> pools;
    for (auto [id, system, light] : registry.view<const ParticleSystem, const ParticleLight2D>().each()) {
        if (light.illuminates) {
            pools.emplace_back(&system, &light);
        }
    }
    // EnTT iterates newest first; the World's order is oldest first.
    std::reverse(pools.begin(), pools.end());
    int left = std::max(limit, 0);
    int dropped = 0;
    const auto t = static_cast<float>(alpha);
    for (const auto& [system, light] : pools) {
        int cap = std::max(light->lightCap, 0);
        const auto intensity = static_cast<float>(std::isfinite(light->lightIntensity) ? std::max(light->lightIntensity, 0.0) : 0.0);
        const auto radius = static_cast<float>(std::isfinite(light->lightRadius) ? std::max(light->lightRadius, 0.0) : 0.0);
        for (const Particle& particle : system->particles) {
            if (cap == 0 || left == 0) {
                ++dropped;
                continue;
            }
            const float life = system->style.fade && particle.lifetime > 0.0f
                                   ? std::clamp(1.0f - particle.age / particle.lifetime, 0.0f, 1.0f)
                                   : 1.0f;
            const float strength = intensity * particle.color.a / 255.0f * life;
            LightInstance splat;
            splat.x = particle.previous.x + (particle.position.x - particle.previous.x) * t;
            splat.y = particle.previous.y + (particle.position.y - particle.previous.y) * t;
            splat.range = radius;
            splat.falloff = 2.0f;
            splat.color = {DecodeSrgb(particle.color.r / 255.0f) * strength, DecodeSrgb(particle.color.g / 255.0f) * strength,
                           DecodeSrgb(particle.color.b / 255.0f) * strength};
            if (strength <= 0.0f || radius <= 0.0f || (view != nullptr && !Reaches(splat, *view))) {
                continue;
            }
            lights.push_back(splat);
            --cap;
            --left;
        }
    }
    return dropped;
}

pxColorRgba EmissionColor(const Emission2D* emission) {
    if (emission == nullptr || !std::isfinite(emission->intensity)) {
        return pxColorRgba(0, 0, 0, 0);
    }
    const double intensity = std::clamp(emission->intensity, 0.0, 16.0);
    return pxColorRgba(emission->color.r, emission->color.g, emission->color.b,
                       static_cast<std::uint8_t>(std::lround(intensity / 16.0 * 255.0)));
}

pxColorRgba PassColor(const entt::registry& registry, entt::entity id, std::uint8_t alpha, LightingPass pass) {
    if (pass == LightingPass::Normals) {
        const auto* normal = registry.try_get<const NormalMap2D>(id);
        const double strength = normal != nullptr && std::isfinite(normal->strength) ? std::clamp(normal->strength, 0.0, 1.0) : 0.0;
        return pxColorRgba(static_cast<std::uint8_t>(std::lround(strength * 255.0)),
                           static_cast<std::uint8_t>(normal != nullptr && !normal->image.empty() ? 255 : 0), alpha, 255);
    }
    return pass == LightingPass::Emission ? EmissionColor(registry.try_get<const Emission2D>(id))
                                          : ReceiverColor(registry.try_get<const LightReceiver2D>(id), alpha);
}

bool HasNormalMaps(World& world) {
    const auto maps = world.Registry().view<const NormalMap2D>();
    return maps.begin() != maps.end();
}

bool HasEmission(World& world) {
    entt::registry& registry = world.Registry();
    const auto emitters = registry.view<const Emission2D>();
    if (emitters.begin() != emitters.end()) {
        return true;
    }
    for (auto [id, light] : registry.view<const ParticleLight2D>().each()) {
        if (light.emissive) {
            return true;
        }
    }
    return false;
}

void DrawParticlePools(World& world, ParticleRenderer& renderer, const Camera2D& camera, double alpha, bool emissive) {
    entt::registry& registry = world.Registry();
    for (auto [id, system] : registry.view<const ParticleSystem>().each()) {
        const auto* light = registry.try_get<const ParticleLight2D>(id);
        if ((light != nullptr && light->emissive) == emissive) {
            renderer.Draw(system.particles, system.style, alpha, camera.MakeView());
        }
    }
}

int ShadowedLightCap(LightQuality2D quality) {
    return quality == LightQuality2D::Low ? 8 : quality == LightQuality2D::High ? 64 : 32;
}

int CapShadowedLights(std::vector<LightInstance>& lights, int cap, const pxRect& view) {
    std::vector<std::pair<double, std::size_t>> shadowed;
    for (std::size_t i = 0; i < lights.size(); ++i) {
        const LightInstance& light = lights[i];
        if (light.shadowLayers == 0u) {
            continue;
        }
        double area = view.width * view.height;
        if (light.kind != LightKind::Directional) {
            const double x0 = std::max(view.x, static_cast<double>(light.x - light.range));
            const double x1 = std::min(view.x + view.width, static_cast<double>(light.x + light.range));
            const double y0 = std::max(view.y, static_cast<double>(light.y - light.range));
            const double y1 = std::min(view.y + view.height, static_cast<double>(light.y + light.range));
            area = std::max(0.0, x1 - x0) * std::max(0.0, y1 - y0);
        }
        shadowed.emplace_back(area * (light.color[0] + light.color[1] + light.color[2]), i);
    }
    const auto keep = static_cast<std::size_t>(std::max(cap, 0));
    if (shadowed.size() <= keep) {
        return 0;
    }
    // The weakest first; ties by order, so the result does not depend on the sort.
    std::stable_sort(shadowed.begin(), shadowed.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    std::vector<bool> removed(lights.size(), false);
    const std::size_t drop = shadowed.size() - keep;
    for (std::size_t i = 0; i < drop; ++i) {
        removed[shadowed[i].second] = true;
    }
    std::size_t next = 0;
    for (std::size_t i = 0; i < lights.size(); ++i) {
        if (!removed[i]) {
            lights[next++] = lights[i];
        }
    }
    lights.resize(next);
    return static_cast<int>(drop);
}

void DrawLightingDebug2D(std::span<const LightInstance> lights, std::span<const OccluderEdge> occluders,
                         ShapeRenderer& renderer, const Camera2D& camera, std::uint32_t flags) {
    const double thickness = 0.75 / camera.Zoom();
    const auto line = [&](double ax, double ay, double bx, double by, pxColorRgba color) {
        ShapeInstance edge;
        edge.position = {static_cast<float>((ax + bx) / 2), static_cast<float>((ay + by) / 2)};
        edge.scale = {static_cast<float>(std::hypot(bx - ax, by - ay) / 2), static_cast<float>(thickness)};
        edge.rotation = static_cast<float>(std::atan2(by - ay, bx - ax));
        edge.color = color;
        renderer.Submit(ShapeKind::Quad, edge);
    };
    if ((flags & kLightingDebugLights2D) != 0) {
        const pxColorRgba color(255, 220, 90);
        for (const LightInstance& light : lights) {
            if (light.kind == LightKind::Directional) {
                const double length = 60.0 / camera.Zoom();
                line(light.x, light.y, light.x + light.directionX * length, light.y + light.directionY * length, color);
                continue;
            }
            ShapeInstance ring;
            ring.position = {light.x, light.y};
            ring.scale = {light.range, light.range};
            ring.color = color;
            renderer.Submit(ShapeKind::Ring, ring);
            if (light.kind == LightKind::Spot) {
                const double half = std::acos(std::clamp(static_cast<double>(light.cosOuter), -1.0, 1.0));
                const double angle = std::atan2(light.directionY, light.directionX);
                for (const double side : {-half, half}) {
                    line(light.x, light.y, light.x + std::cos(angle + side) * light.range,
                         light.y + std::sin(angle + side) * light.range, color);
                }
            }
        }
    }
    if ((flags & kLightingDebugOccluders2D) != 0) {
        for (const OccluderEdge& edge : occluders) {
            line(edge.ax, edge.ay, edge.bx, edge.by, pxColorRgba(120, 230, 255));
        }
    }
}

std::array<std::string, 8> LightLayerNames(const World& world) {
    std::array<std::string, 8> names;
    for (std::size_t i = 0; i < names.size(); ++i) {
        names[i] = "Light layer " + std::to_string(i + 1);
    }
    const auto view = world.Registry().view<const LightLayers2D>();
    if (view.begin() == view.end()) {
        names[0] = "Default";
        return names;
    }
    if (std::next(view.begin()) != view.end()) {
        return names;
    }
    std::array<int, 8> counts{};
    for (const auto& record : view.get<const LightLayers2D>(*view.begin()).layers) {
        const int slot = record.value.slot;
        if (slot < 1 || slot > 8) {
            continue;
        }
        const auto index = static_cast<std::size_t>(slot - 1);
        if (++counts[index] > 1) {
            names[index] = "Light layer " + std::to_string(slot);
        } else if (!record.value.name.empty()) {
            names[index] = record.value.name;
        }
    }
    return names;
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
