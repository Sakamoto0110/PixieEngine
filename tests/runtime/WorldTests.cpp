#include "Check.hpp"

#include "pixie/runtime/Name.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/Velocity2D.hpp"
#include "pixie/runtime/World.hpp"

#include <algorithm>
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
