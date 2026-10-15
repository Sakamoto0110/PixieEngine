#include "pixie/runtime/ScreenCanvas.hpp"

#include "pixie/runtime/Fonts.hpp"
#include "pixie/runtime/SpriteImages.hpp"

#include <cmath>

namespace pixie {

ScreenCanvas::ScreenCanvas(const Camera2D& worldCamera, ShapeRenderer& shapes, PathRenderer& paths,
                           SpriteRenderer& sprites, TextRenderer& text, SpriteImages& images, Fonts& fonts)
    : worldCamera_(worldCamera), shapes_(shapes), paths_(paths), sprites_(sprites), text_(text), images_(images),
      fonts_(fonts) {}

pxVec2 ScreenCanvas::Size() const {
    const FramebufferSize viewport = worldCamera_.Viewport();
    return {static_cast<double>(viewport.width), static_cast<double>(viewport.height)};
}

Camera2D ScreenCanvas::PixelCamera(FramebufferSize viewport) {
    Camera2D camera;
    camera.SetViewport(viewport);
    camera.SetZoom(1.0);
    camera.SetCenter({viewport.width * 0.5, viewport.height * 0.5});
    return camera;
}

void ScreenCanvas::Rect(pxPoint topLeft, pxVec2 size, pxColorRgba color) {
    ShapeInstance instance;
    instance.position = pxPointf(static_cast<float>(topLeft.x + size.x * 0.5), static_cast<float>(topLeft.y + size.y * 0.5));
    instance.scale = {static_cast<float>(size.x * 0.5), static_cast<float>(size.y * 0.5)};
    instance.color = color;
    shapes_.Submit(ShapeKind::Quad, instance);
}

void ScreenCanvas::Circle(pxPoint center, double radius, pxColorRgba color) {
    ShapeInstance instance;
    instance.position = pxPointf(static_cast<float>(center.x), static_cast<float>(center.y));
    instance.scale = {static_cast<float>(radius), static_cast<float>(radius)};
    instance.color = color;
    shapes_.Submit(ShapeKind::Circle, instance);
}

void ScreenCanvas::Ring(pxPoint center, double radius, pxColorRgba color) {
    ShapeInstance instance;
    instance.position = pxPointf(static_cast<float>(center.x), static_cast<float>(center.y));
    instance.scale = {static_cast<float>(radius), static_cast<float>(radius)};
    instance.color = color;
    shapes_.Submit(ShapeKind::Ring, instance);
}

void ScreenCanvas::Shape(ShapeKind kind, const ShapeInstance& instance) {
    shapes_.Submit(kind, instance);
}

void ScreenCanvas::Sprite(std::string_view image, pxPoint topLeft, pxVec2 size, pxColorRgba tint, SpriteFilter filter) {
    SpriteInstance instance;
    instance.position = pxPointf(static_cast<float>(topLeft.x), static_cast<float>(topLeft.y));
    instance.size = {static_cast<float>(size.x), static_cast<float>(size.y)};
    instance.pivot = {0.0f, 0.0f};
    instance.color = tint;
    sprites_.Submit(images_.TextureOf(image).Id(), filter, 0, instance);
}

void ScreenCanvas::Sprite(std::string_view image, pxPoint topLeft) {
    const pxVec2 size = images_.Size(image).value_or(pxVec2{SpriteImages::kMissingSize, SpriteImages::kMissingSize});
    Sprite(image, topLeft, size);
}

void ScreenCanvas::Path(std::span<const pxPoint> points, double width, pxColorRgba color, pxColorRgba fillColor,
                        double fill) {
    paths_.Submit(points, width, color, fillColor, fill);
}

void ScreenCanvas::Text(std::string_view text, pxPoint topLeft, const TextStyle& style) {
    if (Font* font = fonts_.Find(style.font); font != nullptr) {
        text_.SubmitPixels(sprites_, *font, text, topLeft, static_cast<int>(std::lround(style.size)), style.color);
    }
}

pxVec2 ScreenCanvas::MeasureText(std::string_view text, const TextStyle& style) {
    Font* font = fonts_.Find(style.font);
    return font != nullptr ? font->Measure(text, static_cast<int>(std::lround(style.size))) : pxVec2{0.0, 0.0};
}

} // namespace pixie
