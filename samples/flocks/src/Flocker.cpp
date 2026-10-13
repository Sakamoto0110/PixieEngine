#include "Flocker.hpp"

#include "FlockControls.hpp"

#include "pixie/runtime/Acceleration2D.hpp"
#include "pixie/runtime/SpatialGrid.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/Velocity2D.hpp"
#include "pixie/runtime/World.hpp"

#include <algorithm>

namespace {

pxVec2 ClampLength(pxVec2 v, double maxLength) {
    const double length = glm::length(v);
    return length > maxLength && length > 0.0 ? v * (maxLength / length) : v;
}

pxVec2 WithLength(pxVec2 v, double length) {
    const double current = glm::length(v);
    return current > 1e-9 ? v * (length / current) : pxVec2(0.0);
}

} // namespace

void Flocker::Update(const pixie::FrameTime& time, const pixie::Input&) {
    const FlockControls& c = flock_.Get<FlockControls>();
    const Pointer& pointer = flock_.Get<Pointer>();
    const pixie::SpatialGrid& grid = flock_.Get<pixie::SpatialGrid>();
    const entt::registry& registry = GetWorld().Registry();
    const pixie::Entity self = Self();
    const pxPoint position = self.Get<pixie::Transform2D>().position;
    const pxVec2 velocity = self.Get<pixie::Velocity2D>().linear;

    // Each rule asks for a velocity; steering toward it is limited to maxForce per second.
    const auto steerToward = [&](pxVec2 desired) {
        return ClampLength(desired - velocity, c.maxForce);
    };

    int neighbours = 0;
    pxVec2 away(0.0);
    pxVec2 headings(0.0);
    pxVec2 offsets(0.0);
    const double separationSquared = c.separationRadius * c.separationRadius;
    grid.ForEachWithin(position, c.neighbourRadius,
                       [&](entt::entity id, pxPoint other, double distanceSquared) {
                           if (id == self.Id()) {
                               return;
                           }
                           ++neighbours;
                           const pxVec2 offset = other - position;
                           offsets += offset;
                           headings += registry.get<pixie::Velocity2D>(id).linear;
                           if (distanceSquared < separationSquared) {
                               // Closer neighbours push harder.
                               away -= offset / std::max(distanceSquared, 1e-6);
                           }
                       });

    pxVec2 steer(0.0);
    if (neighbours > 0) {
        if (glm::length(away) > 0.0) {
            steer += steerToward(WithLength(away, c.maxSpeed)) * c.separationWeight;
        }
        steer += steerToward(WithLength(headings, c.maxSpeed)) * c.alignmentWeight;
        steer += steerToward(WithLength(offsets, c.maxSpeed)) * c.cohesionWeight;
    }
    const pxVec2 toPointer = pointer.position - position;
    const double pointerDistance = glm::length(toPointer);
    if (pointer.active && pointerDistance < c.pointerRadius && pointerDistance > 1e-6) {
        const double sign = c.pointerMode == PointerMode::Attract ? 1.0 : -1.0;
        steer += steerToward(toPointer * (sign * c.maxSpeed / pointerDistance)) * c.pointerWeight;
    }

    // Keep the speed within [minSpeed, maxSpeed], then express the change as an acceleration.
    pxVec2 next = velocity + steer * time.delta;
    const double speed = glm::length(next);
    if (speed > c.maxSpeed) {
        next *= c.maxSpeed / speed;
    } else if (speed < c.minSpeed) {
        next = speed > 1e-9 ? next * (c.minSpeed / speed) : pxVec2(c.minSpeed, 0.0);
    }
    self.Get<pixie::Acceleration2D>().linear = (next - velocity) / time.delta;
}
