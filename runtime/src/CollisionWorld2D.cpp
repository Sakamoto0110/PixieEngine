#include "pixie/runtime/CollisionWorld2D.hpp"
#include <box2d/box2d.h>
#include <box2d/math_functions.h>
#include <array>
#include <algorithm>
#include <tuple>
#include <atomic>
#include <cfloat>
#include <cmath>
#include <format>
#include <limits>
#include <map>
#include <stdexcept>

namespace pixie {
namespace {
// IDs remain unique across service destruction/recreation, even if its allocation address is reused.
std::atomic<std::uint64_t> nextIdentity{1};
std::uint64_t Identity() {
    auto current = nextIdentity.load();
    for (;;) {
        if (current == std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("physics IDs exhausted");
        if (nextIdentity.compare_exchange_weak(current, current + 1)) return current;
    }
}
void Finite(double value) {
    if (!std::isfinite(value)) throw std::invalid_argument("collision input must be finite");
}
b2QueryFilter Filter(CollisionFilter2D filter) { return {filter.category, filter.mask}; }
struct Planes {
    std::array<b2CollisionPlane, 64> values{};
    int count = 0;
    bool overflow = false;
};
bool CollectPlane(b2ShapeId shape, const b2PlaneResult* result, void* context) {
    if (b2Shape_IsSensor(shape)) return true;
    auto& planes = *static_cast<Planes*>(context);
    if (!result->hit) return true;
    if (planes.count == static_cast<int>(planes.values.size())) {
        planes.overflow = true;
        return false;
    }
    planes.values[planes.count++] = {result->plane, FLT_MAX, 0, true};
    return true;
}
}

struct CollisionWorld2D::Impl {
    struct Shape { b2ShapeId shape{}; ColliderHandle2D handle; std::uint64_t body = 0; bool sensor = false; };
    struct Body { b2BodyId body{}; std::vector<ColliderHandle2D> shapes; };
    double units;
    pxVec2 gravity{0, 0};
    b2WorldId world;
    std::map<std::uint64_t, Body> bodies;
    std::map<std::uint64_t, Shape> shapes;
    Telemetry* telemetry;
    MetricHandle moveTime, rayTime, bodyCount, iterationCount, stepTime, staticCount;
    std::size_t statics = 0, sensorCount = 0, contacts = 0, overlaps = 0;
    std::uint64_t generation = Identity(), steps = 0;
    using PairKey = std::tuple<std::uint64_t, std::uint64_t, PhysicsPairKind2D>;
    std::map<PairKey, PhysicsEvent2D> pairs;
    std::map<std::uint64_t, std::uint64_t> backendShapes;
    std::vector<PhysicsEvent2D> events, pending;
    MetricHandle eventTime, eventCount, contactCount, overlapCount, shapeCount, sensorsMetric, awakeCount;

    Impl(double scale, Telemetry* metrics) : units(scale), telemetry(metrics) {
        Finite(scale);
        if (scale < 0.001 || scale > 1000000) throw std::invalid_argument("unitsPerMeter must be in [0.001, 1000000]");
        if (metrics) {
            moveTime = metrics->RegisterTiming("collision.capsule_move");
            rayTime = metrics->RegisterTiming("collision.raycast");
            staticCount = metrics->RegisterGauge("collision.static_bodies", "bodies");
            bodyCount = metrics->RegisterGauge("physics.bodies", "bodies");
            iterationCount = metrics->RegisterGauge("collision.last_solver_iterations", "iterations");
            stepTime = metrics->RegisterTiming("physics.solve");
            eventTime = metrics->RegisterTiming("physics.events.collect");
            eventCount = metrics->RegisterGauge("physics.events", "events");
            contactCount = metrics->RegisterGauge("physics.contact_pairs", "pairs");
            overlapCount = metrics->RegisterGauge("physics.sensor_pairs", "pairs");
            shapeCount = metrics->RegisterGauge("physics.shapes", "shapes");
            sensorsMetric = metrics->RegisterGauge("physics.sensors", "shapes");
            awakeCount = metrics->RegisterGauge("physics.awake_bodies", "bodies");
        }
        auto definition = b2DefaultWorldDef();
        definition.gravity = {0, 0};
        definition.workerCount = 1;
        world = b2CreateWorld(&definition);
    }
    ~Impl() { b2DestroyWorld(world); }
    float Number(double value) const {
        Finite(value);
        if (std::abs(value) > 1000000) throw std::invalid_argument("physics scalar exceeds supported range");
        return static_cast<float>(value);
    }
    b2Rot Rotation(double value) const {
        Number(value);
        return {static_cast<float>(std::cos(value)), static_cast<float>(std::sin(value))};
    }
    float Scalar(double value) const { return Number(value / units); }
    b2Pos Position(pxPoint p) const { return {Scalar(p.x), Scalar(p.y)}; }
    b2Vec2 Vector(pxVec2 p) const { return {Scalar(p.x), Scalar(p.y)}; }
    void Count() const {
        if (telemetry) {
            telemetry->SetGauge(bodyCount, static_cast<double>(bodies.size()));
            telemetry->SetGauge(staticCount, static_cast<double>(statics));
            telemetry->SetGauge(shapeCount, static_cast<double>(shapes.size()));
            telemetry->SetGauge(sensorsMetric, static_cast<double>(sensorCount));
            telemetry->SetGauge(contactCount, static_cast<double>(contacts));
            telemetry->SetGauge(overlapCount, static_cast<double>(overlaps));
            telemetry->SetGauge(eventCount, static_cast<double>(events.size()));
            telemetry->SetGauge(awakeCount, b2World_GetAwakeBodyCount(world));
        }
    }
    const Body& Get(BodyHandle2D handle) const {
        if (handle.owner != this || !bodies.contains(handle.value))
            throw std::invalid_argument("foreign or stale body handle");
        return bodies.at(handle.value);
    }
    const Shape& Get(ColliderHandle2D handle) const {
        if (handle.owner != this || !shapes.contains(handle.value))
            throw std::invalid_argument("foreign or stale collider handle");
        return shapes.at(handle.value);
    }
    PhysicsEndpoint2D Endpoint(std::uint64_t shape) const {
        const auto& data = shapes.at(shape);
        return {data.handle, {this, data.body}, data.sensor};
    }
    PairKey Key(std::uint64_t a, std::uint64_t b, PhysicsPairKind2D kind) const {
        if (kind == PhysicsPairKind2D::Contact && a > b) std::swap(a, b);
        return {a, b, kind};
    }
    void EndPair(std::map<PairKey, PhysicsEvent2D>::iterator it, PhysicsEndReason2D reason,
                 std::vector<PhysicsEvent2D>& destination) {
        auto event = it->second;
        event.kind = PhysicsEventKind2D::End;
        event.reason = reason;
        event.step = steps + (&destination == &pending ? 1 : 0);
        destination.push_back(event);
        if (event.pair == PhysicsPairKind2D::Contact) --contacts; else --overlaps;
        pairs.erase(it);
    }
    void EndShape(std::uint64_t shape, PhysicsEndReason2D reason) {
        for (auto it = pairs.begin(); it != pairs.end();) {
            auto current = it++;
            if (current->second.a.collider.value == shape || current->second.b.collider.value == shape)
                EndPair(current, reason, pending);
        }
    }
    void Touch(b2ShapeId a, b2ShapeId b, PhysicsPairKind2D kind, bool begin) {
        auto first = backendShapes.find(b2StoreShapeId(a)), second = backendShapes.find(b2StoreShapeId(b));
        if (first == backendShapes.end() || second == backendShapes.end()) return;
        auto key = Key(first->second, second->second, kind);
        auto it = pairs.find(key);
        if (!begin) {
            if (it != pairs.end()) EndPair(it, PhysicsEndReason2D::Separated, events);
        } else if (it == pairs.end()) {
            PhysicsEvent2D event;
            event.pair = kind;
            event.a = Endpoint(std::get<0>(key)); event.b = Endpoint(std::get<1>(key));
            event.step = steps; event.generation = generation;
            pairs.emplace(key, event);
            events.push_back(event);
            if (kind == PhysicsPairKind2D::Contact) ++contacts; else ++overlaps;
        }
    }
    void CollectEvents() {
        std::optional<TelemetryScope> timing;
        if (telemetry) timing.emplace(*telemetry, eventTime);
        const auto contact = b2World_GetContactEvents(world);
        const auto sensor = b2World_GetSensorEvents(world);
        // Ends precede fresh begins after teleport/filter edits on surviving shape identities.
        for (int i = 0; i < contact.endCount; ++i)
            Touch(contact.endEvents[i].shapeIdA, contact.endEvents[i].shapeIdB, PhysicsPairKind2D::Contact, false);
        for (int i = 0; i < sensor.endCount; ++i)
            Touch(sensor.endEvents[i].sensorShapeId, sensor.endEvents[i].visitorShapeId, PhysicsPairKind2D::Sensor, false);
        for (int i = 0; i < contact.beginCount; ++i)
            Touch(contact.beginEvents[i].shapeIdA, contact.beginEvents[i].shapeIdB, PhysicsPairKind2D::Contact, true);
        for (int i = 0; i < sensor.beginCount; ++i)
            Touch(sensor.beginEvents[i].sensorShapeId, sensor.beginEvents[i].visitorShapeId, PhysicsPairKind2D::Sensor, true);
    }
    struct RayQuery { const Impl* owner; bool sensors; std::optional<CollisionHit2D> hit; };
    static float RayHit(b2ShapeId id, b2Pos point, b2Vec2 normal, float fraction, void* context) {
        auto& query = *static_cast<RayQuery*>(context);
        const auto* shape = static_cast<const Shape*>(b2Shape_GetUserData(id));
        if (shape->sensor && !query.sensors) return -1;
        if (!query.hit || fraction < query.hit->fraction
            || (fraction == query.hit->fraction && shape->handle.value < query.hit->collider.value))
            query.hit = CollisionHit2D{shape->handle, {point.x * query.owner->units, point.y * query.owner->units},
                {normal.x, normal.y}, fraction, shape->sensor};
        return static_cast<float>(query.hit->fraction);
    }
    struct OverlapQuery { bool sensors; std::vector<ColliderHandle2D> hits; };
    static bool OverlapHit(b2ShapeId id, void* context) {
        auto& query = *static_cast<OverlapQuery*>(context);
        const auto* shape = static_cast<const Shape*>(b2Shape_GetUserData(id));
        if (query.sensors || !shape->sensor) query.hits.push_back(shape->handle);
        return true;
    }
    struct MoverQuery { b2Pos origin; b2Capsule capsule; b2Vec2 delta; float fraction = 1; };
    static bool MoverHit(b2ShapeId id, void* context) {
        if (b2Shape_IsSensor(id)) return true;
        auto& query = *static_cast<MoverQuery*>(context);
        const auto transform = b2ToRelativeTransform(b2Body_GetTransform(b2Shape_GetBody(id)), query.origin);
        const b2Vec2 points[]{b2InvTransformPoint(transform, query.capsule.center1),
                              b2InvTransformPoint(transform, query.capsule.center2)};
        b2ShapeCastInput input{};
        input.proxy = b2MakeProxy(points, 2, query.capsule.radius);
        input.translation = b2InvRotateVector(transform.q, query.delta);
        input.maxFraction = query.fraction;
        input.canEncroach = true;
        b2CastOutput output{};
        switch (b2Shape_GetType(id)) {
        case b2_polygonShape: { const auto shape = b2Shape_GetPolygon(id); output = b2CastShapePolygon(&shape, &input); break; }
        case b2_circleShape: { const auto shape = b2Shape_GetCircle(id); output = b2CastShapeCircle(&shape, &input); break; }
        case b2_capsuleShape: { const auto shape = b2Shape_GetCapsule(id); output = b2CastShapeCapsule(&shape, &input); break; }
        default: break; // This adapter creates only these shape types.
        }
        if (output.hit && output.fraction > 0) query.fraction = std::min(query.fraction, output.fraction);
        return true;
    }
    float CastMover(b2Pos origin, const b2Capsule& capsule, b2Vec2 delta, b2QueryFilter filter) const {
        if (sensorCount == 0) return b2World_CastMover(world, origin, &capsule, delta, filter);
        MoverQuery query{origin, capsule, delta};
        const float x = capsule.radius, y = std::abs(capsule.center1.y) + capsule.radius;
        const b2AABB box{{-x + std::min(0.0f, delta.x), -y + std::min(0.0f, delta.y)},
                        {x + std::max(0.0f, delta.x), y + std::max(0.0f, delta.y)}};
        b2World_OverlapAABB(world, origin, box, filter, &MoverHit, &query);
        return query.fraction;
    }
    // Build and validate every geometry before creating a body, so bad authoring never reaches an assertion.
    struct Geometry { b2Polygon polygon{}; b2Circle circle{}; b2Capsule capsule{}; BodyShapeKind2D kind; };
    Geometry GeometryFor(const BodyShape2D& data) const {
        Geometry result{};
        result.kind = data.kind;
        const auto offset = Vector({data.offset.x, data.offset.y});
        const auto angle = Number(data.rotation);
        const auto rot = Rotation(angle);
        auto local = [&](b2Vec2 p) { return b2Add(offset, b2RotateVector(rot, p)); };
        switch (data.kind) {
        case BodyShapeKind2D::Box: {
            const auto half = Vector(data.halfSize);
            if (half.x <= 0 || half.y <= 0) throw std::invalid_argument("box half sizes must be positive");
            result.polygon = b2MakeOffsetBox(half.x, half.y, offset, rot);
            break;
        }
        case BodyShapeKind2D::Circle:
        case BodyShapeKind2D::Capsule: {
            const auto radius = Scalar(data.radius), half = Scalar(data.halfSegment);
            if (radius <= 0 || half < 0) throw std::invalid_argument("invalid radius or capsule segment");
            result.circle = {offset, radius};
            result.capsule = {local({0, -half}), local({0, half}), radius};
            break;
        }
        case BodyShapeKind2D::Polygon: {
            if (data.points.size() < 3 || data.points.size() > B2_MAX_POLYGON_VERTICES)
                throw std::invalid_argument("polygon requires 3..8 hull points");
            std::array<b2Vec2, B2_MAX_POLYGON_VERTICES> vertices{};
            for (std::size_t i = 0; i < data.points.size(); ++i)
                vertices[i] = local(Vector({data.points[i].x, data.points[i].y}));
            const auto hull = b2ComputeHull(vertices.data(), static_cast<int>(data.points.size()));
            if (hull.count != static_cast<int>(data.points.size()) || !b2ValidateHull(&hull))
                throw std::invalid_argument("points must form a nondegenerate convex hull");
            result.polygon = b2MakePolygon(&hull, 0);
            break;
        }
        default: throw std::invalid_argument("unknown body shape kind");
        }
        return result;
    }
};
CollisionWorld2D::CollisionWorld2D(double scale, Telemetry* metrics)
    : impl_(std::make_unique<Impl>(scale, metrics)) {}
CollisionWorld2D::~CollisionWorld2D() = default;

BodyHandle2D CollisionWorld2D::AddBody(pxPoint position, double rotation, const RigidBody2D& data,
                                     std::span<const BodyShape2D> input) {
    auto& p = *impl_;
    if (input.empty() || input.size() > 256) throw std::invalid_argument("body requires 1..256 shapes");
    auto def = b2DefaultBodyDef();
    switch (data.type) {
    case BodyType2D::Static: def.type = b2_staticBody; break;
    case BodyType2D::Kinematic: def.type = b2_kinematicBody; break;
    case BodyType2D::Dynamic: def.type = b2_dynamicBody; break;
    default: throw std::invalid_argument("unknown body type");
    }
    def.position = p.Position(position);
    def.rotation = p.Rotation(rotation);
    def.linearVelocity = p.Vector(data.initialVelocity);
    def.angularVelocity = p.Number(data.initialAngularVelocity);
    def.gravityScale = p.Number(data.gravityScale);
    def.linearDamping = p.Number(data.linearDamping);
    def.angularDamping = p.Number(data.angularDamping);
    if (data.linearDamping < 0 || data.angularDamping < 0 || p.Number(data.massOverride) < 0
        || (data.massOverride > 0 && data.massOverride < 0.000001))
        throw std::invalid_argument("damping and mass override must be nonnegative");
    def.motionLocks.angularZ = data.fixedRotation;
    def.isBullet = data.continuous;
    std::vector<Impl::Geometry> geometry;
    double computedMass = 0;
    for (const auto& shape : input) {
        if (p.Number(shape.density) < 0 || p.Number(shape.friction) < 0
            || p.Number(shape.restitution) < 0 || shape.restitution > 1 || shape.category == 0)
            throw std::invalid_argument("invalid shape density, material or collision category");
        geometry.push_back(p.GeometryFor(shape));
        const auto& g = geometry.back();
        const auto density = shape.sensor ? 0.0f : p.Number(shape.density);
        if (g.kind == BodyShapeKind2D::Circle) computedMass += b2ComputeCircleMass(&g.circle, density).mass;
        else if (g.kind == BodyShapeKind2D::Capsule) computedMass += b2ComputeCapsuleMass(&g.capsule, density).mass;
        else computedMass += b2ComputePolygonMass(&g.polygon, density).mass;
    }
    if (data.type == BodyType2D::Dynamic && (!std::isfinite(computedMass) || computedMass < 0.000001 || computedMass > 1000000))
        throw std::invalid_argument("dynamic body requires computed mass in [0.000001, 1000000] kg");
    const BodyHandle2D handle{&p, Identity()};
    auto& record = p.bodies.try_emplace(handle.value).first->second;
    record.body = b2CreateBody(p.world, &def);
    if (def.type == b2_staticBody) ++p.statics;
    try {
        record.shapes.reserve(input.size());
        for (std::size_t i = 0; i < input.size(); ++i) {
            const auto& dataShape = input[i];
            const auto& g = geometry[i];
            const ColliderHandle2D collider{&p, Identity()};
            auto& shapeRecord = p.shapes.try_emplace(collider.value).first->second;
            record.shapes.push_back(collider);
            shapeRecord.handle = collider;
            shapeRecord.body = handle.value;
            shapeRecord.sensor = dataShape.sensor;
            auto shape = b2DefaultShapeDef();
            shape.filter.categoryBits = dataShape.category;
            shape.filter.maskBits = dataShape.mask;
            shape.userData = &shapeRecord;
            shape.density = dataShape.sensor ? 0.0f : p.Number(dataShape.density);
            shape.isSensor = dataShape.sensor;
            shape.enableSensorEvents = true;
            shape.enableContactEvents = true;
            shape.material.friction = p.Number(dataShape.friction);
            shape.material.restitution = p.Number(dataShape.restitution);
            if (g.kind == BodyShapeKind2D::Circle)
                shapeRecord.shape = b2CreateCircleShape(record.body, &shape, &g.circle);
            else if (g.kind == BodyShapeKind2D::Capsule)
                shapeRecord.shape = b2CreateCapsuleShape(record.body, &shape, &g.capsule);
            else shapeRecord.shape = b2CreatePolygonShape(record.body, &shape, &g.polygon);
            if (dataShape.sensor) ++p.sensorCount;
            p.backendShapes.emplace(b2StoreShapeId(shapeRecord.shape), collider.value);
        }
        if (data.type == BodyType2D::Dynamic) {
            auto mass = b2Body_GetMassData(record.body);
            if (!std::isfinite(mass.mass) || mass.mass <= 0 || !std::isfinite(mass.rotationalInertia))
                throw std::invalid_argument("body has unusable computed mass");
            if (data.massOverride > 0) {
                mass.rotationalInertia *= static_cast<float>(data.massOverride / mass.mass);
                mass.mass = static_cast<float>(data.massOverride);
                b2Body_SetMassData(record.body, mass);
            }
        }
    } catch (...) { RemoveBody(handle); throw; }
    p.Count();
    return handle;
}
bool CollisionWorld2D::ContainsBody(BodyHandle2D body) const {
    return body.owner == impl_.get() && impl_->bodies.contains(body.value);
}
bool CollisionWorld2D::RemoveBody(BodyHandle2D body, PhysicsEndReason2D reason) {
    if (!ContainsBody(body)) return false;
    const auto& record = impl_->Get(body);
    if (b2Body_GetType(record.body) == b2_staticBody) --impl_->statics;
    for (auto handle : record.shapes) {
        impl_->EndShape(handle.value, reason);
        const auto& shape = impl_->shapes.at(handle.value);
        impl_->backendShapes.erase(b2StoreShapeId(shape.shape));
        if (shape.sensor) --impl_->sensorCount;
    }
    b2DestroyBody(record.body);
    for (auto shape : record.shapes) impl_->shapes.erase(shape.value);
    impl_->bodies.erase(body.value);
    impl_->Count();
    return true;
}
std::vector<ColliderHandle2D> CollisionWorld2D::BodyColliders(BodyHandle2D body) const { return impl_->Get(body).shapes; }
BodyMotion2D CollisionWorld2D::BodyMotion(BodyHandle2D body) const {
    const auto id = impl_->Get(body).body;
    const auto pos = b2Body_GetPosition(id);
    const auto velocity = b2Body_GetLinearVelocity(id);
    const auto rot = b2Body_GetRotation(id);
    return {{pos.x * impl_->units, pos.y * impl_->units}, std::atan2(static_cast<double>(rot.s), static_cast<double>(rot.c)),
        {velocity.x * impl_->units, velocity.y * impl_->units}, b2Body_GetAngularVelocity(id), b2Body_IsAwake(id)};
}
BodyMass2D CollisionWorld2D::BodyMass(BodyHandle2D body) const {
    const auto mass = b2Body_GetMassData(impl_->Get(body).body);
    return {mass.mass, mass.rotationalInertia * impl_->units * impl_->units,
        {mass.center.x * impl_->units, mass.center.y * impl_->units}};
}
void CollisionWorld2D::SetBodyVelocity(BodyHandle2D body, pxVec2 velocity, double angular) {
    const auto id = impl_->Get(body).body;
    const auto v = impl_->Vector(velocity);
    const auto a = impl_->Number(angular);
    b2Body_SetLinearVelocity(id, v);
    b2Body_SetAngularVelocity(id, a);
    b2Body_SetAwake(id, true);
}
void CollisionWorld2D::TeleportBody(BodyHandle2D body, pxPoint position, double rotation, bool clear) {
    const auto id = impl_->Get(body).body;
    const auto p = impl_->Position(position);
    const auto r = impl_->Rotation(rotation);
    for (auto collider : impl_->Get(body).shapes) impl_->EndShape(collider.value, PhysicsEndReason2D::Teleported);
    b2Body_SetTransform(id, p, r);
    if (clear) { b2Body_SetLinearVelocity(id, {0, 0}); b2Body_SetAngularVelocity(id, 0); }
    b2Body_SetAwake(id, true);
}
void CollisionWorld2D::ApplyForce(BodyHandle2D body, pxVec2 force, pxPoint point) {
    const auto id = impl_->Get(body).body;
    const auto f = impl_->Vector(force);
    const auto p = impl_->Position(point);
    b2Body_ApplyForce(id, f, p, true);
}
void CollisionWorld2D::ApplyImpulse(BodyHandle2D body, pxVec2 impulse, pxPoint point) {
    const auto id = impl_->Get(body).body;
    const auto i = impl_->Vector(impulse);
    const auto p = impl_->Position(point);
    b2Body_ApplyLinearImpulse(id, i, p, true);
}
void CollisionWorld2D::ApplyTorque(BodyHandle2D body, double torque) {
    b2Body_ApplyTorque(impl_->Get(body).body, impl_->Number(torque / (impl_->units * impl_->units)), true);
}
void CollisionWorld2D::ApplyAngularImpulse(BodyHandle2D body, double impulse) {
    b2Body_ApplyAngularImpulse(impl_->Get(body).body, impl_->Number(impulse / (impl_->units * impl_->units)), true);
}
void CollisionWorld2D::SetGravity(pxVec2 gravity) {
    const auto vector = impl_->Vector(gravity);
    if (impl_->gravity == gravity) return;
    b2World_SetGravity(impl_->world, vector);
    impl_->gravity = gravity;
    // A world edit must reach resting bodies, but unchanged per-tick sync must preserve sleep.
    for (const auto& [identity, body] : impl_->bodies) {
        (void)identity;
        if (b2Body_GetType(body.body) == b2_dynamicBody) b2Body_SetAwake(body.body, true);
    }
}
void CollisionWorld2D::StepBodies(double delta, int substeps) {
    Finite(delta);
    if (delta <= 0 || delta > 0.1 || substeps < 1 || substeps > 32)
        throw std::invalid_argument("body step requires delta in (0, 0.1] and 1..32 substeps");
    impl_->events.clear();
    impl_->events.swap(impl_->pending);
    impl_->pending.clear();
    ++impl_->steps;
    for (auto& event : impl_->events) event.step = impl_->steps;
    {
        std::optional<TelemetryScope> timing;
        if (impl_->telemetry) timing.emplace(*impl_->telemetry, impl_->stepTime);
        b2World_Step(impl_->world, static_cast<float>(delta), substeps);
    }
    impl_->CollectEvents();
    impl_->Count();
}
ColliderHandle2D CollisionWorld2D::AddBox(pxPoint center, pxVec2 size, double angle, CollisionFilter2D filter) {
    BodyShape2D shape;
    shape.halfSize = size; shape.category = filter.category; shape.mask = filter.mask;
    RigidBody2D body; body.type = BodyType2D::Static;
    return BodyColliders(AddBody(center, angle, body, {&shape, 1})).front();
}
ColliderHandle2D CollisionWorld2D::AddCircle(pxPoint center, double radius, CollisionFilter2D filter) {
    BodyShape2D shape; shape.kind = BodyShapeKind2D::Circle;
    shape.radius = radius; shape.category = filter.category; shape.mask = filter.mask;
    RigidBody2D body; body.type = BodyType2D::Static;
    return BodyColliders(AddBody(center, 0, body, {&shape, 1})).front();
}
ColliderHandle2D CollisionWorld2D::AddPolygon(pxPoint center, std::span<const pxPoint> points, double angle, CollisionFilter2D filter) {
    BodyShape2D shape; shape.kind = BodyShapeKind2D::Polygon;
    shape.points.assign(points.begin(), points.end()); shape.category = filter.category; shape.mask = filter.mask;
    RigidBody2D body; body.type = BodyType2D::Static;
    return BodyColliders(AddBody(center, angle, body, {&shape, 1})).front();
}
bool CollisionWorld2D::Contains(ColliderHandle2D collider) const {
    return collider.owner == impl_.get() && impl_->shapes.contains(collider.value);
}
bool CollisionWorld2D::Remove(ColliderHandle2D collider, PhysicsEndReason2D reason) {
    if (!Contains(collider)) return false;
    // Compatibility: legacy removal owns the whole body. Compound shapes are removed through RemoveBody.
    return RemoveBody({impl_.get(), impl_->Get(collider).body}, reason);
}
void CollisionWorld2D::SetTransform(ColliderHandle2D collider, pxPoint position, double rotation) {
    TeleportBody({impl_.get(), impl_->Get(collider).body}, position, rotation, false);
}
std::size_t CollisionWorld2D::Size() const { return impl_->shapes.size(); }
std::optional<CollisionHit2D> CollisionWorld2D::RayCast(pxPoint origin, pxVec2 translation,
                                                       CollisionFilter2D filter, bool includeSensors) const {
    std::optional<TelemetryScope> timing;
    if (impl_->telemetry) timing.emplace(*impl_->telemetry, impl_->rayTime);
    impl_->Count();
    Impl::RayQuery query{impl_.get(), includeSensors, {}};
    b2World_CastRay(impl_->world, impl_->Position(origin), impl_->Vector(translation), Filter(filter), &Impl::RayHit, &query);
    return query.hit;
}
std::vector<ColliderHandle2D> CollisionWorld2D::OverlapBox(pxPoint center, pxVec2 halfSize,
                                                        CollisionFilter2D filter, bool includeSensors) const {
    const auto half = impl_->Vector(halfSize);
    if (half.x <= 0 || half.y <= 0) throw std::invalid_argument("overlap box half sizes must be positive");
    const auto box = b2MakeBox(half.x, half.y);
    const auto proxy = b2MakeProxy(box.vertices, box.count, box.radius);
    Impl::OverlapQuery query{includeSensors, {}};
    b2World_OverlapShape(impl_->world, impl_->Position(center), &proxy, Filter(filter), &Impl::OverlapHit, &query);
    std::sort(query.hits.begin(), query.hits.end(), [](auto a, auto b) { return a.value < b.value; });
    return query.hits;
}
void CollisionWorld2D::SetBodyEnabled(BodyHandle2D body, bool enabled) {
    const auto& data = impl_->Get(body);
    if (enabled) b2Body_Enable(data.body);
    else {
        for (auto shape : data.shapes) impl_->EndShape(shape.value, PhysicsEndReason2D::Disabled);
        b2Body_Disable(data.body);
    }
}
void CollisionWorld2D::SetCollisionFilter(ColliderHandle2D collider, CollisionFilter2D filter) {
    if (filter.category == 0) throw std::invalid_argument("collision category must be nonzero");
    const auto& shape = impl_->Get(collider);
    auto current = b2Shape_GetFilter(shape.shape);
    if (current.categoryBits == filter.category && current.maskBits == filter.mask) return;
    impl_->EndShape(collider.value, PhysicsEndReason2D::FilterChanged);
    current.categoryBits = filter.category; current.maskBits = filter.mask;
    b2Shape_SetFilter(shape.shape, current);
    b2Body_SetAwake(impl_->Get(BodyHandle2D{impl_.get(), shape.body}).body, true);
}
std::span<const PhysicsEvent2D> CollisionWorld2D::Events() const { return impl_->events; }
PhysicsStats2D CollisionWorld2D::Stats() const {
    return {impl_->bodies.size(), impl_->shapes.size(), impl_->sensorCount, impl_->contacts,
        impl_->overlaps, b2World_GetAwakeBodyCount(impl_->world)};
}
std::uint64_t CollisionWorld2D::Generation() const { return impl_->generation; }
bool CollisionWorld2D::NeedsSimulation() const {
    return impl_->bodies.size() > impl_->statics || impl_->sensorCount > 0 || !impl_->pending.empty();
}
void CollisionWorld2D::Reset() {
    while (!impl_->bodies.empty()) RemoveBody({impl_.get(), impl_->bodies.begin()->first}, PhysicsEndReason2D::Reset);
    impl_->events.clear();
    impl_->events.swap(impl_->pending);
    impl_->pending.clear();
    impl_->steps = 0;
    impl_->generation = Identity();
    impl_->Count();
}

void CollisionWorld2D::ValidateMoverCapsule(double radius, double halfSegment) const {
    const float r = impl_->Scalar(radius), half = impl_->Scalar(halfSegment);
    // Box2D 3.2.0's mover queries require radius > 2 * B2_LINEAR_SLOP (0.01 m).
    // Check the converted float too: rounding a barely larger double can reach the assertion.
    if (r <= 0.01f)
        throw std::invalid_argument(std::format("capsule radius must exceed {} world units at {} units per meter; increase the motor radius", 0.01 * impl_->units, impl_->units));
    if (half < 0) throw std::invalid_argument("capsule half segment must be nonnegative");
}

CapsuleMotion2D CollisionWorld2D::MoveCapsule(pxPoint center, double radius, double halfSegment,
                                             pxVec2 translation, pxVec2 velocity, double minGroundNormal,
                                             CollisionFilter2D filter) const {
    std::optional<TelemetryScope> timing;
    if (impl_->telemetry) timing.emplace(*impl_->telemetry, impl_->moveTime);
    impl_->Count();
    ValidateMoverCapsule(radius, halfSegment);
    const float r = impl_->Scalar(radius), half = impl_->Scalar(halfSegment);
    Finite(minGroundNormal);
    if (r <= 0 || half < 0 || minGroundNormal <= 0 || minGroundNormal > 1)
        throw std::invalid_argument("invalid capsule or ground threshold");
    b2Pos position = impl_->Position(center);
    const auto delta = impl_->Vector(translation);
    const auto target = b2OffsetPos(position, delta);
    const auto initialVelocity = impl_->Vector(velocity);
    const b2Capsule capsule{{0, -half}, {0, half}, r};
    const auto query = Filter(filter);
    Planes planes;
    int iterations = 0;
    auto collect = [&] {
        planes.count = 0;
        b2World_CollideMover(impl_->world, position, &capsule, query, &CollectPlane, &planes);
        if (planes.overflow) throw std::runtime_error("capsule contact plane capacity exceeded");
    };
    for (int pass = 0; pass < 8; ++pass) {
        collect();
        const b2Vec2 remaining{static_cast<float>(target.x - position.x), static_cast<float>(target.y - position.y)};
        const auto solve = b2SolvePlanes(remaining, planes.values.data(), planes.count);
        iterations += solve.iterationCount;
        const float fraction = impl_->CastMover(position, capsule, solve.delta, query);
        const auto advance = b2MulSV(fraction, solve.delta);
        position = b2OffsetPos(position, advance);
        if (b2LengthSquared(advance) < 1e-10f) break;
    }
    collect();
    // A fresh contact query has no solver push history. Clip only touching planes;
    // otherwise b2ClipVector sees zero pushes and keeps velocity into a wall.
    // Box2D's plane solver permits 0.005 m of penetration to prevent jitter.
    for (int i = 0; i < planes.count; ++i) {
        if (b2PlaneSeparation(planes.values[i].plane, {0, 0}) <= 0.0051f)
            planes.values[i].push = 1.0f;
    }
    const auto clipped = b2ClipVector(initialVelocity, planes.values.data(), planes.count);
    CapsuleMotion2D result;
    result.position = {position.x * impl_->units, position.y * impl_->units};
    result.velocity = {clipped.x * impl_->units, clipped.y * impl_->units};
    result.planeCount = planes.count;
    result.solverIterations = iterations;
    for (int i = 0; i < planes.count; ++i) {
        const auto normal = planes.values[i].plane.normal;
        if (planes.values[i].push > 0 && normal.y <= -minGroundNormal && b2Dot(initialVelocity, normal) <= 0.001f) {
            if (!result.grounded || normal.y < result.groundNormal.y) result.groundNormal = {normal.x, normal.y};
            result.grounded = true;
        }
    }
    if (impl_->telemetry) impl_->telemetry->SetGauge(impl_->iterationCount, iterations);
    return result;
}
} // namespace pixie
