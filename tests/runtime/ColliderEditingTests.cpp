#include "Check.hpp"
#include "pixie/runtime/ColliderEditing2D.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/World.hpp"
#include "pixie/runtime/Shape.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

using namespace pixie;

TEST(ColliderHandlesFollowGeometryUnderReflectedScaledRotatedTransforms) {
    World world;
    auto entity = world.Create("collider tool");
    auto& t = entity.Add<Transform2D>();
    t.position = {100, 50}; t.scale = {-2, 3}; t.rotation = std::numbers::pi / 2;
    auto& c = entity.Add<Collider2D>();
    c.halfSize = {4, 6}; c.offset = {5, -2}; c.rotation = std::numbers::pi / 2;
    entity.Add<Shape>(); // Drawable dimensions do not determine collider handles.
    const auto handles = DescribeColliderHandles2D(world, entity.Id(), -1);
    CHECK(handles.has_value());
    if (!handles) return;
    CHECK(std::abs(handles->center.x - 106) < 1e-8 && std::abs(handles->center.y - 40) < 1e-8);
    CHECK(std::abs(handles->resize.x - 94) < 1e-8 && std::abs(handles->resize.y - 52) < 1e-8);
    CHECK(DragCollider2D(world, entity.Id(), -1, ColliderDrag2D::Resize, {82, 58}));
    CHECK(std::abs(c.halfSize.x - 8) < 1e-8 && std::abs(c.halfSize.y - 9) < 1e-8);
    CHECK(c.offset == pxPoint(5, -2) && t.position == pxPoint(100, 50));
    CHECK(DragCollider2D(world, entity.Id(), -1, ColliderDrag2D::Rotate, {106, 20}));
    CHECK(std::abs(c.rotation) < 1e-8);
    CHECK(world.Steps() == 0);
}

TEST(ColliderToolsKeepCompoundItemsAndMetadataIndependent) {
    World world;
    auto entity = world.Create("compound tool");
    entity.Add<Transform2D>();
    auto& compound = entity.Add<CompoundCollider2D>();
    compound.shapes.push_back({Collider2D{}, {}});
    compound.shapes[1].value.offset = {200, 50};
    compound.shapes[1].unknownFields = "future data";
    const auto first = compound.shapes[0];
    CHECK(!DescribeColliderHandles2D(world, entity.Id(), -1));
    CHECK(!DescribeColliderHandles2D(world, entity.Id(), 2));
    CHECK(DragCollider2D(world, entity.Id(), 1, ColliderDrag2D::Resize, {230, 70}));
    CHECK(compound.shapes[1].value.halfSize == pxVec2(30, 20));
    CHECK(compound.shapes[0] == first);
    CHECK(compound.shapes[1].unknownFields == "future data");
    CHECK(DragCollider2D(world, entity.Id(), 1, ColliderDrag2D::Rotate, {210, 60}, true));
    CHECK(std::abs(compound.shapes[1].value.rotation - std::numbers::pi / 4) < 1e-8);
}

TEST(ColliderCircleAndPolygonResizePreserveTheirGeometryRules) {
    World world;
    auto entity = world.Create("circle tool");
    auto& t = entity.Add<Transform2D>();
    auto& c = entity.Add<Collider2D>();
    c.kind = ColliderKind2D::Circle;
    t.scale = {-2, 2};
    CHECK(DragCollider2D(world, entity.Id(), -1, ColliderDrag2D::Resize, {-60, 80}));
    CHECK(c.radius == 50);
    CHECK(!DescribeColliderHandles2D(world, entity.Id(), -1)->canRotate);
    CHECK(!DragCollider2D(world, entity.Id(), -1, ColliderDrag2D::Rotate, {60, 80}));
    t.scale = {2, 3};
    CHECK(!DescribeColliderHandles2D(world, entity.Id(), -1));
    t.scale = {1, 1};
    c.kind = ColliderKind2D::Polygon;
    c.points = {{-10, -5}, {10, -5}, {10, 5}, {-10, 5}};
    const auto before = c.points;
    CHECK(DragCollider2D(world, entity.Id(), -1, ColliderDrag2D::Resize, {20, 10}));
    for (std::size_t i = 0; i < before.size(); ++i)
        CHECK(c.points[i] == pxPoint(before[i].x * 2, before[i].y * 2));
    c.points[1] = c.points[0];
    CHECK(!DescribeColliderHandles2D(world, entity.Id(), -1));
}

TEST(ColliderDragRejectsNonfiniteAndMissingInputsWithoutChangingAuthoredData) {
    World world;
    auto entity = world.Create("guarded collider");
    entity.Add<Transform2D>();
    auto& c = entity.Add<Collider2D>();
    const auto before = c;
    CHECK(!DragCollider2D(world, entity.Id(), -1, ColliderDrag2D::Resize, {std::numeric_limits<double>::infinity(), 1}));
    CHECK(!DragCollider2D(world, entity.Id(), -1, ColliderDrag2D::Resize, {1e20, 1}));
    CHECK(!DragCollider2D(world, entity.Id(), -1, static_cast<ColliderDrag2D>(99), {1, 1}));
    CHECK(c == before);
    entity.Add<CharacterMotor2D>();
    CHECK(!DescribeColliderHandles2D(world, entity.Id(), -1));
    entity.Remove<CharacterMotor2D>();
    entity.Get<Transform2D>().scale = {0, 1};
    CHECK(!DescribeColliderHandles2D(world, entity.Id(), -1));
    entity.Destroy();
    CHECK(!DescribeColliderHandles2D(world, entity.Id(), -1));
}

TEST(CharacterCapsuleHandlesChangeRadiusAndFullHeightIndependently) {
    World world;
    auto entity = world.Create("capsule handles");
    entity.Add<Transform2D>().position = {25, 40};
    auto& motor = entity.Add<CharacterMotor2D>();
    const auto tuning = motor;
    const auto handles = DescribeColliderHandles2D(world, entity.Id(), -1);
    CHECK(handles.has_value());
    if (!handles) return;
    CHECK(handles->center == pxPoint(25, 40) && handles->resize == pxPoint(37, 40));
    CHECK(handles->height == pxPoint(25, 16) && handles->canHeight && !handles->canRotate);
    CHECK(SceneCollisionWorld2D(world) == nullptr && world.Steps() == 0);
    CHECK(DragCollider2D(world, entity.Id(), -1, ColliderDrag2D::Resize, {45, 900}));
    CHECK(motor.radius == 20 && motor.halfSegment == 12);
    CHECK(DragCollider2D(world, entity.Id(), -1, ColliderDrag2D::Height, {900, -10}));
    CHECK(motor.radius == 20 && motor.halfSegment == 30);
    const auto outline = MovementOutline2D(world, entity.Id());
    double top = 40, bottom = 40;
    for (const auto& point : outline) { top = std::min(top, point.y); bottom = std::max(bottom, point.y); }
    CHECK(std::abs(top + 10) < 1e-8 && std::abs(bottom - 90) < 1e-8);
    CHECK(DescribeColliderHandles2D(world, entity.Id(), -1)->height == pxPoint(25, -10));
    // A height below the diameter becomes the supported zero-length capsule segment.
    CHECK(DragCollider2D(world, entity.Id(), -1, ColliderDrag2D::Height, {25, 35}));
    CHECK(motor.radius == 20 && motor.halfSegment == 0);
    motor.radius = tuning.radius;
    motor.halfSegment = tuning.halfSegment;
    CHECK(motor == tuning); // Other movement/jump/gravity tuning was not touched.
    CHECK(entity.Get<Transform2D>().position == pxPoint(25, 40) && world.Steps() == 0);
}

TEST(CharacterRadiusHandlesHonorTheBackendMinimumAtEveryWorldScale) {
    for (double units : {0.001, 32.0, 1000000.0}) {
        World world;
        auto settings = world.Create("settings");
        settings.Add<PhysicsSettings2D>().unitsPerMeter = units;
        auto entity = world.Create("scaled capsule");
        entity.Add<Transform2D>();
        auto& motor = entity.Add<CharacterMotor2D>();
        motor.radius = units * 0.1;
        motor.halfSegment = units * 0.2;
        CHECK(DragCollider2D(world, entity.Id(), -1, ColliderDrag2D::Resize, {}));
        CHECK(static_cast<float>(motor.radius / units) > 0.01f);
        CHECK(motor.radius < units * 0.010001 && motor.halfSegment == units * 0.2);
        CollisionWorld2D collision(units);
        collision.ValidateMoverCapsule(motor.radius, motor.halfSegment);
        const auto motion = collision.MoveCapsule({}, motor.radius, motor.halfSegment, {}, {}, 0.5);
        CHECK(motion.position == pxPoint());
    }
}

TEST(CharacterCapsuleHandlesRejectConflictsAndInvalidDataWithoutPartialChanges) {
    World world;
    auto entity = world.Create("guarded capsule");
    auto& t = entity.Add<Transform2D>();
    auto& motor = entity.Add<CharacterMotor2D>();
    const auto before = motor;
    CHECK(!DragCollider2D(world, entity.Id(), -1, ColliderDrag2D::Rotate, {30, 30}));
    CHECK(!DragCollider2D(world, entity.Id(), 0, ColliderDrag2D::Resize, {30, 30}));
    CHECK(!DragCollider2D(world, entity.Id(), -1, ColliderDrag2D::Height, {0, std::numeric_limits<double>::infinity()}));
    CHECK(!DragCollider2D(world, entity.Id(), -1, ColliderDrag2D::Resize, {1e20, 0}));
    CHECK(motor == before);
    t.rotation = 0.1;
    CHECK(!DescribeColliderHandles2D(world, entity.Id(), -1));
    t.rotation = 0;
    t.scale = {2, 2};
    CHECK(!DescribeColliderHandles2D(world, entity.Id(), -1));
    t.scale = {1, 1};
    entity.Add<RigidBody2D>();
    CHECK(!DescribeColliderHandles2D(world, entity.Id(), -1));
    entity.Remove<RigidBody2D>();
    entity.Add<Collider2D>();
    CHECK(!DescribeColliderHandles2D(world, entity.Id(), -1));
    entity.Remove<Collider2D>();
    entity.Add<CompoundCollider2D>();
    CHECK(!DescribeColliderHandles2D(world, entity.Id(), -1));
    entity.Remove<CompoundCollider2D>();
    auto first = world.Create("first settings");
    first.Add<PhysicsSettings2D>();
    auto second = world.Create("duplicate settings");
    second.Add<PhysicsSettings2D>();
    CHECK(!DescribeColliderHandles2D(world, entity.Id(), -1));
    second.Destroy();
    CHECK(DescribeColliderHandles2D(world, entity.Id(), -1).has_value());
    motor.halfSegment = -1;
    CHECK(!DescribeColliderHandles2D(world, entity.Id(), -1));
    motor = before;
    first.Get<PhysicsSettings2D>().unitsPerMeter = 0;
    CHECK(!DescribeColliderHandles2D(world, entity.Id(), -1));
}
