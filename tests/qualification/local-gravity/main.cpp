#include <box2d/box2d.h>
#include <box2d/constants.h>
#include <box2d/math_functions.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <map>
#include <set>
#include <string_view>
#include <utility>
#include <vector>

namespace {

constexpr float kDelta = 1.0f / 60.0f;
constexpr int kSubsteps = 4;
using Pair = std::pair<int, int>;

float Length(b2Vec2 value) { return std::hypot(value.x, value.y); }
float Distance(b2Pos a, b2Pos b) { return std::hypot(float(a.x - b.x), float(a.y - b.y)); }
Pair SortedPair(int a, int b) { return std::minmax(a, b); }

struct Body {
    int tag = 0;
    int scope = 0; // Harness labels only; 0 means World-only.
    b2BodyId id{};
    b2ShapeId shape{};
    b2Vec2 effectiveGravity{};
    float gravityScale = 1;
};

struct Fixture {
    b2WorldId world{};
    std::vector<Body> bodies;
    std::map<std::uint64_t, int> shapeTags;
    std::set<Pair> touching;
    int begins = 0, ends = 0;
    int currentTick = 0;
    int deferredRequestTick = 0;
    int scopeDuringRequest = 0;
    bool inNativeStep = false;
    bool requestedInsideStep = false;

    static void RequestDeferredTransfer(b2ShapeId, b2ShapeId, b2Manifold*, void* context) {
        auto& fixture = *static_cast<Fixture*>(context);
        if (fixture.currentTick < 30 || fixture.deferredRequestTick != 0) return;
        fixture.deferredRequestTick = fixture.currentTick;
        fixture.scopeDuringRequest = fixture.Tagged(1).scope;
        fixture.requestedInsideStep = fixture.inNativeStep;
        // Only record the request. Membership and gravity are changed after the complete step.
    }

    explicit Fixture(b2Vec2 gravity) {
        b2WorldDef definition = b2DefaultWorldDef();
        definition.gravity = gravity;
        definition.workerCount = 1;
        world = b2CreateWorld(&definition);
    }
    ~Fixture() { b2DestroyWorld(world); }
    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;

    Body& Add(int tag, int scope, b2BodyType type, b2Pos position, b2Vec2 halfSize,
              b2Vec2 velocity, float density, float scale, bool bullet, b2Vec2 effectiveGravity,
              bool preSolve = false) {
        b2BodyDef definition = b2DefaultBodyDef();
        definition.type = type;
        definition.position = position;
        definition.linearVelocity = velocity;
        definition.gravityScale = scale;
        definition.isBullet = bullet;
        const b2BodyId id = b2CreateBody(world, &definition);
        b2ShapeDef shapeDefinition = b2DefaultShapeDef();
        shapeDefinition.density = density;
        shapeDefinition.enableContactEvents = true;
        shapeDefinition.enablePreSolveEvents = preSolve;
        const b2Polygon polygon = b2MakeBox(halfSize.x, halfSize.y);
        const b2ShapeId shape = b2CreatePolygonShape(id, &shapeDefinition, &polygon);
        shapeTags.emplace(b2StoreShapeId(shape), tag);
        bodies.push_back({tag, scope, id, shape, effectiveGravity, scale});
        return bodies.back();
    }

    Body& Tagged(int tag) {
        return *std::find_if(bodies.begin(), bodies.end(),
                             [tag](const Body& body) { return body.tag == tag; });
    }

    void Step(bool compensate, b2Vec2 baseGravity, int tick) {
        currentTick = tick;
        if (compensate) {
            for (const Body& body : bodies) {
                if (b2Body_GetType(body.id) != b2_dynamicBody || !b2Body_IsAwake(body.id)) continue;
                const float mass = b2Body_GetMass(body.id);
                const b2Vec2 force{
                    mass * body.gravityScale * (body.effectiveGravity.x - baseGravity.x),
                    mass * body.gravityScale * (body.effectiveGravity.y - baseGravity.y)};
                // Experimental only. The public Box2D API supports wake=false; no runtime path uses this.
                b2Body_ApplyForceToCenter(body.id, force, false);
            }
        }
        inNativeStep = true;
        b2World_Step(world, kDelta, kSubsteps);
        inNativeStep = false;
        const b2ContactEvents events = b2World_GetContactEvents(world);
        for (int i = 0; i < events.beginCount; ++i) {
            const auto& event = events.beginEvents[i];
            touching.insert(SortedPair(shapeTags.at(b2StoreShapeId(event.shapeIdA)),
                                       shapeTags.at(b2StoreShapeId(event.shapeIdB))));
            ++begins;
        }
        for (int i = 0; i < events.endCount; ++i) {
            const auto& event = events.endEvents[i];
            touching.erase(SortedPair(shapeTags.at(b2StoreShapeId(event.shapeIdA)),
                                      shapeTags.at(b2StoreShapeId(event.shapeIdB))));
            ++ends;
        }
    }
};

struct Case {
    std::string_view name;
    b2Vec2 base;
    b2Vec2 effective;
    float gravityScale = 1;
    float density = 1;
    bool floor = false;
    bool bullet = false;
    bool massEdit = false;
    bool transfer = false;
    bool crossScopeContact = false;
    bool sleep = false;
    bool differentGravityContact = false;
    bool sleepingTransfer = false;
    bool deferredTransfer = false;
};

struct Outcome {
    bool pass = true;
    bool sleepObserved = false;
    bool extraTunneling = false;
    int contactMismatchSteps = 0;
    int eventMismatchSteps = 0;
    int referencePairPeak = 0, experimentalPairPeak = 0;
    int referenceBegins = 0, experimentalBegins = 0;
    int referenceEnds = 0, experimentalEnds = 0;
    int unexpectedWakeSteps = 0;
    bool referenceTunnel = false;
    bool experimentalTunnel = false;
    bool transferContinuity = true;
    bool sleepAtTransfer = false;
    int transferWakeCalls = 0;
    int deferredRequestTick = 0;
    bool deferredAppliedAfterStep = true;
    int crossScopePairSteps = 0;
    float positionError1 = 0, positionError60 = 0, positionError600 = 0;
    float linearError1 = 0, linearError60 = 0, linearError600 = 0;
    float angularError1 = 0, angularError60 = 0, angularError600 = 0;
};

void Populate(Fixture& fixture, const Case& spec, bool reference) {
    if (spec.floor) {
        fixture.Add(10, 0, b2_staticBody, {0, 0}, {20, spec.bullet ? 0.05f : 0.2f},
                    {0, 0}, 1, 1, false, {});
    }
    const b2Pos start = spec.floor ? b2Pos{0, spec.bullet ? -5.0f : -2.0f} : b2Pos{0, 0};
    const b2Vec2 velocity = spec.bullet ? b2Vec2{0, 80} : b2Vec2{0, 0};
    fixture.Add(1, 101, b2_dynamicBody, start, {0.5f, 0.5f}, velocity,
                spec.density, spec.gravityScale, spec.bullet, spec.effective, spec.deferredTransfer);
    if (spec.crossScopeContact) {
        // Collinear different Scene gravities have a native reference: a single World gravity
        // with body 2's gravityScale at 0.5. The experimental body uses its scoped gravity at
        // full scale. Both paths therefore have the same intended acceleration in one solve.
        const float secondScale = spec.differentGravityContact && reference ? 0.5f : spec.gravityScale;
        const b2Vec2 secondGravity = spec.differentGravityContact && !reference
            ? b2Vec2{0, 5} : spec.effective;
        fixture.Add(2, 202, b2_dynamicBody, {0, -3.0f}, {0.5f, 0.5f}, {0, 0},
                    spec.density, secondScale, false, secondGravity);
    }
    if (spec.deferredTransfer) {
        b2World_SetPreSolveCallback(fixture.world, &Fixture::RequestDeferredTransfer, nullptr, &fixture);
    }
}

bool CheckVelocity(float error, float reference) {
    return error <= std::max(0.001f, 0.001f * reference);
}

Outcome Run(const Case& spec) {
    Fixture reference(spec.effective), experimental(spec.base);
    Populate(reference, spec, true);
    Populate(experimental, spec, false);
    Outcome result;
    const b2BodyId referenceId = reference.Tagged(1).id;
    const b2BodyId experimentalId = experimental.Tagged(1).id;
    const b2ShapeId referenceShape = reference.Tagged(1).shape;
    const b2ShapeId experimentalShape = experimental.Tagged(1).shape;
    const b2BodyId secondReferenceId = spec.crossScopeContact ? reference.Tagged(2).id : b2BodyId{};
    const b2BodyId secondExperimentalId = spec.crossScopeContact ? experimental.Tagged(2).id : b2BodyId{};
    const b2ShapeId secondReferenceShape = spec.crossScopeContact ? reference.Tagged(2).shape : b2ShapeId{};
    const b2ShapeId secondExperimentalShape = spec.crossScopeContact ? experimental.Tagged(2).shape : b2ShapeId{};
    bool referenceTunnel = false, experimentalTunnel = false;
    const auto transfer = [&](b2Vec2 next) {
        const b2Pos previousPosition = b2Body_GetPosition(experimentalId);
        const b2Vec2 previousVelocity = b2Body_GetLinearVelocity(experimentalId);
        const float previousAngularVelocity = b2Body_GetAngularVelocity(experimentalId);
        const auto previousContacts = experimental.touching;
        if (spec.sleepingTransfer) {
            result.sleepAtTransfer = !b2Body_IsAwake(referenceId) && !b2Body_IsAwake(experimentalId);
        }
        b2World_SetGravity(reference.world, next);
        experimental.Tagged(1).scope = 202;
        experimental.Tagged(1).effectiveGravity = next;
        b2Body_SetAwake(referenceId, true);
        b2Body_SetAwake(experimentalId, true);
        ++result.transferWakeCalls;
        const b2Pos currentPosition = b2Body_GetPosition(experimentalId);
        const b2Vec2 currentVelocity = b2Body_GetLinearVelocity(experimentalId);
        result.transferContinuity &= B2_ID_EQUALS(referenceId, reference.Tagged(1).id) &&
            B2_ID_EQUALS(experimentalId, experimental.Tagged(1).id) &&
            B2_ID_EQUALS(experimentalShape, experimental.Tagged(1).shape) &&
            currentPosition.x == previousPosition.x && currentPosition.y == previousPosition.y &&
            currentVelocity.x == previousVelocity.x && currentVelocity.y == previousVelocity.y &&
            b2Body_GetAngularVelocity(experimentalId) == previousAngularVelocity &&
            experimental.touching == previousContacts;
        result.pass &= result.transferContinuity;
    };

    for (int tick = 1; tick <= 600; ++tick) {
        if (spec.massEdit && tick == 31) {
            b2Shape_SetDensity(referenceShape, spec.density * 10, true);
            b2Shape_SetDensity(experimentalShape, spec.density * 10, true);
        }
        if (spec.transfer && tick == 61) transfer({4, 0});
        if (spec.sleepingTransfer && tick == 181) transfer({0, 0});
        const bool asleepBefore = !b2Body_IsAwake(experimentalId);
        reference.Step(false, spec.effective, tick);
        experimental.Step(true, spec.base, tick);
        if ((spec.sleep || spec.sleepingTransfer) && tick > 120) {
            result.sleepObserved |= asleepBefore;
            if (asleepBefore && b2Body_IsAwake(experimentalId)) ++result.unexpectedWakeSteps;
        }
        if (spec.deferredTransfer && result.deferredRequestTick == 0 &&
            (reference.deferredRequestTick != 0 || experimental.deferredRequestTick != 0)) {
            result.deferredAppliedAfterStep &= reference.deferredRequestTick == experimental.deferredRequestTick &&
                reference.requestedInsideStep && experimental.requestedInsideStep &&
                reference.scopeDuringRequest == 101 && experimental.scopeDuringRequest == 101 &&
                experimental.Tagged(1).scope == 101;
            if (result.deferredRequestTick == 0 &&
                reference.deferredRequestTick == tick && experimental.deferredRequestTick == tick) {
                result.deferredRequestTick = tick;
                transfer({4, 0});
            }
        }
        if (reference.touching != experimental.touching ||
            b2World_GetCounters(reference.world).contactCount != b2World_GetCounters(experimental.world).contactCount) {
            ++result.contactMismatchSteps;
        }
        if (reference.begins != experimental.begins || reference.ends != experimental.ends) {
            ++result.eventMismatchSteps;
        }
        result.referencePairPeak = std::max(result.referencePairPeak, int(reference.touching.size()));
        result.experimentalPairPeak = std::max(result.experimentalPairPeak, int(experimental.touching.size()));
        if (reference.touching.contains({1, 2}) && experimental.touching.contains({1, 2})) {
            ++result.crossScopePairSteps;
        }
        if (spec.floor) {
            const float lowerSide = spec.bullet ? 0.55f : 0.7f;
            referenceTunnel |= b2Body_GetPosition(referenceId).y > lowerSide;
            experimentalTunnel |= b2Body_GetPosition(experimentalId).y > lowerSide;
        }
        if (tick != 1 && tick != 60 && tick != 600) continue;
        float positionError = 0, linearError = 0, angularError = 0;
        for (int tag = 1; tag <= (spec.crossScopeContact ? 2 : 1); ++tag) {
            const b2BodyId refId = reference.Tagged(tag).id;
            const b2BodyId expId = experimental.Tagged(tag).id;
            const b2Pos refPosition = b2Body_GetPosition(refId);
            const b2Pos expPosition = b2Body_GetPosition(expId);
            const b2Vec2 refVelocity = b2Body_GetLinearVelocity(refId);
            const b2Vec2 expVelocity = b2Body_GetLinearVelocity(expId);
            const float refAngular = b2Body_GetAngularVelocity(refId);
            const float bodyPositionError = Distance(refPosition, expPosition);
            const float bodyLinearError = Length({refVelocity.x - expVelocity.x,
                                                   refVelocity.y - expVelocity.y});
            const float bodyAngularError = std::abs(refAngular - b2Body_GetAngularVelocity(expId));
            positionError = std::max(positionError, bodyPositionError);
            linearError = std::max(linearError, bodyLinearError);
            angularError = std::max(angularError, bodyAngularError);
            if (!CheckVelocity(bodyLinearError, Length(refVelocity)) ||
                !CheckVelocity(bodyAngularError, std::abs(refAngular))) result.pass = false;
        }
        if (tick == 1) {
            result.positionError1 = positionError;
            result.linearError1 = linearError;
            result.angularError1 = angularError;
        } else if (tick == 60) {
            result.positionError60 = positionError;
            result.linearError60 = linearError;
            result.angularError60 = angularError;
        } else {
            result.positionError600 = positionError;
            result.linearError600 = linearError;
            result.angularError600 = angularError;
        }
        if (tick == 1 && positionError > 0.1f * B2_LINEAR_SLOP) result.pass = false;
        if (tick == 60 && positionError > 0.5f * B2_LINEAR_SLOP) result.pass = false;
    }
    result.extraTunneling = experimentalTunnel && !referenceTunnel;
    result.referenceTunnel = referenceTunnel;
    result.experimentalTunnel = experimentalTunnel;
    result.referenceBegins = reference.begins;
    result.experimentalBegins = experimental.begins;
    result.referenceEnds = reference.ends;
    result.experimentalEnds = experimental.ends;
    if (result.contactMismatchSteps || result.eventMismatchSteps || result.extraTunneling || result.unexpectedWakeSteps) {
        result.pass = false;
    }
    if (spec.sleep && !result.sleepObserved) result.pass = false;
    if (spec.sleepingTransfer && (!result.sleepAtTransfer || result.transferWakeCalls != 1)) result.pass = false;
    if (spec.deferredTransfer && (result.deferredRequestTick == 0 || !result.deferredAppliedAfterStep ||
                                  result.transferWakeCalls != 1)) result.pass = false;
    if (spec.crossScopeContact && result.crossScopePairSteps == 0) result.pass = false;
    if (!B2_ID_EQUALS(referenceId, reference.Tagged(1).id) ||
        !B2_ID_EQUALS(experimentalId, experimental.Tagged(1).id) ||
        !B2_ID_EQUALS(referenceShape, reference.Tagged(1).shape) ||
        !B2_ID_EQUALS(experimentalShape, experimental.Tagged(1).shape)) result.pass = false;
    if (spec.crossScopeContact &&
        (!B2_ID_EQUALS(secondReferenceId, reference.Tagged(2).id) ||
         !B2_ID_EQUALS(secondExperimentalId, experimental.Tagged(2).id) ||
         !B2_ID_EQUALS(secondReferenceShape, reference.Tagged(2).shape) ||
         !B2_ID_EQUALS(secondExperimentalShape, experimental.Tagged(2).shape))) result.pass = false;
    return result;
}

int ObserveMixedScopeContacts() {
    Fixture experimental({0, 0});
    experimental.Add(10, 0, b2_staticBody, {0, 0}, {20, 0.2f}, {0, 0}, 1, 1, false, {});
    experimental.Add(1, 101, b2_dynamicBody, {0, -1.2f}, {0.5f, 0.5f}, {0, 0},
                     1, 1, false, {0, 10});
    experimental.Add(2, 202, b2_dynamicBody, {0, -2.2f}, {0.5f, 0.5f}, {0, 0},
                     1, 1, false, {4, 0});
    int crossScopePairSteps = 0;
    for (int tick = 0; tick < 600; ++tick) {
        experimental.Step(true, {0, 0}, tick + 1);
        if (experimental.touching.contains({1, 2})) ++crossScopePairSteps;
    }
    return crossScopePairSteps;
}

} // namespace

int main() {
    // All quantities are Box2D meters, seconds, meters/second and radians/second.
    constexpr std::array cases{
        Case{"world-zero-override", {0, 0}, {0, 10}},
        Case{"effective-zero-base-nonzero", {0, 10}, {0, 0}},
        Case{"lateral-override", {0, 10}, {4, 0}},
        Case{"gravity-scale-half", {0, 0}, {0, 10}, 0.5f},
        Case{"density-change", {0, 0}, {0, 10}, 1, 1, false, false, true},
        Case{"resting-contact", {0, 0}, {0, 10}, 1, 1, true},
        Case{"ccd-thin-floor", {0, 0}, {0, 10}, 1, 1, true, true},
        Case{"cross-scope-contact", {0, 0}, {0, 10}, 1, 1, true, false, false, false, true},
        Case{"scope-transfer", {0, 0}, {0, 10}, 1, 1, true, false, false, true},
        Case{"sleep-unchanged", {0, 0}, {0, 10}, 1, 1, true, false, false, false, false, true},
        Case{.name = "cross-scope-different-collinear", .base = {0, 0}, .effective = {0, 10},
             .floor = true, .crossScopeContact = true, .differentGravityContact = true},
        Case{.name = "sleeping-scope-transfer", .base = {0, 0}, .effective = {0, 10},
             .floor = true, .sleepingTransfer = true},
        Case{.name = "deferred-contact-transfer", .base = {0, 0}, .effective = {0, 10},
             .floor = true, .deferredTransfer = true},
    };
    std::printf("case,result,pos1_m,pos60_m,pos600_m,lin1_mps,lin60_mps,lin600_mps,"
                "ang1_radps,ang60_radps,ang600_radps,contact_mismatch_steps,event_mismatch_steps,"
                "ref_pair_peak,exp_pair_peak,ref_begins,exp_begins,ref_ends,exp_ends,"
                "ref_tunnel,exp_tunnel,extra_tunneling,sleep_observed,unexpected_wake_steps,"
                "transfer_continuity,sleep_at_transfer,transfer_wake_calls,deferred_request_tick,"
                "deferred_after_step,cross_scope_pair_steps\n");
    bool allPass = true;
    for (const Case& spec : cases) {
        const Outcome outcome = Run(spec);
        allPass &= outcome.pass;
        std::printf("%.*s,%s,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,"
                    "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
                    int(spec.name.size()), spec.name.data(), outcome.pass ? "PASS" : "FAIL",
                    outcome.positionError1, outcome.positionError60, outcome.positionError600,
                    outcome.linearError1, outcome.linearError60, outcome.linearError600,
                    outcome.angularError1, outcome.angularError60, outcome.angularError600,
                    outcome.contactMismatchSteps, outcome.eventMismatchSteps,
                    outcome.referencePairPeak, outcome.experimentalPairPeak,
                    outcome.referenceBegins, outcome.experimentalBegins, outcome.referenceEnds,
                    outcome.experimentalEnds, outcome.referenceTunnel, outcome.experimentalTunnel,
                    outcome.extraTunneling, outcome.sleepObserved, outcome.unexpectedWakeSteps,
                    outcome.transferContinuity, outcome.sleepAtTransfer, outcome.transferWakeCalls,
                    outcome.deferredRequestTick, outcome.deferredAppliedAfterStep,
                    outcome.crossScopePairSteps);
    }
    const int mixedPairSteps = ObserveMixedScopeContacts();
    std::printf("# mixed-scope-different-gravity: cross_scope_pair_steps=%d; observational only; "
                "no equivalent one-World global-gravity reference\n", mixedPairSteps);
    std::printf("experimental criteria: %s; linearSlop=%.9g m\n", allPass ? "PASS" : "FAIL", B2_LINEAR_SLOP);
    return allPass ? 0 : 1;
}
