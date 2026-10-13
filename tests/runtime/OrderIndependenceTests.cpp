#include "Check.hpp"

#include "pixie/runtime/Acceleration2D.hpp"
#include "pixie/runtime/SpatialIndex.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/Velocity2D.hpp"
#include "pixie/runtime/World.hpp"

#include <vector>

using namespace pixie;

namespace {

struct Member {};

constexpr double kRadius = 40.0;

// Flock-like steering from the neighbours' positions and velocities. `writeVelocity` chooses how the
// result is applied: through its own Acceleration2D, as the runtime asks, or straight into its own
// Velocity2D, which later Behaviours of the same step then read.
template<bool writeVelocity>
struct Steer final : Behaviour {
    Entity flock;
    explicit Steer(Entity f) : flock(f) {}

    void Update(const FrameTime& time, const Input&) override {
        World& world = GetWorld();
        const pxPoint self = Self().Get<Transform2D>().position;
        pxVec2 towardOthers(0.0);
        pxVec2 heading(0.0);
        flock.Get<SpatialGrid>().ForEachWithin(self, kRadius, [&](entt::entity id, pxPoint p, double) {
            if (id == Self().Id()) {
                return;
            }
            towardOthers += p - self;
            heading += world.Registry().get<Velocity2D>(id).linear;
        });
        const pxVec2 steer = towardOthers * 0.5 + heading * 0.1;
        if constexpr (writeVelocity) {
            Self().Get<Velocity2D>().linear += steer * time.delta;
        } else {
            Self().Get<Acceleration2D>().linear = steer;
        }
    }
};

template<bool writeVelocity>
void Populate(World& world) {
    Entity flock = world.Create("flock");
    AddSpatialIndex<Member>(world, flock, kRadius);
    Random& random = world.GetRandom();
    for (int i = 0; i < 400; ++i) {
        Entity e = world.Create();
        e.Add<Transform2D>(Transform2D{pxPoint(random.Range(0.0, 300.0), random.Range(0.0, 300.0))});
        e.Add<Velocity2D>(Velocity2D{random.Direction() * 20.0});
        e.Add<Acceleration2D>();
        e.Add<Member>();
        e.AddBehaviour<Steer<writeVelocity>>(flock);
    }
}

std::vector<pxPoint> Positions(World& world) {
    std::vector<pxPoint> positions;
    // Entity creation order is the same in both Worlds, so sorting by entity lines them up.
    world.Registry().sort<Transform2D>([](entt::entity a, entt::entity b) { return a < b; });
    for (auto [id, transform] : world.Registry().view<const Transform2D>().each()) {
        positions.push_back(transform.position);
    }
    return positions;
}

// Steps two identical Worlds, one with its Behaviours' update order shuffled, and says whether
// they ended up bit for bit the same.
template<bool writeVelocity>
bool SameAfterShuffling() {
    const Input input;
    World ordered(5);
    World shuffled(5);
    Populate<writeVelocity>(ordered);
    Populate<writeVelocity>(shuffled);
    Random scramble(99);
    std::vector<std::uint64_t> keys(1024);
    for (auto& key : keys) {
        key = scramble.NextU64();
    }
    shuffled.Registry().sort<Steer<writeVelocity>>([&](entt::entity a, entt::entity b) {
        return keys[entt::to_entity(a) % keys.size()] < keys[entt::to_entity(b) % keys.size()];
    });
    for (int step = 0; step < 30; ++step) {
        ordered.Step(1.0 / 60.0, input);
        shuffled.Step(1.0 / 60.0, input);
    }
    return Positions(ordered) == Positions(shuffled);
}

} // namespace

TEST(StepResultDoesNotDependOnBehaviourOrder) {
    CHECK(SameAfterShuffling<false>());
}

TEST(WritingSharedStateDuringUpdateDoesDependOnOrder) {
    // The negative control: the shuffle above would catch a Behaviour that breaks the rule.
    CHECK(!SameAfterShuffling<true>());
}
