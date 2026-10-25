#include "Check.hpp"
#include "pixie/runtime/CollisionWorld2D.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>
using namespace pixie;
namespace {
constexpr double delta = 1.0 / 60;
BodyHandle2D Make(CollisionWorld2D& world, pxPoint at, const BodyShape2D& shape,
                  RigidBody2D definition = {}, double angle = 0) {
    return world.AddBody(at, angle, definition, {&shape, 1});
}
void Step(CollisionWorld2D& world, int n) { for (int i = 0; i < n; ++i) world.StepBodies(delta, 4); }
}
TEST(BodyTenBoxStackQualifiesPenetrationSettlingAndSleepAtTenToOneMassRatio) {
    for (double bottomDensity : {1.0, 10.0}) {
        CollisionWorld2D world;
        world.SetGravity({0, 600});
        world.AddBox({0, 200}, {300, 16});
        std::array<BodyHandle2D, 10> boxes{};
        for (int i = 0; i < 10; ++i) {
            BodyShape2D shape;
            shape.density = i == 0 ? bottomDensity : 1;
            boxes[static_cast<std::size_t>(i)] = Make(world, {0, 168.0 - 32 * i}, shape);
        }
        std::array<BodyMotion2D, 10> last{};
        std::array<double, 10> minY{}, maxY{}, minAngle{}, maxAngle{};
        minY.fill(std::numeric_limits<double>::infinity()); minAngle = minY;
        maxY.fill(-std::numeric_limits<double>::infinity()); maxAngle = maxY;
        double worstPenetration = 0;
        for (int tick = 0; tick < 3600; ++tick) {
            world.StepBodies(delta, 4);
            if (tick < 3540) continue;
            for (std::size_t i = 0; i < boxes.size(); ++i) {
                last[i] = world.BodyMotion(boxes[i]);
                minY[i] = std::min(minY[i], last[i].position.y); maxY[i] = std::max(maxY[i], last[i].position.y);
                minAngle[i] = std::min(minAngle[i], last[i].rotation); maxAngle[i] = std::max(maxAngle[i], last[i].rotation);
                CHECK(std::abs(last[i].position.x) < 1);
                const double penetration = i == 0 ? last[i].position.y + 16 - 184
                    : last[i].position.y + 32 - last[i - 1].position.y;
                worstPenetration = std::max(worstPenetration, penetration);
            }
        }
        double maxDrift = 0, maxTurn = 0;
        for (std::size_t i = 0; i < boxes.size(); ++i) {
            maxDrift = std::max(maxDrift, maxY[i] - minY[i]);
            maxTurn = std::max(maxTurn, maxAngle[i] - minAngle[i]);
        }
        std::printf("stack density %.0f: penetration %.6f u, drift %.6f u, turn %.6f rad, awake %d\n",
            bottomDensity, worstPenetration, maxDrift, maxTurn, world.Stats().awakeBodies);
        CHECK(worstPenetration <= 0.5);
        CHECK(maxDrift <= 0.25 && maxTurn <= 0.5 * std::acos(-1.0) / 180);
        CHECK(world.Stats().awakeBodies == 0);
        world.ApplyImpulse(boxes.back(), {20, -20}, last.back().position);
        CHECK(world.Stats().awakeBodies > 0);
    }
}
TEST(BodySurfaceFrictionDistinguishesHoldingFromSlidingOnTheSameSlope) {
    std::array<double, 2> displacement{};
    constexpr double angle = 0.25;
    for (int i = 0; i < 2; ++i) {
        CollisionWorld2D world;
        world.SetGravity({0, 600});
        BodyShape2D ramp; ramp.halfSize = {500, 10}; ramp.friction = static_cast<double>(i);
        RigidBody2D fixed; fixed.type = BodyType2D::Static;
        Make(world, {}, ramp, fixed, angle);
        BodyShape2D box; box.friction = static_cast<double>(i);
        RigidBody2D body; body.fixedRotation = true;
        const pxPoint start{std::sin(angle) * 26, -std::cos(angle) * 26};
        const auto handle = Make(world, start, box, body, angle);
        Step(world, 120);
        displacement[static_cast<std::size_t>(i)] = world.BodyMotion(handle).position.x - start.x;
    }
    std::printf("slope displacement: friction 0 %.6f u, friction 1 %.6f u\n", displacement[0], displacement[1]);
    CHECK(displacement[0] > 100);
    CHECK(std::abs(displacement[1]) < 0.5);
}
TEST(BodyRestitutionControlsBounceAndLowSpeedContactsSettle) {
    std::array<double, 2> fastestUp{};
    for (int i = 0; i < 2; ++i) {
        CollisionWorld2D world;
        world.SetGravity({0, 600});
        world.AddBox({0, 200}, {300, 10});
        BodyShape2D ball; ball.kind = BodyShapeKind2D::Circle; ball.restitution = static_cast<double>(i);
        auto handle = Make(world, {}, ball);
        bool approachedFloor = false;
        for (int tick = 0; tick < 120; ++tick) {
            world.StepBodies(delta);
            const auto motion = world.BodyMotion(handle);
            if (motion.position.y > 160) approachedFloor = true;
            if (approachedFloor) fastestUp[static_cast<std::size_t>(i)] = std::min(fastestUp[static_cast<std::size_t>(i)], motion.velocity.y);
        }
    }
    CHECK(fastestUp[0] > -1 && fastestUp[1] < -150);
    CollisionWorld2D slow;
    slow.AddBox({0, 200}, {300, 10});
    BodyShape2D ball; ball.kind = BodyShapeKind2D::Circle; ball.restitution = 1;
    RigidBody2D body; body.initialVelocity = {0, 20};
    auto handle = Make(slow, {0, 173}, ball, body);
    for (int tick = 0; tick < 90; ++tick) {
        slow.StepBodies(delta);
        CHECK(slow.BodyMotion(handle).velocity.y > -1);
    }
    CHECK(std::abs(slow.BodyMotion(handle).velocity.y) < 0.1);
}
TEST(BodyFastModeQualifiesThinStaticAndMovingWallsThrough1024UnitsPerSecond) {
    double worstPenetration = 0;
    for (double speed : {64.0, 256.0, 512.0, 1024.0}) for (bool moving : {false, true}) for (double mass : {0.1, 1.0, 1000.0}) {
        CollisionWorld2D world;
        BodyShape2D wall; wall.halfSize = {4, 100};
        RigidBody2D wallBody; wallBody.type = moving ? BodyType2D::Kinematic : BodyType2D::Static;
        wallBody.initialVelocity = moving ? pxVec2{-128, 0} : pxVec2{0, 0};
        auto obstacle = Make(world, {40, 0}, wall, wallBody);
        BodyShape2D projectile; projectile.halfSize = {4, 4};
        RigidBody2D body; body.initialVelocity = {speed, 0}; body.continuous = true; body.massOverride = mass;
        auto actor = Make(world, {-100, 0}, projectile, body);
        double caseWorst = 0; int worstTick = 0, firstContact = -1; double worstAngle = 0, finalPenetration = 0;
        for (int tick = 0; tick < 240; ++tick) {
            world.StepBodies(delta);
            const auto a = world.BodyMotion(actor), b = world.BodyMotion(obstacle);
            const double extent = 4 * (std::abs(std::cos(a.rotation)) + std::abs(std::sin(a.rotation)));
            const auto penetration = a.position.x + extent + 4 - b.position.x;
            worstPenetration = std::max(worstPenetration, penetration);
            if (penetration > caseWorst) { caseWorst = penetration; worstTick = tick; worstAngle = a.rotation; }
            finalPenetration = penetration;
            // No crossing: the center stays outside the wall, even during soft-contact recovery.
            CHECK(a.position.x <= b.position.x - 4);
            if (firstContact < 0 && world.Stats().contactPairs > 0) firstContact = tick;
            // The 0.5-unit penetration gate concerns settled contacts, not their first solver frame.
            if (firstContact >= 0 && tick >= firstContact + 30) CHECK(penetration <= 0.5);
        }
        CHECK(firstContact >= 0);
        std::printf("fast case speed %.0f moving %d mass %.1f: worst %.6f at %d angle %.6f; final %.6f\n",
            speed, moving ? 1 : 0, mass, caseWorst, worstTick, worstAngle, finalPenetration);
    }
    std::printf("fast mode: worst axis-aligned wall penetration %.6f u\n", worstPenetration);
}
TEST(BodyAngularEightRadiansPerSecondKeepsExpectedFreeRotation) {
    CollisionWorld2D world;
    BodyShape2D shape;
    RigidBody2D body; body.initialAngularVelocity = 8;
    auto handle = Make(world, {}, shape, body);
    Step(world, 60);
    const auto motion = world.BodyMotion(handle);
    CHECK(std::abs(motion.angularVelocity - 8) < 0.0001);
    CHECK(std::abs(std::remainder(motion.rotation - 8, 2 * std::acos(-1.0))) < 0.01);
}
