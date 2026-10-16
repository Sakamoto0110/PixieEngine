#include "pixie/runtime/SpriteDrawing.hpp"

#include "pixie/runtime/Culling.hpp"

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

bool HasRegion(const Sprite& sprite) {
    return sprite.region.width > 0.0 && sprite.region.height > 0.0;
}

// The drawn rectangle in world units, before rotation, relative to the pivot: its top-left corner
// and its signed size (a negative scale mirrors it).
struct LocalRect {
    pxVec2 topLeft;
    pxVec2 size;
};

LocalRect Local(const Sprite& sprite, const Transform2D& transform, pxVec2 pixelSize) {
    const pxVec2 size = pixelSize * transform.scale;
    return {-sprite.pivot * size, size};
}

pxPoint ToWorld(const Transform2D& transform, pxVec2 local) {
    const double c = std::cos(transform.rotation);
    const double s = std::sin(transform.rotation);
    return pxPoint(transform.position.x + c * local.x - s * local.y, transform.position.y + s * local.x + c * local.y);
}

} // namespace

pxVec2 SpritePixelSize(const Sprite& sprite, SpriteImages& images) {
    if (HasRegion(sprite)) {
        return pxVec2(sprite.region.width, sprite.region.height);
    }
    if (const auto size = images.Size(sprite.image)) {
        return *size;
    }
    return pxVec2(SpriteImages::kMissingSize, SpriteImages::kMissingSize);
}

pxPoint SpritePoint(const Sprite& sprite, const Transform2D& transform, pxVec2 pixelSize, pxPoint uv) {
    const LocalRect rect = Local(sprite, transform, pixelSize);
    return ToWorld(transform, rect.topLeft + pxVec2(uv.x, uv.y) * rect.size);
}

std::optional<pxPoint> SpriteLocal(const Sprite& sprite, const Transform2D& transform, pxVec2 pixelSize,
                                   pxPoint world) {
    const LocalRect rect = Local(sprite, transform, pixelSize);
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
    return {SpritePoint(sprite, transform, pixelSize, pxPoint(0.0, 0.0)),
            SpritePoint(sprite, transform, pixelSize, pxPoint(1.0, 0.0)),
            SpritePoint(sprite, transform, pixelSize, pxPoint(1.0, 1.0)),
            SpritePoint(sprite, transform, pixelSize, pxPoint(0.0, 1.0))};
}

bool SpriteContains(const Sprite& sprite, const Transform2D& transform, pxVec2 pixelSize, pxPoint point,
                    double tolerance) {
    const LocalRect rect = Local(sprite, transform, pixelSize);
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

void SubmitSprites(World& world, SpriteRenderer& renderer, SpriteImages& images, double alpha,
                   const TextSources* texts, const pxRect* view) {
    entt::registry& registry = world.Registry();
    ForEachDrawnInOrder(world, [&](const Drawn& item) {
        const auto* previous = registry.try_get<const PreviousTransform2D>(item.id);
        const Transform2D drawn =
            previous != nullptr ? Interpolate(previous->value, item.transform, alpha) : item.transform;
        if (item.text != nullptr) {
            if (texts != nullptr && (view == nullptr || TextInView(*item.text, drawn, TextBlock(*item.text, texts->fonts), *view))) {
                SubmitText(*item.text, drawn, texts->fonts, texts->renderer, renderer, texts->zoom);
            }
            return;
        }
        const Sprite& sprite = *item.sprite;
        if (view != nullptr && !SpriteInView(sprite, drawn, SpritePixelSize(sprite, images), *view)) {
            return;
        }
        const Texture& texture = images.TextureOf(sprite.image);
        const pxVec2 size = SpritePixelSize(sprite, images) * drawn.scale;

        SpriteInstance instance;
        instance.position = pxPointf(static_cast<float>(drawn.position.x), static_cast<float>(drawn.position.y));
        instance.rotation = static_cast<float>(drawn.rotation);
        instance.size = pxVec2f(size);
        instance.pivot = pxVec2f(sprite.pivot);
        // A region is in the image's pixels; the checkerboard of a missing image is drawn whole.
        if (HasRegion(sprite) && images.Size(sprite.image)) {
            const pxVec2 image = *images.Size(sprite.image);
            const pxRect& r = sprite.region;
            instance.uv = {static_cast<float>(r.x / image.x), static_cast<float>(r.y / image.y),
                           static_cast<float>((r.x + r.width) / image.x), static_cast<float>((r.y + r.height) / image.y)};
        }
        instance.color = sprite.color;
        renderer.Submit(texture.Id(), sprite.filter, sprite.layer, instance);
    });
}

} // namespace pixie
