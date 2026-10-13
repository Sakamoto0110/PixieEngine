#include "pixie/runtime/ShapeDrawing.hpp"

#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

#include <cmath>
#include <numbers>

namespace pixie {

namespace {

// Interpolates an angle the short way round, so a turn across ±pi does not spin backwards.
double LerpAngle(double from, double to, double t) {
    const double difference = std::remainder(to - from, 2.0 * std::numbers::pi);
    return from + difference * t;
}

} // namespace

void SubmitShapes(World& world, ShapeRenderer& renderer, double alpha) {
    auto view = world.Registry().view<const Transform2D, const PreviousTransform2D, const Shape>();
    for (auto [id, current, previous, shape] : view.each()) {
        const Transform2D& from = previous.value;
        const pxPoint position = pxPoint::Lerp(from.position, current.position, alpha);
        const pxVec2 scale = glm::mix(from.scale, current.scale, alpha) * shape.size;
        ShapeInstance instance;
        instance.position = pxPointf(static_cast<float>(position.x), static_cast<float>(position.y));
        instance.rotation = static_cast<float>(LerpAngle(from.rotation, current.rotation, alpha));
        instance.scale = pxVec2f(scale);
        instance.color = shape.color;
        renderer.Submit(shape.kind, instance);
    }
}

} // namespace pixie
