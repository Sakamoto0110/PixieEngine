#include "BoidsScene.hpp"

#include "Boid.hpp"
#include "BoidControls.hpp"

#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/Velocity2D.hpp"
#include "pixie/runtime/World.hpp"

#include <cmath>
#include <vector>

namespace {

constexpr pxColorRgba kPalette[] = {
    {120, 200, 255}, {140, 220, 190}, {170, 180, 255}, {110, 170, 235},
};

void AddBoid(pixie::World& world, pixie::Entity controls) {
    pixie::Random& random = world.GetRandom();
    const pxRect area = controls.Get<Bounds>().area;
    pixie::Entity boid = world.Create();
    const pxPoint position(random.Range(area.x, area.Right()), random.Range(area.y, area.Bottom()));
    const pxVec2 velocity = random.Direction() * controls.Get<BoidControls>().cruiseSpeed;
    boid.Add<pixie::Transform2D>(pixie::Transform2D{position, std::atan2(velocity.y, velocity.x)});
    boid.Add<pixie::Velocity2D>(pixie::Velocity2D{velocity, 0.0});
    const pxColorRgba color = kPalette[random.Int(0, static_cast<int>(std::size(kPalette)) - 1)];
    boid.Add<pixie::Shape>(pixie::Shape{pixie::ShapeKind::Triangle, pxVec2(7.0, 4.5), color});
    boid.AddBehaviour<Boid>(controls);
}

// Moves boids that left the bounds to the opposite edge, without drawing the jump as motion.
// (The view's Boid is only a filter.)
void WrapAtBounds(pixie::World& world, const pixie::FrameTime&, const pixie::Input&) {
    for (auto [id, bounds] : world.Registry().view<const Bounds>().each()) {
        const pxRect area = bounds.area;
        for (auto [boid, transform, behaviour] :
             world.Registry().view<pixie::Transform2D, const Boid>().each()) {
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

pixie::Entity SetUpBoids(pixie::World& world, const pxRect& area, int count) {
    pixie::Entity controls = world.Create("controls");
    controls.Add<BoidControls>();
    controls.Add<Pointer>();
    controls.Add<Bounds>(Bounds{area});
    world.AddSystem(pixie::SystemPhase::PostUpdate, "boids.wrap", &WrapAtBounds);
    SetBoidCount(world, controls, count);
    return controls;
}

void SetBoidCount(pixie::World& world, pixie::Entity controls, int count) {
    int current = BoidCount(world);
    while (current < count) {
        AddBoid(world, controls);
        ++current;
    }
    if (current > count) {
        std::vector<entt::entity> extra;
        for (auto [id, boid] : world.Registry().view<const Boid>().each()) {
            if (current-- <= count) {
                break;
            }
            extra.push_back(id);
        }
        for (const entt::entity id : extra) {
            world.Destroy(world.Wrap(id));
        }
    }
}

int BoidCount(pixie::World& world) {
    return static_cast<int>(world.Registry().storage<Boid>().size());
}
