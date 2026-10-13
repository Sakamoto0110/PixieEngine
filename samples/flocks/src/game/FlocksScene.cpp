#include "FlocksScene.hpp"

#include "FlockControls.hpp"
#include "Flocker.hpp"

#include "pixie/runtime/Acceleration2D.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/SpatialIndex.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/Velocity2D.hpp"
#include "pixie/runtime/World.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <numbers>
#include <vector>

namespace {

// A color for a heading, around the hue circle, so boids moving together share a color.
pxColorRgba HeadingColor(double angle) {
    const double hue = (angle + std::numbers::pi) / (2.0 * std::numbers::pi) * 6.0;
    const auto channel = [hue](double offset) {
        const double k = std::fmod(offset + hue, 6.0);
        const double v = 1.0 - std::max(0.0, std::min({k, 4.0 - k, 1.0}));
        return static_cast<std::uint8_t>(std::lround((0.45 + 0.5 * v) * 255.0));
    };
    return pxColorRgba(channel(5.0), channel(3.0), channel(1.0));
}

void AddBoid(pixie::World& world, pixie::Entity flock, std::optional<pxPoint> at = {}) {
    pixie::Random& random = world.GetRandom();
    const pxRect area = flock.Get<Bounds>().area;
    const FlockControls& controls = flock.Get<FlockControls>();
    pixie::Entity boid = world.Create();
    const pxPoint position =
        at ? *at : pxPoint(random.Range(area.x, area.Right()), random.Range(area.y, area.Bottom()));
    const pxVec2 velocity = random.Direction() * random.Range(controls.minSpeed, controls.maxSpeed);
    const double heading = std::atan2(velocity.y, velocity.x);
    boid.Add<pixie::Transform2D>(pixie::Transform2D{position, heading});
    boid.Add<pixie::Velocity2D>(pixie::Velocity2D{velocity, 0.0});
    boid.Add<pixie::Acceleration2D>();
    boid.Add<pixie::Shape>(pixie::Shape{pixie::ShapeKind::Triangle, pxVec2(5.5, 3.5), HeadingColor(heading)});
    boid.Add<FlockMember>();
    boid.AddBehaviour<Flocker>(flock);
}

// After integration: point each boid along its velocity, color it by heading, and wrap it at the
// bounds without drawing the jump as motion.
void OrientAndWrap(pixie::World& world, const pixie::FrameTime&, const pixie::Input&) {
    for (auto [id, bounds] : world.Registry().view<const Bounds>().each()) {
        const pxRect area = bounds.area;
        auto boids = world.Registry().view<pixie::Transform2D, const pixie::Velocity2D,
                                           pixie::Shape, const FlockMember>();
        for (const entt::entity boid : boids) {
            auto& transform = boids.get<pixie::Transform2D>(boid);
            const pxVec2 velocity = boids.get<const pixie::Velocity2D>(boid).linear;
            transform.rotation = std::atan2(velocity.y, velocity.x);
            boids.get<pixie::Shape>(boid).color = HeadingColor(transform.rotation);

            pxPoint& p = transform.position;
            const pxPoint before = p;
            if (p.x < area.x) {
                p.x += area.width;
            } else if (p.x >= area.Right()) {
                p.x -= area.width;
            }
            if (p.y < area.y) {
                p.y += area.height;
            } else if (p.y >= area.Bottom()) {
                p.y -= area.height;
            }
            if (p != before) {
                world.ResetInterpolation(world.Wrap(boid));
            }
        }
    }
}

} // namespace

pixie::Entity SetUpFlocks(pixie::World& world, const pxRect& area, int count) {
    pixie::Entity flock = world.Create("flock");
    flock.Add<FlockControls>();
    flock.Add<Pointer>();
    flock.Add<Bounds>(Bounds{area});
    pixie::AddSpatialIndex<FlockMember>(world, flock, flock.Get<FlockControls>().neighbourRadius,
                                        "flocks.grid");
    world.AddSystem(pixie::SystemPhase::PostUpdate, "flocks.orient-and-wrap", &OrientAndWrap);
    SetFlockSize(world, flock, count);
    return flock;
}

void SetFlockSize(pixie::World& world, pixie::Entity flock, int count) {
    int current = FlockSize(world);
    while (current < count) {
        AddBoid(world, flock);
        ++current;
    }
    std::vector<entt::entity> extra;
    for (const entt::entity id : world.Registry().view<const FlockMember>()) {
        if (current <= count) {
            break;
        }
        extra.push_back(id);
        --current;
    }
    for (const entt::entity id : extra) {
        world.Destroy(world.Wrap(id));
    }
}

int FlockSize(pixie::World& world) {
    return static_cast<int>(world.Registry().storage<FlockMember>().size());
}

void AddBoidAt(pixie::World& world, pixie::Entity flock, pxPoint at) {
    AddBoid(world, flock, at);
}
