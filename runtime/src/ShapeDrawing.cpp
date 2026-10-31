#include "pixie/runtime/ShapeDrawing.hpp"

#include "pixie/runtime/Culling.hpp"
#include "pixie/runtime/LightingDrawing.hpp"

#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

namespace pixie {

namespace {

void Submit(World& world, ShapeRenderer& renderer, double alpha, const pxRect* view, LightingPass pass) {
    entt::registry& registry = world.Registry();
    auto entities = registry.view<const Transform2D, const PreviousTransform2D, const Shape>();
    for (auto [id, current, previous, shape] : entities.each()) {
        if (!shape.visible) {
            continue;
        }
        const Transform2D drawn = Interpolate(previous.value, current, alpha);
        if (view != nullptr && !ShapeInView(shape, drawn, *view)) {
            continue;
        }
        const pxVec2 scale = drawn.scale * shape.size;
        ShapeInstance instance;
        instance.position = pxPointf(static_cast<float>(drawn.position.x), static_cast<float>(drawn.position.y));
        instance.rotation = static_cast<float>(drawn.rotation);
        instance.scale = pxVec2f(scale);
        instance.color = pass == LightingPass::Color ? shape.color : PassColor(registry, id, shape.color.a, pass);
        renderer.Submit(shape.kind, instance);
    }
}

} // namespace

void SubmitShapes(World& world, ShapeRenderer& renderer, double alpha, const pxRect* view) {
    Submit(world, renderer, alpha, view, LightingPass::Color);
}

void SubmitShapeReceivers(World& world, ShapeRenderer& renderer, double alpha, const pxRect* view) {
    Submit(world, renderer, alpha, view, LightingPass::Receivers);
}

void SubmitShapeEmission(World& world, ShapeRenderer& renderer, double alpha, const pxRect* view) {
    Submit(world, renderer, alpha, view, LightingPass::Emission);
}

} // namespace pixie
