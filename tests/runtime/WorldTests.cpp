#include "Check.hpp"

#include "pixie/runtime/Name.hpp"
#include "pixie/runtime/RigidBody2D.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/Velocity2D.hpp"
#include "pixie/runtime/World.hpp"

#include <algorithm>
#include <limits>
#include <string>
#include <stdexcept>
#include <vector>

using namespace pixie;

namespace {

std::vector<std::string> journal;

struct Logger final : Behaviour {
    std::string tag;
    explicit Logger(std::string t) : tag(std::move(t)) {}
    void Start() override { journal.push_back(tag + ".start"); }
    void Update(const FrameTime&, const Input&) override { journal.push_back(tag + ".update"); }
    void Stop() override { journal.push_back(tag + ".stop"); }
};

// Destroys a target entity from its Update, and records whether the target was still alive.
struct Destroyer final : Behaviour {
    Entity target;
    bool sawTargetAliveAfterDestroy = false;
    explicit Destroyer(Entity t) : target(t) {}
    void Update(const FrameTime&, const Input&) override {
        if (target.IsAlive()) {
            target.Destroy();
            sawTargetAliveAfterDestroy = target.IsAlive();
        }
    }
};

struct Counter {
    int value = 0;
};

// Reads its own Counter through Self().
struct Incrementer final : Behaviour {
    void Update(const FrameTime&, const Input&) override { ++Self().Get<Counter>().value; }
};

const Input kNoInput;

} // namespace

TEST(StepRunsPreUpdateThenBehavioursThenPostUpdate) {
    journal.clear();
    World world;
    world.AddSystem(SystemPhase::PostUpdate, "post", [](World&, const FrameTime&, const Input&) {
        journal.push_back("post");
    });
    world.AddSystem(SystemPhase::PreUpdate, "pre", [](World&, const FrameTime&, const Input&) {
        journal.push_back("pre");
    });
    world.Create().AddBehaviour<Logger>("a");
    world.Step(1.0 / 60.0, kNoInput);
    CHECK((journal == std::vector<std::string>{"pre", "a.start", "a.update", "post"}));
}

TEST(BehaviourStartsOnceStopsOnDestroyAndWorldEnd) {
    journal.clear();
    {
        World world;
        Entity a = world.Create();
        a.AddBehaviour<Logger>("a");
        world.Create().AddBehaviour<Logger>("b");
        world.Step(0.1, kNoInput);
        world.Step(0.1, kNoInput);
        a.Destroy();
        CHECK(!a.IsAlive());
        journal.push_back("end");
    }
    // b's Stop comes from the World ending; a's from its destruction. Updates within one class run
    // in storage order, so only the relative order per behaviour is checked.
    const auto count = [](const std::string& entry) {
        return std::count(journal.begin(), journal.end(), entry);
    };
    CHECK(count("a.start") == 1 && count("b.start") == 1);
    CHECK(count("a.update") == 2 && count("b.update") == 2);
    CHECK(count("a.stop") == 1 && count("b.stop") == 1);
    const auto at = [](const std::string& entry) {
        return std::find(journal.begin(), journal.end(), entry) - journal.begin();
    };
    CHECK(at("a.stop") < at("end") && at("end") < at("b.stop"));
}

TEST(BehaviourThatNeverStartedDoesNotStop) {
    journal.clear();
    {
        World world;
        world.Create().AddBehaviour<Logger>("idle");
    }
    CHECK(journal.empty());
}

TEST(RemovingABehaviourStopsIt) {
    journal.clear();
    World world;
    Entity e = world.Create();
    e.AddBehaviour<Logger>("a");
    world.Step(0.1, kNoInput);
    e.Remove<Logger>();
    CHECK(journal.back() == "a.stop");
    CHECK(e.IsAlive() && !e.Has<Logger>());
}

TEST(DestructionDuringAStepWaitsForItsEnd) {
    World world;
    Entity target = world.Create("target");
    Entity destroyer = world.Create();
    destroyer.AddBehaviour<Destroyer>(target);
    bool aliveInPostUpdate = false;
    world.AddSystem(SystemPhase::PostUpdate, "probe", [&](World&, const FrameTime&, const Input&) {
        aliveInPostUpdate = target.IsAlive();
    });
    world.Step(0.1, kNoInput);
    CHECK(destroyer.Get<Destroyer>().sawTargetAliveAfterDestroy);
    CHECK(aliveInPostUpdate);
    CHECK(!target.IsAlive());
    CHECK(world.EntityCount() == 1);
}

TEST(DeferredActionsRunAfterTheStep) {
    World world;
    bool ran = false;
    std::size_t entitiesSeen = 0;
    world.AddSystem(SystemPhase::PreUpdate, "spawner", [&](World& w, const FrameTime&, const Input&) {
        w.Defer([&](World& inner) {
            ran = true;
            CHECK(!inner.IsStepping());
            inner.Create("spawned");
            entitiesSeen = inner.EntityCount();
        });
        CHECK(!ran);
    });
    world.Step(0.1, kNoInput);
    CHECK(ran);
    CHECK(entitiesSeen == 1);
}

TEST(WorldKeepsItsOwnTime) {
    World world;
    std::vector<std::uint64_t> indices;
    std::vector<double> elapsed;
    world.AddSystem(SystemPhase::PreUpdate, "clock", [&](World&, const FrameTime& time, const Input&) {
        indices.push_back(time.index);
        elapsed.push_back(time.elapsed);
    });
    world.Step(0.5, kNoInput);
    world.Step(0.25, kNoInput);
    CHECK((indices == std::vector<std::uint64_t>{0, 1}));
    CHECK((elapsed == std::vector<double>{0.0, 0.5}));
    CHECK(world.Steps() == 2 && world.Elapsed() == 0.75);
}

TEST(VelocityIsIntegratedAfterBehavioursAndPreviousTransformKept) {
    World world;
    Entity e = world.Create();
    e.Add<Transform2D>(Transform2D{pxPoint(1.0, 2.0), 0.0});
    e.Add<Velocity2D>(Velocity2D{pxVec2(10.0, -4.0), 1.0});
    CHECK(e.Has<PreviousTransform2D>());
    world.Step(0.5, kNoInput);
    const Transform2D& now = e.Get<Transform2D>();
    const Transform2D& before = e.Get<PreviousTransform2D>().value;
    CHECK(before.position == pxPoint(1.0, 2.0));
    CHECK(now.position == pxPoint(6.0, 0.0));
    CHECK(now.rotation == 0.5);
    world.ResetInterpolation(e);
    CHECK(e.Get<PreviousTransform2D>().value.position == now.position);
}

TEST(EntityHandleReachesComponents) {
    World world;
    Entity e = world.Create("named");
    CHECK(e.Get<Name>().value == "named");
    CHECK(e.TryGet<Counter>() == nullptr);
    e.Add<Counter>().value = 3;
    CHECK(e.Has<Counter>() && e.Get<Counter>().value == 3);
    e.AddBehaviour<Incrementer>();
    world.Step(0.1, kNoInput);
    world.Step(0.1, kNoInput);
    CHECK(e.Get<Counter>().value == 5);
    e.Remove<Counter>();
    CHECK(!e.Has<Counter>());
    Entity copy = e;
    CHECK(copy == e && copy.IsAlive());
}


TEST(PhysicsPhasePrecedesPostUpdateRegardlessOfRegistrationOrder) {
    World world; std::vector<int> order;
    world.AddSystem(SystemPhase::PostUpdate, "early-post", [&](World&, const FrameTime&, const Input&) { order.push_back(2); });
    world.AddSystem(SystemPhase::Physics, "late-physics", [&](World&, const FrameTime&, const Input&) { order.push_back(1); });
    world.AddSystem(SystemPhase::PostUpdate, "late-post", [&](World&, const FrameTime&, const Input&) { order.push_back(3); });
    world.Step(1.0/60, {});
    CHECK((order == std::vector<int>{1, 2, 3}));
}
TEST(ThrownSystemRestoresSteppingAndAllowsRecoveryOutsideTheStep) {
    World world; bool fail = true;
    world.AddSystem(SystemPhase::Physics, "throw-once", [&](World&, const FrameTime&, const Input&) {
        if (fail) { fail = false; throw std::runtime_error("deliberate failure"); }
    });
    bool threw = false;
    try { world.Step(1.0/60, {}); } catch (const std::runtime_error&) { threw = true; }
    CHECK(threw && !world.IsStepping());
    bool immediate = false; world.Defer([&](World&) { immediate = true; }); CHECK(immediate);
    world.Step(1.0/60, {}); CHECK(!world.IsStepping());
}

TEST(LegacyVelocityIntegrationPrecedesNativePhysicsAndPostConsumers) {
    World world; auto entity = world.Create(); entity.Add<Transform2D>(); entity.Add<Velocity2D>().linear = {60,0};
    bool physicsSawMoved = false, postSawMoved = false;
    world.AddSystem(SystemPhase::PostUpdate, "consumer", [&](World&, const FrameTime&, const Input&) { postSawMoved = entity.Get<Transform2D>().position.x == 1; });
    world.AddSystem(SystemPhase::Physics, "native-observer", [&](World&, const FrameTime&, const Input&) { physicsSawMoved = entity.Get<Transform2D>().position.x == 1; });
    world.Step(1.0/60, {}); CHECK(physicsSawMoved && postSawMoved);
}

TEST(LogicalScopesKeepStableIdentityAndWorldOnlyMembership) {
    World world;
    const ScopeId roomA{11}, roomB{22};
    CHECK(!world.CreateScope(ScopeId{}, "reserved"));
    CHECK(world.CreateScope(roomA, "Room A"));
    CHECK(world.CreateScope(roomB, "Room B"));
    CHECK(!world.CreateScope(roomA, "duplicate"));
    Entity corridor = world.Create("corridor");
    Entity actor = world.Create("actor");
    CHECK(world.ScopeOf(corridor) == ScopeId{});
    CHECK(world.SetScope(actor, roomA));
    CHECK(world.RenameScope(roomA, "Renamed room"));
    CHECK(world.ScopeOf(actor) == roomA);
    const auto scopes = world.Scopes();
    CHECK(scopes.size() == 2);
    CHECK(scopes[0].id == roomA && scopes[0].name == "Renamed room");
    CHECK(scopes[1].id == roomB && scopes[1].name == "Room B");
    CHECK(!world.RemoveScope(roomA));
    CHECK(world.RemoveScope(roomB));
    CHECK(!world.SetScope(actor, roomB));
    CHECK(world.SetScope(actor, ScopeId{}));
    CHECK(world.RemoveScope(roomA));
    CHECK(world.ScopeOf(actor) == ScopeId{});
    CHECK(world.ScopeOf(corridor) == ScopeId{});
}

TEST(ScopeTransferDuringStepAppliesAfterPhysicsAndPreservesEntityState) {
    World world;
    const ScopeId roomA{101}, roomB{202};
    CHECK(world.CreateScope(roomA, "A") && world.CreateScope(roomB, "B"));
    Entity actor = world.Create();
    actor.Add<SceneId>(SceneId{9001});
    actor.Add<Transform2D>(Transform2D{pxPoint(3.0, 4.0), 0.0});
    actor.Add<Velocity2D>(Velocity2D{pxVec2(2.0, 0.0), 0.0});
    CHECK(world.SetScope(actor, roomA));
    const auto originalId = actor.Id();
    bool sawOldMembershipInPhysics = false;
    bool sawOldMembershipInPost = false;
    bool sawNewMembershipOnNextTick = false;
    world.AddSystem(SystemPhase::PreUpdate, "transfer", [&](World& w, const FrameTime&, const Input&) {
        if (w.Steps() == 0) {
            CHECK(w.SetScope(actor, roomB));
            CHECK(w.ScopeOf(actor) == roomA);
            CHECK(!w.RemoveScope(roomA));
        }
    });
    world.AddSystem(SystemPhase::Physics, "scope-probe", [&](World& w, const FrameTime&, const Input&) {
        if (w.Steps() == 0) sawOldMembershipInPhysics = w.ScopeOf(actor) == roomA;
        if (w.Steps() == 1) sawNewMembershipOnNextTick = w.ScopeOf(actor) == roomB;
    });
    world.AddSystem(SystemPhase::PostUpdate, "scope-probe-post", [&](World& w, const FrameTime&, const Input&) {
        if (w.Steps() == 0) sawOldMembershipInPost = w.ScopeOf(actor) == roomA;
    });
    world.Step(0.5, kNoInput);
    CHECK(sawOldMembershipInPhysics && sawOldMembershipInPost);
    CHECK(actor.IsAlive() && actor.Id() == originalId);
    CHECK(actor.Get<SceneId>().value == 9001);
    CHECK(world.ScopeOf(actor) == roomB);
    CHECK(actor.Get<Transform2D>().position == pxPoint(4.0, 4.0));
    CHECK(actor.Get<Velocity2D>().linear == pxVec2(2.0, 0.0));
    world.Step(0.5, kNoInput);
    CHECK(sawNewMembershipOnNextTick);
    CHECK(world.RemoveScope(roomA));
    CHECK(!world.RemoveScope(roomB));
    actor.Destroy();
    CHECK(world.RemoveScope(roomB));
    CHECK(!world.ScopeOf(actor).has_value());
}

TEST(ScopeRejectsForeignEntitiesAndDestroyedDeferredTransfers) {
    World world, other;
    const ScopeId room{7};
    CHECK(world.CreateScope(room, "room"));
    Entity foreign = other.Create();
    CHECK(!world.SetScope(foreign, room));
    CHECK(!world.ScopeOf(foreign).has_value());
    Entity actor = world.Create();
    world.AddSystem(SystemPhase::PreUpdate, "destroy-transferee", [&](World& w, const FrameTime&, const Input&) {
        CHECK(w.SetScope(actor, room));
        actor.Destroy();
    });
    world.Step(0.01, kNoInput);
    CHECK(!actor.IsAlive());
    CHECK(world.RemoveScope(room));
}

TEST(SceneGravityResolutionKeepsInactiveValuesAndExplicitZero) {
    World world;
    world.Create().Add<PhysicsSettings2D>().gravity = {0, 100};
    const ScopeId inherit{1}, multiply{2}, overrideScope{3};
    CHECK(world.CreateScope(inherit, "inherit"));
    CHECK(world.CreateScope(multiply, "multiply"));
    CHECK(world.CreateScope(overrideScope, "override"));
    const SceneGravity2D authored{SceneGravityMode2D::Multiply, {30, 40}, 0.75};
    CHECK(world.SetScopeGravity(multiply, authored));
    CHECK(world.SetScopeGravity(overrideScope, {SceneGravityMode2D::Override, {30, 40}, 0.75}));
    Entity actor = world.Create();
    CHECK(world.ResolveGravity(actor) && world.ResolveGravity(actor).value == pxVec2(0, 100));
    CHECK(world.ResolveGravity(inherit).value == pxVec2(0, 100));
    CHECK(world.ResolveGravity(multiply).value == pxVec2(0, 75));
    CHECK(world.ResolveGravity(overrideScope).value == pxVec2(30, 40));
    CHECK(world.SetScope(actor, multiply));
    CHECK(world.ResolveGravity(actor).value == pxVec2(0, 75));
    world.Registry().view<PhysicsSettings2D>().each([](auto, auto& settings) { settings.gravity = {0, 200}; });
    CHECK(world.ResolveGravity(multiply).value == pxVec2(0, 150));
    CHECK(world.ResolveGravity(overrideScope).value == pxVec2(30, 40));
    CHECK(world.SetScopeGravity(multiply, {SceneGravityMode2D::Multiply, {30, 40}, 0}));
    CHECK(world.ResolveGravity(multiply).value == pxVec2(0, 0));
    CHECK(world.SetScopeGravity(multiply, {SceneGravityMode2D::Inherit, {30, 40}, 0}));
    CHECK((world.Scopes()[1].gravity == std::optional<SceneGravity2D>{SceneGravity2D{SceneGravityMode2D::Inherit, {30, 40}, 0}}));
}

TEST(SceneGravityRejectsInvalidEditsWithoutChangingAuthoredState) {
    World world;
    const ScopeId room{9};
    CHECK(world.CreateScope(room, "room"));
    const SceneGravity2D original{SceneGravityMode2D::Override, {3, 4}, 0.5};
    CHECK(world.SetScopeGravity(room, original));
    const auto unchanged = [&] { CHECK(world.Scopes()[0].gravity == std::optional<SceneGravity2D>{original}); };
    CHECK(!world.SetScopeGravity(room, {SceneGravityMode2D::Multiply, {3, 4}, -1})); unchanged();
    CHECK(!world.SetScopeGravity(room, {SceneGravityMode2D::Inherit, {std::numeric_limits<double>::quiet_NaN(), 4}, 1})); unchanged();
    CHECK(!world.SetScopeGravity(room, {static_cast<SceneGravityMode2D>(99), {3, 4}, 1})); unchanged();
    CHECK(!world.SetScopeGravity(room, {SceneGravityMode2D::Multiply, {3, 4}, 1e12})); unchanged();
    CHECK(!world.ResolveGravity(ScopeId{999}));
    world.Create().Add<PhysicsSettings2D>();
    world.Create().Add<PhysicsSettings2D>();
    CHECK(!world.SetScopeGravity(room, {SceneGravityMode2D::Inherit, {}, 1})); unchanged();
}

TEST(ProspectiveWorldPhysicsSettingsValidateEveryRetainedScopeWithoutMutation) {
    World world;
    const ScopeId room{1};
    CHECK(world.CreateScope(room, "room", SceneGravity2D{SceneGravityMode2D::Multiply, {30, 40}, 1000}));
    PhysicsSettings2D candidate;
    CHECK(world.ValidatePhysicsSettings(candidate).empty());
    candidate.gravity = {0, 1000000};
    CHECK(!world.ValidatePhysicsSettings(candidate).empty());
    candidate = {};
    candidate.unitsPerMeter = 0;
    CHECK(!world.ValidatePhysicsSettings(candidate).empty());
    candidate = {};
    candidate.substeps = 33;
    CHECK(!world.ValidatePhysicsSettings(candidate).empty());
    CHECK(world.EntityCount() == 0 && world.Scopes().size() == 1 && world.Steps() == 0);
    CHECK(world.ResolveGravity(room).value == pxVec2(0, 1100000));
}
