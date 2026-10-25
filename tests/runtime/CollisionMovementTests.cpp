#include "Check.hpp"
#include "pixie/runtime/CharacterController2D.hpp"
#include "pixie/runtime/Velocity2D.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

using namespace pixie;
namespace {
template<typename Fn> bool Rejected(Fn fn) {
    try { fn(); } catch (const std::invalid_argument&) { return true; }
    return false;
}
void Steps(CharacterController2D& actor, const CollisionWorld2D& world, int count, double input = 0) {
    for (int i = 0; i < count; ++i) actor.Step(world, input, false, 1.0 / 60);
}
}

TEST(CollisionQueriesUseWorldUnitsAndBothFilterDirections) {
    CollisionWorld2D collision;
    const auto box = collision.AddBox({0, 100}, {50, 10}, 0, {2, 4});
    CHECK(!collision.RayCast({0, 0}, {0, 200}));
    auto hit = collision.RayCast({0, 0}, {0, 200}, {4, 2});
    CHECK(hit.has_value());
    if (hit) {
        CHECK(hit->collider == box);
        CHECK(std::abs(hit->point.y - 90) < 0.01);
        CHECK(hit->normal.y < -0.99);
        CHECK(std::abs(hit->fraction - 0.45) < 0.001);
    }
    CHECK(!collision.RayCast({0, 0}, {0, 200}, {8, 2}));
}

TEST(CollisionHandlesRejectOtherWorldsAndRemainStaleAfterReplacement) {
    CollisionWorld2D a, b;
    const auto first = a.AddCircle({0, 0}, 10);
    const auto other = b.AddCircle({0, 0}, 10);
    CHECK(!a.Contains(other));
    CHECK(!a.Remove(other));
    CHECK(Rejected([&] { a.SetTransform(other, {1, 2}); }));
    CHECK(a.Remove(first));
    const auto replacement = a.AddCircle({0, 0}, 10);
    CHECK(first != replacement);
    CHECK(!a.Contains(first));
    CHECK(Rejected([&] { a.SetTransform(first, {1, 2}); }));
    CHECK(a.Size() == 1);
}

TEST(CollisionGeometryEditsUpdateQueriesImmediately) {
    CollisionWorld2D collision;
    const auto body = collision.AddBox({0, 100}, {40, 10});
    collision.SetTransform(body, {200, 100});
    CHECK(!collision.RayCast({0, 0}, {0, 200}));
    CHECK(collision.RayCast({200, 0}, {0, 200}).has_value());
    collision.Remove(body);
    CHECK(!collision.RayCast({200, 0}, {0, 200}));
}

TEST(CollisionCircleAndPolygonReturnClosestSurfaceAndValidNormals) {
    CollisionWorld2D collision;
    const auto circle = collision.AddCircle({0, 50}, 10);
    const std::array triangle = {pxPoint{-50, 0}, pxPoint{50, 0}, pxPoint{0, -40}};
    const auto polygon = collision.AddPolygon({0, 150}, triangle);
    auto nearest = collision.RayCast({0, 0}, {0, 200});
    CHECK(nearest.has_value());
    if (nearest) {
        CHECK(nearest->collider == circle);
        CHECK(std::abs(nearest->point.y - 40) < 0.001);
    }
    collision.Remove(circle);
    nearest = collision.RayCast({0, 0}, {0, 200});
    CHECK(nearest.has_value());
    if (nearest) {
        CHECK(nearest->collider == polygon);
        CHECK(std::abs(nearest->point.y - 110) < 0.001);
        CHECK(std::abs(nearest->normal.x * nearest->normal.x + nearest->normal.y * nearest->normal.y - 1) < 0.001);
    }
}

TEST(CapsuleReportsContactCapacityOverflowInsteadOfDroppingPlanes) {
    CollisionWorld2D collision;
    for (int i = 0; i < 65; ++i) collision.AddBox({0, 110}, {500, 10});
    bool overflow = false;
    try { collision.MoveCapsule({0, 76}, 12, 12, {}, {}); }
    catch (const std::runtime_error&) { overflow = true; }
    CHECK(overflow);
}

TEST(CollisionRejectsMalformedGeometryAndNonfiniteInputs) {
    CHECK(Rejected([] { CollisionWorld2D bad(0); }));
    CollisionWorld2D collision;
    CHECK(Rejected([&] { collision.AddBox({}, {-1, 2}); }));
    CHECK(Rejected([&] { collision.AddCircle({}, 0); }));
    CHECK(Rejected([&] { collision.RayCast({}, {std::numeric_limits<double>::infinity(), 0}); }));
    const std::array line = {pxPoint{0, 0}, pxPoint{1, 0}, pxPoint{2, 0}};
    CHECK(Rejected([&] { collision.AddPolygon({}, line); }));
    CHECK(Rejected([&] { collision.MoveCapsule({}, -1, 2, {}, {}); }));
    CHECK(collision.Size() == 0);
}

TEST(CapsuleCastsStopAtThinWallsAtHighSpeed) {
    CollisionWorld2D collision;
    collision.AddBox({100, 0}, {1, 200});
    auto result = collision.MoveCapsule({-100, 0}, 12, 12, {1000, 0}, {1000, 0});
    // The pinned plane solver permits 0.005 backend meters of contact penetration.
    CHECK(std::abs(result.position.x - (87 + 0.005 * 32)) < 0.002);
    CHECK(result.position.x > 85);
    CHECK(std::abs(result.velocity.x) < 0.01);
    CHECK(!result.grounded);
}

TEST(CapsuleResolvesInitialFloorOverlapWithoutRequestedMovement) {
    CollisionWorld2D collision;
    collision.AddBox({0, 110}, {300, 10});
    auto result = collision.MoveCapsule({0, 80}, 12, 12, {}, {});
    CHECK(std::abs(result.position.y - (76 + 0.005 * 32)) < 0.002);
    CHECK(result.position.y > 75);
    CHECK(result.grounded);
}

TEST(CharacterLandsAndRemainsStationaryWithoutDrift) {
    CollisionWorld2D collision;
    collision.AddBox({0, 110}, {500, 10});
    CharacterController2D actor;
    Steps(actor, collision, 120);
    CHECK(actor.State().grounded);
    CHECK(std::abs(actor.State().position.y - (76 + 0.005 * 32)) < 0.002);
    const auto landed = actor.State().position;
    Steps(actor, collision, 300);
    CHECK(std::abs(actor.State().position.x - landed.x) < 0.001);
    CHECK(std::abs(actor.State().position.y - landed.y) < 0.001);
}

TEST(CharacterBrakesReversesAndDoesNotAccumulateWallVelocity) {
    CollisionWorld2D collision;
    collision.AddBox({0, 110}, {500, 10});
    collision.AddBox({100, 0}, {2, 100});
    CharacterController2D actor;
    Steps(actor, collision, 60);
    Steps(actor, collision, 120, 1);
    CHECK(std::abs(actor.State().position.x - (86 + 0.005 * 32)) < 0.002);
    CHECK(std::abs(actor.State().velocity.x) < 0.01);
    Steps(actor, collision, 20, -1);
    CHECK(actor.State().velocity.x < -150);
    Steps(actor, collision, 20, 0);
    CHECK(std::abs(actor.State().velocity.x) < 0.001);
}

TEST(CharacterJumpLeavesFloorAndCeilingClipsUpwardVelocity) {
    CollisionWorld2D collision;
    collision.AddBox({0, 110}, {500, 10});
    collision.AddBox({0, -10}, {500, 10});
    CharacterController2D actor({0, 70});
    Steps(actor, collision, 30);
    actor.Step(collision, 0, true, 1.0 / 60);
    CHECK(!actor.State().grounded);
    CHECK(actor.State().velocity.y < 0);
    double minY = actor.State().position.y;
    for (int i = 0; i < 15; ++i) {
        actor.Step(collision, 0, false, 1.0 / 60);
        minY = std::min(minY, actor.State().position.y);
    }
    CHECK(minY >= 24 - 0.005 * 32 - 0.002);
    Steps(actor, collision, 90);
    CHECK(actor.State().grounded);
}

TEST(CharacterClimbsWalkableRampAndRejectsSteepSupportAsGround) {
    CollisionWorld2D collision;
    collision.AddBox({0, 140}, {300, 10}, -0.25);
    CharacterSettings2D settings;
    settings.maxSpeed = 60;
    CharacterController2D actor({-80, 50}, settings);
    Steps(actor, collision, 90);
    CHECK(actor.State().grounded);
    const double startY = actor.State().position.y;
    Steps(actor, collision, 90, 1);
    CHECK(actor.State().position.x > -10);
    CHECK(actor.State().position.y < startY - 10);
    CHECK(actor.State().grounded);
    CollisionWorld2D steep;
    steep.AddBox({0, 100}, {100, 10}, -1.2);
    auto result = steep.MoveCapsule({0, 0}, 12, 12, {0, 100}, {0, 100});
    CHECK(!result.grounded);
}

TEST(CharacterLocalReplayIsRepeatableAndTeleportClearsMotion) {
    auto trace = [] {
        CollisionWorld2D collision;
        collision.AddBox({0, 110}, {500, 10});
        CharacterController2D actor;
        std::vector<double> result;
        for (int i = 0; i < 240; ++i) {
            actor.Step(collision, i < 120 ? 1 : -1, i == 80, 1.0 / 60);
            result.push_back(actor.State().position.x);
            result.push_back(actor.State().position.y);
        }
        actor.Teleport({10, 20});
        CHECK(actor.State().position.x == 10);
        CHECK(actor.State().velocity.x == 0);
        CHECK(!actor.State().grounded);
        return result;
    };
    CHECK(trace() == trace());
}

TEST(CharacterScaleConversionHasDocumentedContactTolerance) {
    CollisionWorld2D a(32), b(64);
    a.AddBox({0, 110}, {500, 10});
    b.AddBox({0, 110}, {500, 10});
    CharacterController2D first, second;
    Steps(first, a, 120);
    Steps(second, b, 120);
    CHECK(std::abs(first.State().position.y - (76 + 0.005 * 32)) < 0.002);
    CHECK(std::abs(second.State().position.y - (76 + 0.005 * 64)) < 0.002);
}

TEST(CharacterBuffersJumpJustBeforeLandingAndAllowsShortLedgeGrace) {
    CollisionWorld2D floor;
    floor.AddBox({0, 110}, {500, 10});
    CharacterController2D buffered({0, 76});
    buffered.Step(floor, 0, true, 1.0 / 60);
    CHECK(buffered.State().grounded);
    buffered.Step(floor, 0, false, 1.0 / 60);
    CHECK(buffered.State().velocity.y < -300);
    CHECK(!buffered.State().grounded);

    CollisionWorld2D ledge;
    ledge.AddBox({-200, 110}, {200, 10});
    CharacterController2D grace({-20, 30});
    Steps(grace, ledge, 60);
    bool leftSupport = false;
    for (int i = 0; i < 60; ++i) {
        const bool wasGrounded = grace.State().grounded;
        grace.Step(ledge, 1, false, 1.0 / 60);
        if (wasGrounded && !grace.State().grounded) { leftSupport = true; break; }
    }
    CHECK(leftSupport);
    grace.Step(ledge, 1, true, 1.0 / 60);
    CHECK(grace.State().velocity.y < -300);
}

TEST(CharacterRejectsInvalidConfigurationAndFrameDeltas) {
    CollisionWorld2D collision;
    CharacterController2D actor;
    CHECK(Rejected([&] { actor.Step(collision, 0, false, 0); }));
    CHECK(Rejected([&] { actor.Step(collision, 0, false, 1); }));
    auto settings = CharacterSettings2D{};
    settings.radius = 0;
    CHECK(Rejected([&] { CharacterController2D bad({}, settings); }));
}

TEST(MoverMinimumRadiusIsCheckedBeforeBackendQueriesAtEachScale) {
    for (const double scale : {1.0, 32.0, 1000.0}) {
        CollisionWorld2D world(scale);
        const double minimum = 0.01 * scale;
        for (double radius : {0.0, minimum * 0.5, minimum, std::nextafter(minimum, 1e6)}) {
            // A double barely above the boundary still rounds to the backend's forbidden float.
            CHECK(Rejected([&] { world.MoveCapsule({}, radius, 1, {}, {}, 0.5); }));
        }
        const auto motion = world.MoveCapsule({}, minimum * 1.01, 1, {1, 0}, {60, 0}, 0.5);
        CHECK(std::abs(motion.position.x - 1) < 1e-4);
        CHECK(Rejected([&] { world.ValidateMoverCapsule(minimum * 2, -1); }));
        CHECK(Rejected([&] { world.ValidateMoverCapsule(std::numeric_limits<double>::quiet_NaN(), 0); }));
    }
}

TEST(CollisionWorldDoesNotAlterLegacyWorldVelocityIntegration) {
    CollisionWorld2D collision;
    collision.AddBox({0, 100}, {20, 10});
    World world;
    auto entity = world.Create();
    entity.Add<Transform2D>();
    entity.Add<Velocity2D>().linear = {10, 20};
    world.Step(0.5, {});
    CHECK(entity.Get<Transform2D>().position.x == 5);
    CHECK(entity.Get<Transform2D>().position.y == 10);
}

TEST(CollisionMetricsCollectWithoutNativeRendering) {
    Telemetry metrics;
    metrics.SetEnabled(true);
    CollisionWorld2D collision(32, &metrics);
    collision.AddBox({0, 110}, {500, 10});
    CharacterController2D actor;
    Steps(actor, collision, 60);
    auto snapshots = metrics.Snapshot();
    // Physics adds timing/gauges to the same collector; unrelated registrations are not a regression.
    bool staticGauge = false, solveTiming = false;
    for (const auto& metric : snapshots) {
        if (metric.name == "collision.static_bodies") staticGauge = metric.gauge == 1;
        if (metric.name == "physics.solve") solveTiming = true;
    }
    CHECK(staticGauge && solveTiming);
    CHECK(metrics.Timing(metrics.RegisterTiming("collision.capsule_move")).samples == 60);
    metrics.SetEnabled(false);
    metrics.Reset();
    collision.AddBox({1000, 110}, {100, 10});
    metrics.SetEnabled(true);
    actor.Step(collision, 0, false, 1.0 / 60);
    bool countRefreshed = false;
    for (const auto& metric : metrics.Snapshot())
        if (metric.name == "collision.static_bodies") countRefreshed = metric.gauge == 2;
    CHECK(countRefreshed);
}
