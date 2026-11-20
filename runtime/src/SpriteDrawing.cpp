#include "pixie/runtime/SpriteDrawing.hpp"

#include "pixie/runtime/Culling.hpp"
#include "pixie/runtime/LightingDrawing.hpp"
#include "pixie/runtime/Parenting2D.hpp"

#include "pixie/runtime/Sprite.hpp"
#include "pixie/runtime/SpriteImages.hpp"
#include "pixie/runtime/Text.hpp"
#include "pixie/runtime/TextDrawing.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace pixie {

namespace {

bool HasRegion(const SpriteSource& source) {
    return source.region.width > 0.0 && source.region.height > 0.0;
}

pxVec2 PixelSize(const SpriteSource& source, SpriteImages& images) {
    if (HasRegion(source)) {
        return {source.region.width, source.region.height};
    }
    return images.Size(source.image).value_or(pxVec2(SpriteImages::kMissingSize, SpriteImages::kMissingSize));
}

// The drawn rectangle in world units, before rotation, relative to the pivot: its top-left corner
// and its signed size (a negative scale mirrors it).
struct LocalRect {
    pxVec2 topLeft;
    pxVec2 size;
};

LocalRect Local(const SpriteGeometry& geometry, const Transform2D& transform) {
    const pxVec2 size = geometry.pixelSize * transform.scale;
    return {-geometry.pivot * size, size};
}

pxPoint ToWorld(const Transform2D& transform, pxVec2 local) {
    const double c = std::cos(transform.rotation);
    const double s = std::sin(transform.rotation);
    return pxPoint(transform.position.x + c * local.x - s * local.y, transform.position.y + s * local.x + c * local.y);
}

} // namespace

SpriteGeometry ResolveSpriteGeometry(const Sprite& sprite, SpriteImages& images) {
    const auto source = images.Source(sprite);
    return {PixelSize(source, images), source.pivot};
}

pxVec2 SpritePixelSize(const Sprite& sprite, SpriteImages& images) {
    return ResolveSpriteGeometry(sprite, images).pixelSize;
}

pxPoint SpritePoint(const Sprite& sprite, const Transform2D& transform, pxVec2 pixelSize, pxPoint uv) {
    return SpritePoint({pixelSize, sprite.pivot}, transform, uv);
}

pxPoint SpritePoint(const SpriteGeometry& geometry, const Transform2D& transform, pxPoint uv) {
    const LocalRect rect = Local(geometry, transform);
    return ToWorld(transform, rect.topLeft + pxVec2(uv.x, uv.y) * rect.size);
}

std::optional<pxPoint> SpriteLocal(const Sprite& sprite, const Transform2D& transform, pxVec2 pixelSize,
                                   pxPoint world) {
    return SpriteLocal({pixelSize, sprite.pivot}, transform, world);
}

std::optional<pxPoint> SpriteLocal(const SpriteGeometry& geometry, const Transform2D& transform, pxPoint world) {
    const LocalRect rect = Local(geometry, transform);
    if (rect.size.x == 0.0 || rect.size.y == 0.0) {
        return std::nullopt;
    }
    const double dx = world.x - transform.position.x;
    const double dy = world.y - transform.position.y;
    const double c = std::cos(transform.rotation);
    const double s = std::sin(transform.rotation);
    const double x = c * dx + s * dy;
    const double y = -s * dx + c * dy;
    return pxPoint((x - rect.topLeft.x) / rect.size.x, (y - rect.topLeft.y) / rect.size.y);
}

std::array<pxPoint, 4> SpriteCorners(const Sprite& sprite, const Transform2D& transform, pxVec2 pixelSize) {
    return SpriteCorners({pixelSize, sprite.pivot}, transform);
}

std::array<pxPoint, 4> SpriteCorners(const SpriteGeometry& geometry, const Transform2D& transform) {
    return {SpritePoint(geometry, transform, pxPoint(0.0, 0.0)),
            SpritePoint(geometry, transform, pxPoint(1.0, 0.0)),
            SpritePoint(geometry, transform, pxPoint(1.0, 1.0)),
            SpritePoint(geometry, transform, pxPoint(0.0, 1.0))};
}

bool SpriteContains(const Sprite& sprite, const Transform2D& transform, pxVec2 pixelSize, pxPoint point,
                    double tolerance) {
    return SpriteContains({pixelSize, sprite.pivot}, transform, point, tolerance);
}

bool SpriteContains(const SpriteGeometry& geometry, const Transform2D& transform, pxPoint point, double tolerance) {
    const LocalRect rect = Local(geometry, transform);
    // The point in the sprite's frame: moved to the pivot and turned back.
    const double dx = point.x - transform.position.x;
    const double dy = point.y - transform.position.y;
    const double c = std::cos(transform.rotation);
    const double s = std::sin(transform.rotation);
    const double x = c * dx + s * dy;
    const double y = -s * dx + c * dy;
    const double left = std::min(rect.topLeft.x, rect.topLeft.x + rect.size.x);
    const double right = std::max(rect.topLeft.x, rect.topLeft.x + rect.size.x);
    const double top = std::min(rect.topLeft.y, rect.topLeft.y + rect.size.y);
    const double bottom = std::max(rect.topLeft.y, rect.topLeft.y + rect.size.y);
    return x >= left - tolerance && x <= right + tolerance && y >= top - tolerance && y <= bottom + tolerance;
}

void ForEachDrawnInOrder(World& world, const std::function<void(const Drawn&)>& visit) {
    struct Item {
        entt::entity id;
        const Transform2D* transform;
        const Sprite* sprite;
        const Text* text;
        int layer;
    };
    entt::registry& registry = world.Registry();
    std::vector<Item> order;
    for (auto [id, transform] : registry.view<const Transform2D>().each()) {
        const auto* sprite = registry.try_get<const Sprite>(id);
        const auto* text = registry.try_get<const Text>(id);
        // Hidden ones are neither drawn nor picked.
        if (sprite != nullptr && !sprite->visible) {
            sprite = nullptr;
        }
        if (text != nullptr && !text->visible) {
            text = nullptr;
        }
        // Pushed text first: the list is reversed below, which puts the sprite first.
        if (text != nullptr) {
            order.push_back({id, &transform, nullptr, text, text->layer});
        }
        if (sprite != nullptr) {
            order.push_back({id, &transform, sprite, nullptr, sprite->layer});
        }
    }
    // EnTT iterates newest first; the World's order is oldest first.
    std::reverse(order.begin(), order.end());
    std::stable_sort(order.begin(), order.end(), [](const Item& a, const Item& b) { return a.layer < b.layer; });
    for (const Item& item : order) {
        visit(Drawn{item.id, *item.transform, item.sprite, item.text, item.layer});
    }
}

void ForEachSpriteInDrawOrder(World& world,
                              const std::function<void(entt::entity, const Transform2D&, const Sprite&)>& visit) {
    ForEachDrawnInOrder(world, [&](const Drawn& drawn) {
        if (drawn.sprite != nullptr) {
            visit(drawn.id, drawn.transform, *drawn.sprite);
        }
    });
}

namespace {

void Submit(World& world, SpriteRenderer& renderer, SpriteImages& images, double alpha, const TextSources* texts,
            const pxRect* view, LightingPass pass) {
    entt::registry& registry = world.Registry();
    ForEachDrawnInOrder(world, [&](const Drawn& item) {
        const Transform2D drawn = InterpolatedTransform2D(world.Wrap(item.id), alpha);
        if (item.text != nullptr) {
            if (texts != nullptr && (view == nullptr || TextInView(*item.text, drawn, TextBlock(*item.text, texts->fonts), *view))) {
                if (pass != LightingPass::Color) {
                    Text recolored = *item.text;
                    recolored.color = PassColor(registry, item.id, item.text->color.a, pass);
                    SubmitText(recolored, drawn, texts->fonts, texts->renderer, renderer, texts->zoom);
                } else {
                    SubmitText(*item.text, drawn, texts->fonts, texts->renderer, renderer, texts->zoom);
                }
            }
            return;
        }
        const Sprite& sprite = *item.sprite;
        const SpriteSource source = images.Source(sprite);
        const pxVec2 pixelSize = PixelSize(source, images);
        if (view != nullptr && !SpriteInView(sprite, drawn, pixelSize, *view)) {
            return;
        }
        // The normal pass draws a sprite's normal map in its place, when it has one.
        const auto* normalMap = pass == LightingPass::Normals ? registry.try_get<const NormalMap2D>(item.id) : nullptr;
        const bool atlasMissing = sprite.atlasFrame.IsSet() && source.image.empty();
        const auto image = normalMap != nullptr && !normalMap->image.empty() && !atlasMissing
            ? std::string_view(normalMap->image) : source.image;
        const Texture& texture = images.TextureOf(image);
        const pxVec2 size = pixelSize * drawn.scale;

        SpriteInstance instance;
        instance.position = pxPointf(static_cast<float>(drawn.position.x), static_cast<float>(drawn.position.y));
        instance.rotation = static_cast<float>(drawn.rotation);
        instance.size = pxVec2f(size);
        instance.pivot = pxVec2f(source.pivot);
        // A region is in the image's pixels; the checkerboard of a missing image is drawn whole.
        if (const auto imageSize = HasRegion(source) ? images.Size(source.image) : std::nullopt) {
            const pxRect& r = source.region;
            instance.uv = {static_cast<float>(r.x / imageSize->x), static_cast<float>(r.y / imageSize->y),
                           static_cast<float>((r.x + r.width) / imageSize->x), static_cast<float>((r.y + r.height) / imageSize->y)};
        }
        instance.color = pass == LightingPass::Color ? sprite.color : PassColor(registry, item.id, sprite.color.a, pass);
        if (pass == LightingPass::Normals && atlasMissing) instance.color.g = 0;
        renderer.Submit(texture.Id(), sprite.filter, sprite.layer, instance);
    });
}

} // namespace

void SubmitSprites(World& world, SpriteRenderer& renderer, SpriteImages& images, double alpha,
                   const TextSources* texts, const pxRect* view) {
    Submit(world, renderer, images, alpha, texts, view, LightingPass::Color);
}

void SubmitSpriteReceivers(World& world, SpriteRenderer& renderer, SpriteImages& images, double alpha,
                           const TextSources* texts, const pxRect* view) {
    Submit(world, renderer, images, alpha, texts, view, LightingPass::Receivers);
}

void SubmitSpriteEmission(World& world, SpriteRenderer& renderer, SpriteImages& images, double alpha,
                          const TextSources* texts, const pxRect* view) {
    Submit(world, renderer, images, alpha, texts, view, LightingPass::Emission);
}

void SubmitSpriteNormals(World& world, SpriteRenderer& renderer, SpriteImages& images, double alpha,
                         const TextSources* texts, const pxRect* view) {
    Submit(world, renderer, images, alpha, texts, view, LightingPass::Normals);
}

} // namespace pixie
