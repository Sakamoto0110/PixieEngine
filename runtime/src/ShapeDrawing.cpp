#include "pixie/runtime/ShapeDrawing.hpp"

#include "pixie/runtime/Culling.hpp"

#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

namespace pixie {

void SubmitShapes(World& world, ShapeRenderer& renderer, double alpha, const pxRect* view) {
    auto entities = world.Registry().view<const Transform2D, const PreviousTransform2D, const Shape>();
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
        instance.color = shape.color;
        renderer.Submit(shape.kind, instance);
    }
}

} // namespace pixie
