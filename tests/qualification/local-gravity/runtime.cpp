#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/ParticlePhysics2D.hpp"
#include "pixie/runtime/ParticleStep.hpp"
#include "pixie/runtime/ParticleSystem.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/World.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

using namespace pixie;
namespace {
constexpr double kDelta = 1.0 / 60.0;
constexpr double kSlopMeters = 0.005;
constexpr ScopeId kRoomA{101}, kRoomB{202};
bool wakeSleepers = false, omitCompensation = false;
bool changingWorldBase = false;

double Length(pxVec2 vector) { return std::hypot(vector.x, vector.y); }
double Distance(pxPoint a, pxPoint b) { return std::hypot(a.x - b.x, a.y - b.y); }
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
bool VelocityWithin(double error, double reference) {
    return error <= std::max(0.001, reference * 0.001);
}

struct Spec {
    const char* name;
    pxVec2 base{0, 0}, effective{0, 10};
    double scale = 1;
    bool floor = false, bullet = false, wall = false, massChange = false, scaleChange = false, transfer = false;
    bool sleep = false, sleepingTransfer = false, secondBody = false, differentGravity = false, differentDirection = false;
    bool changeWorldBase = false;
};
using Event = std::tuple<int, int, int, std::uint32_t, std::uint32_t>;
std::vector<Event> Events(World& world) {
    std::vector<Event> result;
    for (const auto& event : ScenePhysicsEvents2D(world)) {
        auto a = static_cast<std::uint32_t>(event.entityA);
        auto b = static_cast<std::uint32_t>(event.entityB);
        if (b < a) std::swap(a, b);
        result.emplace_back(static_cast<int>(event.event.kind), static_cast<int>(event.event.pair),
                            static_cast<int>(event.event.reason), a, b);
    }
    std::sort(result.begin(), result.end());
    return result;
}
std::set<std::pair<std::uint32_t, std::uint32_t>> Contacts(World& world, std::initializer_list<Entity> entities) {
    std::set<std::pair<std::uint32_t, std::uint32_t>> result;
    auto* collision = ScenePhysicsWorld2D(world);
    for (const auto entity : entities) {
        if (!entity) continue;
        for (const auto& point : collision->BodyContacts(SceneBody2D(world, entity.Id()))) {
            auto a = static_cast<std::uint32_t>(SceneColliderEntity2D(world, point.a.collider));
            auto b = static_cast<std::uint32_t>(SceneColliderEntity2D(world, point.b.collider));
            if (b < a) std::swap(a, b);
            result.emplace(a, b);
        }
    }
    return result;
}

// Qualification-only adapter. Normal runtime integration is deliberately unchanged.
// It uses actual runtime scopes and the public force API, after the normal sync,
// once per complete World tick. Existing sleepers are skipped; a changed effective
// value wakes its affected body once using the explicit public velocity operation.
struct Fixture {
    World world;
    Entity settings, actor, other;
    const Spec& spec;
    bool experimental;
    std::array<pxVec2, 2> previous{};
    bool initialized = false, requested = false, oldScopeInStep = false;
    bool sleepingAtTransfer = false, transferContinuity = true;
    int changeWakeCalls = 0;

    Fixture(const Spec& value, bool candidate) : spec(value), experimental(candidate) {
        settings = world.Create("settings");
        auto& physics = settings.Add<PhysicsSettings2D>();
        physics.gravity = candidate ? spec.base : spec.effective;
        physics.unitsPerMeter = 1;
        SceneGravity2D first; first.mode = SceneGravityMode2D::Override; first.value = spec.effective;
        SceneGravity2D next; next.mode = SceneGravityMode2D::Override;
        next.value = spec.secondBody ? (spec.differentGravity ? spec.effective * 0.5 : spec.effective) : pxVec2{4, 0};
        if (spec.differentDirection) next.value = {4, 0};
        Require(world.CreateScope(kRoomA, "A", first) && world.CreateScope(kRoomB, "B", next), "create scopes");
        if (spec.floor) {
            auto floor = world.Create("floor");
            floor.Add<Transform2D>();
            floor.Add<Collider2D>().halfSize = spec.wall ? pxVec2{0.01, 20} : pxVec2{20, spec.bullet ? 0.01 : 0.2};
        }
        actor = AddBody("actor", spec.wall ? pxPoint{-2, 0} : pxPoint{0, spec.bullet ? -2.0 : -1.2}, spec.scale);
        if (spec.secondBody) {
            other = AddBody("other", {0, -2.2}, !candidate && spec.differentGravity ? spec.scale * 0.5 : spec.scale);
            Require(world.SetScope(other, kRoomB), "assign second body to a distinct actual Scene");
        }
        InstallSceneMovement2D(world);
        if (candidate) world.AddSystem(SystemPhase::PreUpdate, "qualification.scope-force",
            [this](World&, const FrameTime&, const Input&) { Compensate(); });
        world.AddSystem(SystemPhase::Physics, "qualification.scope-transfer",
            [this](World& current, const FrameTime&, const Input&) {
                if ((!spec.transfer && !spec.sleepingTransfer) || requested) return;
                auto* physicsWorld = ScenePhysicsWorld2D(current);
                const auto handle = SceneBody2D(current, actor.Id());
                const auto motion = physicsWorld->BodyMotion(handle);
                if (spec.sleepingTransfer ? motion.awake || current.Steps() < 120 : current.Steps() != 59) return;
                sleepingAtTransfer = !motion.awake;
                requested = true;
                Require(current.SetScope(actor, kRoomB), "deferred SetScope request");
                oldScopeInStep = current.ScopeOf(actor) == kRoomA;
                const auto unchanged = physicsWorld->BodyMotion(handle);
                transferContinuity &= SceneBody2D(current, actor.Id()) == handle &&
                    unchanged.position == motion.position && unchanged.velocity == motion.velocity &&
                    unchanged.angularVelocity == motion.angularVelocity;
            });
    }
    Entity AddBody(const char* name, pxPoint position, double scale) {
        auto entity = world.Create(name);
        entity.Add<Transform2D>().position = position;
        entity.Add<Collider2D>().halfSize = {0.5, 0.5};
        auto& body = entity.Add<RigidBody2D>();
        body.gravityScale = scale;
        body.continuous = spec.bullet;
        if (spec.bullet) body.initialVelocity = spec.wall ? pxVec2{100, 0} : pxVec2{0, 100};
        Require(world.SetScope(entity, kRoomA), "assign actor");
        return entity;
    }
    void Compensate() {
        auto* collision = ScenePhysicsWorld2D(world);
        std::size_t index = 0;
        for (const auto entity : {actor, other}) {
            if (!entity) continue;
            const auto handle = SceneBody2D(world, entity.Id());
            Require(collision && collision->ContainsBody(handle), "synced body before qualification force");
            const auto resolved = world.ResolveGravity(entity);
            Require(static_cast<bool>(resolved), "resolve actual runtime scope gravity");
            auto motion = collision->BodyMotion(handle);
            if (initialized && resolved.value != previous[index]) {
                collision->SetBodyVelocity(handle, motion.velocity, motion.angularVelocity);
                motion = collision->BodyMotion(handle);
                ++changeWakeCalls;
            }
            previous[index++] = resolved.value;
            if (!motion.awake && !wakeSleepers) continue;
            const auto mass = collision->BodyMass(handle);
            const auto delta = resolved.value - collision->Gravity();
            const auto force = delta * (mass.mass * entity.Get<RigidBody2D>().gravityScale);
            if (!omitCompensation && (force.x != 0 || force.y != 0))
                collision->ApplyForce(handle, force, collision->BodyWorldPoint(handle, mass.localCenter));
        }
        initialized = true;
    }
};

bool BodyComparison(const Spec& spec) {
    Fixture reference(spec, false), candidate(spec, true);
    bool pass = true, sleepObserved = false, referenceTunnel = false, candidateTunnel = false;
    int contacts = 0, events = 0, sleepDifferences = 0, transferTick = 0, crossScopePairSteps = 0;
    int worldBaseChanges = 0, unexpectedEffectiveChanges = 0;
    bool asleepBeforeFirstBaseChange = false, stableBodyIdentity = true;
    BodyHandle2D referenceIdentity, candidateIdentity;
    std::vector<ColliderHandle2D> referenceShapes, candidateShapes;
    double maximumPositionAllSteps = 0, maximumVelocityAllSteps = 0;
    std::array<double, 3> position{}, linear{}, angular{};
    for (int tick = 1; tick <= 600; ++tick) {
        if (spec.massChange && tick == 30) {
            reference.actor.Get<RigidBody2D>().massOverride = 3;
            candidate.actor.Get<RigidBody2D>().massOverride = 3;
        }
        if (spec.scaleChange && tick == 30) {
            reference.actor.Get<RigidBody2D>().gravityScale = 1.5;
            candidate.actor.Get<RigidBody2D>().gravityScale = 1.5;
        }
        if (spec.changeWorldBase && tick >= 180 && tick % 60 == 0) {
            // A constant Scene Override is independent of changes to the World
            // base. The reference keeps native global gravity10 throughout;
            // only the experimental World base changes. Both initial bodies
            // must already be sleeping for the unchanged-effective wake test.
            const std::array<pxVec2, 8> bases{{{0, 20}, {0, -5}, {0, 0}, {0, 50},
                                              {0, 0}, {4, 0}, {0, 0}, {0, 20}}};
            if (worldBaseChanges == 0) {
                auto* a = ScenePhysicsWorld2D(reference.world);
                auto* b = ScenePhysicsWorld2D(candidate.world);
                asleepBeforeFirstBaseChange = !a->BodyMotion(referenceIdentity).awake &&
                                               !b->BodyMotion(candidateIdentity).awake;
            }
            candidate.settings.Get<PhysicsSettings2D>().gravity = bases[static_cast<std::size_t>(worldBaseChanges)];
            ++worldBaseChanges;
        }
        reference.world.Step(kDelta, {});
        candidate.world.Step(kDelta, {});
        pass &= SceneMovementErrors2D(reference.world).empty() && SceneMovementErrors2D(candidate.world).empty();
        auto* refPhysics = ScenePhysicsWorld2D(reference.world);
        auto* expPhysics = ScenePhysicsWorld2D(candidate.world);
        if (spec.changeWorldBase) {
            if (tick == 1) {
                referenceIdentity = SceneBody2D(reference.world, reference.actor.Id());
                candidateIdentity = SceneBody2D(candidate.world, candidate.actor.Id());
                referenceShapes = refPhysics->BodyColliders(referenceIdentity);
                candidateShapes = expPhysics->BodyColliders(candidateIdentity);
            }
            stableBodyIdentity &= SceneBody2D(reference.world, reference.actor.Id()) == referenceIdentity &&
                                  SceneBody2D(candidate.world, candidate.actor.Id()) == candidateIdentity &&
                                  refPhysics->BodyColliders(referenceIdentity) == referenceShapes &&
                                  expPhysics->BodyColliders(candidateIdentity) == candidateShapes;
            const auto effective = candidate.world.ResolveGravity(candidate.actor);
            if (!effective || effective.value != spec.effective) ++unexpectedEffectiveChanges;
        }
        if (refPhysics->Stats().contactPairs != expPhysics->Stats().contactPairs ||
            Contacts(reference.world, {reference.actor, reference.other}) !=
            Contacts(candidate.world, {candidate.actor, candidate.other})) ++contacts;
        if (Events(reference.world) != Events(candidate.world)) ++events;
        if (spec.secondBody) {
            auto a = reference.actor.Number(), b = reference.other.Number();
            if (b < a) std::swap(a, b);
            if (Contacts(reference.world, {reference.actor, reference.other}).contains({a, b}) &&
                Contacts(candidate.world, {candidate.actor, candidate.other}).contains({a, b})) ++crossScopePairSteps;
        }
        const auto refMotion = refPhysics->BodyMotion(SceneBody2D(reference.world, reference.actor.Id()));
        const auto expMotion = expPhysics->BodyMotion(SceneBody2D(candidate.world, candidate.actor.Id()));
        if (spec.changeWorldBase) {
            maximumPositionAllSteps = std::max(maximumPositionAllSteps, Distance(refMotion.position, expMotion.position));
            maximumVelocityAllSteps = std::max(maximumVelocityAllSteps, Length(refMotion.velocity - expMotion.velocity));
        }
        if (refMotion.awake != expMotion.awake) ++sleepDifferences;
        sleepObserved |= !refMotion.awake && !expMotion.awake;
        if (spec.floor) {
            referenceTunnel |= (spec.wall ? refMotion.position.x : refMotion.position.y) > 0.6;
            candidateTunnel |= (spec.wall ? expMotion.position.x : expMotion.position.y) > 0.6;
        }
        if (candidate.requested && transferTick == 0) {
            transferTick = tick;
            pass &= reference.requested && reference.oldScopeInStep && candidate.oldScopeInStep;
            pass &= reference.world.ScopeOf(reference.actor) == kRoomB && candidate.world.ScopeOf(candidate.actor) == kRoomB;
            reference.settings.Get<PhysicsSettings2D>().gravity = {4, 0};
        }
        const int sample = tick == 1 ? 0 : tick == 60 ? 1 : tick == 600 ? 2 : -1;
        if (sample < 0) continue;
        for (const auto pair : {std::pair{reference.actor, candidate.actor}, std::pair{reference.other, candidate.other}}) {
            if (!pair.first) continue;
            const auto a = refPhysics->BodyMotion(SceneBody2D(reference.world, pair.first.Id()));
            const auto b = expPhysics->BodyMotion(SceneBody2D(candidate.world, pair.second.Id()));
            const auto p = Distance(a.position, b.position);
            const auto v = Length(a.velocity - b.velocity);
            const auto w = std::abs(a.angularVelocity - b.angularVelocity);
            position[sample] = std::max(position[sample], p);
            linear[sample] = std::max(linear[sample], v);
            angular[sample] = std::max(angular[sample], w);
            pass &= VelocityWithin(v, Length(a.velocity)) && VelocityWithin(w, std::abs(a.angularVelocity));
            if (tick == 1) pass &= p <= 0.1 * kSlopMeters;
            if (tick == 60) pass &= p <= 0.5 * kSlopMeters;
        }
    }
    pass &= contacts == 0 && events == 0 && sleepDifferences == 0 && !(candidateTunnel && !referenceTunnel);
    if (spec.sleep || spec.sleepingTransfer) pass &= sleepObserved;
    if (spec.secondBody) pass &= crossScopePairSteps > 0;
    if (spec.transfer || spec.sleepingTransfer)
        pass &= transferTick != 0 && candidate.transferContinuity && reference.transferContinuity && candidate.changeWakeCalls == 1;
    if (spec.sleepingTransfer) pass &= candidate.sleepingAtTransfer && reference.sleepingAtTransfer;
    if (spec.changeWorldBase) {
        pass &= worldBaseChanges == 8 && asleepBeforeFirstBaseChange && stableBodyIdentity && unexpectedEffectiveChanges == 0;
        std::printf("# changing-world-base: changes=%d,asleep_before_first_change=%d,stable_body_and_shape_ids=%d,"
                    "unexpected_effective_changes=%d,all_step_max_position_m=%.9g,all_step_max_velocity_mps=%.9g; "
                    "reference native global=(0 10),experimental Scene Override=(0 10) remains fixed\n",
                    worldBaseChanges, asleepBeforeFirstBaseChange, stableBodyIdentity, unexpectedEffectiveChanges,
                    maximumPositionAllSteps, maximumVelocityAllSteps);
    }
    std::printf("body,%s,%s,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%d,%d,%d,%d,%d,%d,%d,%d\n",
        spec.name, pass ? "PASS" : "FAIL", position[0], position[1], position[2], linear[0], linear[1], linear[2],
        angular[0], angular[1], angular[2], contacts, events, sleepDifferences, referenceTunnel, candidateTunnel,
        transferTick, candidate.changeWakeCalls, crossScopePairSteps);
    return pass;
}

// Local motor/particle integration uses its normal production path here. The
// independent unit regressions compare full1/60/600-step native references.
bool ConsumerBoundary(bool particles, bool withBackend) {
    World world;
    world.Create("settings").Add<PhysicsSettings2D>().gravity = {0, 600};
    SceneGravity2D gravity; gravity.mode = SceneGravityMode2D::Override;
    gravity.value = particles ? pxVec2{900, 0} : pxVec2{0, 0};
    Require(world.CreateScope(kRoomA, "local", gravity), "consumer scope");
    Entity entity = world.Create(particles ? "pool" : "motor");
    Require(world.SetScope(entity, kRoomA), "consumer membership");
    if (particles) {
        entity.Add<ParticleSystem>().EmitDynamic({0, 0}, {0, 0}, 20);
        entity.Add<ParticlePhysics2D>().gravityScale = 0.5;
        AddParticleStep(world);
    } else {
        entity.Add<Transform2D>();
        entity.Add<CharacterMotor2D>().gravityScale = 0.5;
    }
    if (withBackend || !particles) InstallSceneMovement2D(world);
    world.Step(kDelta, {});
    pxVec2 velocity;
    if (particles) {
        const auto& pool = entity.Get<ParticleSystem>().particles;
        Require(pool.size() == 1, "particle remains alive");
        velocity = {pool[0].velocity.x, pool[0].velocity.y};
    } else {
        const auto* state = CharacterMotion2D(world, entity.Id());
        Require(state != nullptr, "zero-gravity local motor remains valid");
        velocity = state->velocity;
    }
    const auto localExpected = particles ? pxVec2{7.5, 0} : pxVec2{0, 0};
    const bool pass = Length(velocity - localExpected) < 0.001 && world.ResolveGravity(entity).value == gravity.value;
    std::printf("local-consumer,%s-%s,%s,velocity=(%.9g %.9g)\n",
        particles ? "particle" : "motor", withBackend ? "backend" : "no-backend", pass ? "PASS" : "FAIL", velocity.x, velocity.y);
    return pass;
}

bool MixedDirectionObservation() {
    const Spec spec{.name="non-collinear-actual-scopes", .floor=true, .secondBody=true, .differentDirection=true};
    Fixture candidate(spec, true);
    int crossScenePairs = 0;
    auto a = candidate.actor.Number(), b = candidate.other.Number();
    if (b < a) std::swap(a, b);
    for (int tick = 1; tick <= 600; ++tick) {
        candidate.world.Step(kDelta, {});
        if (Contacts(candidate.world, {candidate.actor, candidate.other}).contains({a, b})) ++crossScenePairs;
        if (!SceneMovementErrors2D(candidate.world).empty()) return false;
    }
    std::printf("observation,%s,NOT-COMPARABLE,cross_scene_pair_steps=%d; no equivalent one-World global-gravity reference\n",
                spec.name, crossScenePairs);
    return crossScenePairs > 0;
}

bool SceneGlobalBoundary(const char* name) {
    ComponentTypes types;
    DescribeRuntimeComponents(types);
    World reference, scoped;
    const auto path = std::filesystem::path(PIXIE_SCOPE_QUALIFICATION_SCENE_ROOT) / name / "scenes/main.scene";
    Require(LoadSceneFile(reference, types, path).ok && LoadSceneFile(scoped, types, path).ok,
            "load real project scene without rewriting it");
    SceneGravity2D lateral; lateral.mode = SceneGravityMode2D::Inherit; lateral.value = {900, 0};
    SceneGravity2D zero; zero.mode = SceneGravityMode2D::Multiply; zero.multiplier = 1; zero.value = {0, 0};
    Require(scoped.CreateScope(kRoomA, "Qualification lateral", lateral) && scoped.CreateScope(kRoomB, "Qualification zero", zero),
            "add actual runtime scopes to loaded scene");
    std::map<std::uint64_t, Entity> original;
    for (const auto [id, persistent] : reference.Registry().view<const SceneId>().each())
        original.emplace(persistent.value, reference.Wrap(id));
    std::vector<std::pair<Entity, Entity>> pairs;
    for (const auto [id, persistent] : scoped.Registry().view<const SceneId>().each()) {
        auto entity = scoped.Wrap(id);
        Require(scoped.SetScope(entity, (persistent.value & 1) ? kRoomA : kRoomB), "assign loaded entity membership");
        pairs.emplace_back(original.at(persistent.value), entity);
    }
    InstallSceneMovement2D(reference); InstallSceneMovement2D(scoped);
    AddParticleStep(reference); AddParticleStep(scoped);
    bool pass = true;
    double maxPosition = 0, maxVelocity = 0;
    int contactMismatch = 0, eventMismatch = 0;
    for (int tick = 1; tick <= 600; ++tick) {
        reference.Step(kDelta, {}); scoped.Step(kDelta, {});
        pass &= SceneMovementErrors2D(reference) == SceneMovementErrors2D(scoped);
        pass &= ParticlePhysicsErrors2D(reference) == ParticlePhysicsErrors2D(scoped);
        auto* aPhysics = ScenePhysicsWorld2D(reference);
        auto* bPhysics = ScenePhysicsWorld2D(scoped);
        if (static_cast<bool>(aPhysics) != static_cast<bool>(bPhysics)) pass = false;
        if (aPhysics && bPhysics && aPhysics->Stats().contactPairs != bPhysics->Stats().contactPairs) ++contactMismatch;
        if (Events(reference) != Events(scoped)) ++eventMismatch;
        if (tick != 1 && tick != 60 && tick != 600) continue;
        for (const auto& [a, b] : pairs) {
            if (a.Has<Transform2D>() && b.Has<Transform2D>())
                maxPosition = std::max(maxPosition, Distance(a.Get<Transform2D>().position, b.Get<Transform2D>().position));
            if (a.Has<CharacterMotor2D>()) {
                const auto* aMotion = CharacterMotion2D(reference, a.Id());
                const auto* bMotion = CharacterMotion2D(scoped, b.Id());
                pass &= static_cast<bool>(aMotion) == static_cast<bool>(bMotion);
                if (aMotion && bMotion) maxVelocity = std::max(maxVelocity, Length(aMotion->velocity - bMotion->velocity));
            }
            if (a.Has<ParticleSystem>()) {
                const auto& aParticles = a.Get<ParticleSystem>().particles;
                const auto& bParticles = b.Get<ParticleSystem>().particles;
                pass &= aParticles.size() == bParticles.size();
                for (std::size_t i = 0; i < std::min(aParticles.size(), bParticles.size()); ++i) {
                    maxPosition = std::max(maxPosition, std::hypot(double(aParticles[i].position.x - bParticles[i].position.x),
                                                                  double(aParticles[i].position.y - bParticles[i].position.y)));
                    maxVelocity = std::max(maxVelocity, std::hypot(double(aParticles[i].velocity.x - bParticles[i].velocity.x),
                                                                  double(aParticles[i].velocity.y - bParticles[i].velocity.y)));
                }
            }
        }
    }
    pass &= maxPosition == 0 && maxVelocity == 0 && contactMismatch == 0 && eventMismatch == 0;
    std::printf("global-scene,%s,%s,max_position=%.9g,max_velocity=%.9g,contact_mismatch=%d,event_mismatch=%d\n",
                name, pass ? "PASS" : "FAIL", maxPosition, maxVelocity, contactMismatch, eventMismatch);
    return pass;
}
} // namespace

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        const std::string_view argument(argv[i]);
        if (argument == "--wake-sleepers") wakeSleepers = true;
        else if (argument == "--omit-compensation") omitCompensation = true;
        else if (argument == "--changing-world-base") changingWorldBase = true;
        else { std::fprintf(stderr, "unknown qualification argument: %s\n", argv[i]); return 2; }
    }
    std::puts("# Experimental body adapter; production bodies remain global. Motors/pools use their normal local resolver.");
    std::puts("kind,case,result,pos1_m,pos60_m,pos600_m,lin1_mps,lin60_mps,lin600_mps,ang1_radps,ang60_radps,ang600_radps,contact_mismatch_steps,event_mismatch_steps,sleep_mismatch_steps,ref_tunnel,exp_tunnel,transfer_tick,change_wake_calls,cross_scope_pair_steps");
    bool passed = true;
    try {
        const std::array cases{
            Spec{"world-zero"}, Spec{"effective-zero", {0, 10}, {0, 0}},
            Spec{"lateral", {0, 10}, {4, 0}}, Spec{"gravity-scale", {0, 0}, {0, 10}, 0.5},
            Spec{.name="mass-change", .massChange=true}, Spec{.name="gravity-scale-change", .scaleChange=true},
            Spec{.name="resting-contact", .floor=true},
            Spec{.name="ccd", .floor=true, .bullet=true}, Spec{.name="same-gravity-cross-scene-contact", .floor=true, .secondBody=true},
            Spec{.name="lateral-ccd", .base={0, 10}, .effective={4, 0}, .floor=true, .bullet=true, .wall=true},
            Spec{.name="different-collinear-cross-scene-contact", .floor=true, .secondBody=true, .differentGravity=true},
            Spec{.name="deferred-runtime-transfer", .floor=true, .transfer=true},
            Spec{.name="sleep-unchanged", .floor=true, .sleep=true},
            Spec{.name="sleeping-runtime-transfer", .floor=true, .sleepingTransfer=true}
        };
        for (const auto& spec : cases) passed &= BodyComparison(spec);
        if (changingWorldBase) {
            const Spec spec{.name="override-stable-changing-world-base", .floor=true, .sleep=true, .changeWorldBase=true};
            passed &= BodyComparison(spec);
        }
        passed &= MixedDirectionObservation();
        passed &= ConsumerBoundary(false, true);
        passed &= ConsumerBoundary(true, false);
        passed &= ConsumerBoundary(true, true);
        for (const auto* name : {"light-machine", "light-lab", "physics-lab", "coupling-lab"})
            passed &= SceneGlobalBoundary(name);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "runtime qualification error: %s\n", error.what());
        return 2;
    }
    std::printf("runtime experimental criteria: %s; no activation approval inferred\n", passed ? "PASS" : "FAIL");
    return passed ? 0 : 1;
}
