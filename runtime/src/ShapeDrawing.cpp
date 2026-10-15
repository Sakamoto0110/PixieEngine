#include "pixie/runtime/ShapeDrawing.hpp"

#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

namespace pixie {

void SubmitShapes(World& world, ShapeRenderer& renderer, double alpha) {
    auto view = world.Registry().view<const Transform2D, const PreviousTransform2D, const Shape>();
    for (auto [id, current, previous, shape] : view.each()) {
        const Transform2D drawn = Interpolate(previous.value, current, alpha);
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
