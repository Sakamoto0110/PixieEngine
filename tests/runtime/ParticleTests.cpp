#include "Check.hpp"

#include "pixie/runtime/Attractor.hpp"
#include "pixie/runtime/AttractorAffector.hpp"
#include "pixie/runtime/InteractionAffector.hpp"
#include "pixie/runtime/ParticleSpawner.hpp"
#include "pixie/runtime/ParticleSystem.hpp"
#include "pixie/runtime/ParticleStep.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

using namespace pixie;

namespace {

const Input kNoInput;
constexpr double kStep = 1.0 / 60.0;

// Records the pool size it sees, and whether it ran.
struct Probe final : ParticleAffector {
    std::vector<std::size_t>* seen;
    explicit Probe(std::vector<std::size_t>* s) : seen(s) {}
    void Apply(World&, Entity, std::span<Particle> particles, const FrameTime&) override {
        seen->push_back(particles.size());
    }
};

double Length(pxPointf p) {
    return std::sqrt(static_cast<double>(p.x) * p.x + static_cast<double>(p.y) * p.y);
}

} // namespace

// A particle system entity, and a spawner entity at `at` feeding it.
struct Fed {
    Entity system;
    Entity spawner;
};

Fed MakeFed(World& world, pxPoint at) {
    AddParticleStep(world);
    Fed fed{world.Create("system"), world.Create("spawner")};
    fed.system.Add<ParticleSystem>();
    fed.spawner.Add<Transform2D>(Transform2D{at});
    fed.spawner.Add<ParticleSpawner>().system = fed.system.Id();
    return fed;
}

TEST(RateEmissionAgingAndExpiry) {
    World world;
    Fed fed = MakeFed(world, pxPoint(5.0, 5.0));
    ParticleSpawner& spawner = fed.spawner.Get<ParticleSpawner>();
    spawner.rate = 60.0;      // one per step
    spawner.lifetime = 0.25f; // 15 steps
    std::vector<std::size_t> seen;
    fed.system.Get<ParticleSystem>().AddAffector<Probe>(&seen);
    for (int i = 0; i < 10; ++i) {
        world.Step(kStep, kNoInput);
    }
    CHECK(fed.system.Get<ParticleSystem>().particles.size() == 10);
    // Affectors see the pool after this step's emission.
    CHECK(seen.front() == 1 && seen.back() == 10);
    for (int i = 0; i < 30; ++i) {
        world.Step(kStep, kNoInput);
    }
    // A particle lives 15 steps: the pool settles at 15.
    const auto& particles = fed.system.Get<ParticleSystem>().particles;
    CHECK(particles.size() == 15);
    for (const Particle& p : particles) {
        CHECK(p.age < 0.25f + 1e-4f);
        CHECK(p.position == pxPointf(5.0f, 5.0f)); // emitted at the spawner, with speed 0
    }
}

TEST(EmissionStopsAtCapacityAndDisabledAffectorsDoNotRun) {
    World world;
    Fed fed = MakeFed(world, pxPoint(0.0, 0.0));
    fed.spawner.Get<ParticleSpawner>().rate = 6000.0;
    ParticleSystem& system = fed.system.Get<ParticleSystem>();
    system.capacity = 250;
    std::vector<std::size_t> seen;
    system.AddAffector<Probe>(&seen).SetEnabled(false);
    for (int i = 0; i < 5; ++i) {
        world.Step(kStep, kNoInput);
    }
    CHECK(fed.system.Get<ParticleSystem>().particles.size() == 250);
    CHECK(!fed.system.Get<ParticleSystem>().Emit(pxPointf(0.0f, 0.0f), pxVec2f(0.0f), 0.0f));
    CHECK(seen.empty());
}

TEST(TwoSpawnersFeedOnePool) {
    World world;
    Fed fed = MakeFed(world, pxPoint(0.0, 0.0));
    fed.spawner.Get<ParticleSpawner>().rate = 60.0;
    Entity second = world.Create();
    second.Add<Transform2D>(Transform2D{pxPoint(100.0, 0.0)});
    ParticleSpawner& other = second.Add<ParticleSpawner>();
    other.system = fed.system.Id();
    other.rate = 120.0;
    Entity unattached = world.Create();
    unattached.Add<Transform2D>();
    unattached.Add<ParticleSpawner>().rate = 600.0; // feeds no system
    for (int i = 0; i < 10; ++i) {
        world.Step(kStep, kNoInput);
    }
    const auto& particles = fed.system.Get<ParticleSystem>().particles;
    CHECK(particles.size() == 30);
    const auto fromSecond = std::count_if(particles.begin(), particles.end(),
                                          [](const Particle& p) { return p.position.x == 100.0f; });
    CHECK(fromSecond == 20);
}

namespace {

// One step of a spawner configured by `setUp`, emitting `count` particles at speed 10 with no spread,
// from (50, 50) rotated by `rotation`; returns them.
template<typename SetUp>
std::vector<Particle> Emitted(int count, double rotation, SetUp&& setUp) {
    World world;
    Fed fed = MakeFed(world, pxPoint(50.0, 50.0));
    fed.spawner.Get<Transform2D>().rotation = rotation;
    ParticleSpawner& s = fed.spawner.Get<ParticleSpawner>();
    s.rate = count / kStep;
    s.speedMin = s.speedMax = 10.0;
    s.spread = 0.0;
    setUp(world, s);
    world.Step(kStep, kNoInput);
    // Each particle moved one step after emission; take it back to where it started.
    std::vector<Particle> particles = fed.system.Get<ParticleSystem>().particles;
    for (Particle& p : particles) {
        p.position = p.previous;
    }
    return particles;
}

double Distance(pxPointf p, double x, double y) {
    return std::hypot(p.x - x, p.y - y);
}

double Angle(pxVec2f v) {
    return std::atan2(static_cast<double>(v.y), static_cast<double>(v.x));
}

bool Near(double a, double b, double tolerance = 1e-4) {
    return std::abs(std::remainder(a - b, 2.0 * 3.14159265358979)) <= tolerance;
}

} // namespace

TEST(EmissionShapesPlaceParticlesOnTheShape) {
    for (const Particle& p : Emitted(50, 0.0, [](World&, ParticleSpawner&) {})) {
        CHECK(Distance(p.position, 50.0, 50.0) < 1e-4);
    }
    for (const Particle& p : Emitted(200, 0.0, [](World&, ParticleSpawner& s) {
             s.shape = EmissionShape::Circle;
             s.radius = 20.0;
         })) {
        CHECK(Distance(p.position, 50.0, 50.0) <= 20.0 + 1e-3);
    }
    for (const Particle& p : Emitted(200, 0.0, [](World&, ParticleSpawner& s) {
             s.shape = EmissionShape::Ring;
             s.radius = 20.0;
         })) {
        CHECK(std::abs(Distance(p.position, 50.0, 50.0) - 20.0) < 1e-3);
    }
    // A 30 x 10 rectangle (half extents), turned 90 degrees: 10 wide and 30 tall on screen.
    for (const Particle& p : Emitted(200, 3.14159265358979 / 2.0, [](World&, ParticleSpawner& s) {
             s.shape = EmissionShape::Rectangle;
             s.size = pxVec2(30.0, 10.0);
         })) {
        CHECK(std::abs(p.position.x - 50.0f) <= 10.0f + 1e-3f && std::abs(p.position.y - 50.0f) <= 30.0f + 1e-3f);
    }
    // A segment from the spawner to (+40, 0), turned 90 degrees: from (50, 50) to (50, 90).
    for (const Particle& p : Emitted(200, 3.14159265358979 / 2.0, [](World&, ParticleSpawner& s) {
             s.shape = EmissionShape::Segment;
             s.segmentEnd = pxVec2(40.0, 0.0);
         })) {
        CHECK(std::abs(p.position.x - 50.0f) < 1e-3f && p.position.y >= 50.0f - 1e-3f && p.position.y <= 90.0f + 1e-3f);
    }
}

TEST(DirectionModesAimTheParticles) {
    // Fixed: the spawner's rotation plus the direction.
    for (const Particle& p : Emitted(20, 0.5, [](World&, ParticleSpawner& s) { s.direction = 0.25; })) {
        CHECK(Near(Angle(p.velocity), 0.75));
    }
    // Shape normal on a ring: straight outward.
    for (const Particle& p : Emitted(100, 0.0, [](World&, ParticleSpawner& s) {
             s.shape = EmissionShape::Ring;
             s.radius = 20.0;
             s.directionMode = DirectionMode::ShapeNormal;
         })) {
        CHECK(Near(Angle(p.velocity), std::atan2(p.position.y - 50.0, p.position.x - 50.0), 1e-3));
    }
    // Shape normal on a segment along +X: the left normal, +90 degrees.
    for (const Particle& p : Emitted(20, 0.0, [](World&, ParticleSpawner& s) {
             s.shape = EmissionShape::Segment;
             s.segmentEnd = pxVec2(40.0, 0.0);
             s.directionMode = DirectionMode::ShapeNormal;
         })) {
        CHECK(Near(Angle(p.velocity), 3.14159265358979 / 2.0));
    }
    // Toward a target at (50, -50): straight up on screen.
    for (const Particle& p : Emitted(20, 1.0, [](World& world, ParticleSpawner& s) {
             Entity target = world.Create();
             target.Add<Transform2D>(Transform2D{pxPoint(50.0, -50.0)});
             s.directionMode = DirectionMode::TowardTarget;
             s.target = target.Id();
         })) {
        CHECK(Near(Angle(p.velocity), -3.14159265358979 / 2.0));
    }
    // Spread: every angle within half the spread of the direction.
    for (const Particle& p : Emitted(300, 0.0, [](World&, ParticleSpawner& s) { s.spread = 1.0; })) {
        CHECK(std::abs(Angle(p.velocity)) <= 0.5 + 1e-4);
    }
}

TEST(CircularOrbitKeepsItsRadius) {
    World world;
    AddParticleStep(world);
    Entity sun = world.Create();
    sun.Add<Transform2D>();
    Attractor& attractor = sun.Add<Attractor>();
    attractor.softening = 0.0;
    Entity e = world.Create();
    ParticleSystem& emitter = e.Add<ParticleSystem>();
    emitter.AddAffector<AttractorAffector>();
    const double radius = 200.0;
    const double speed = std::sqrt(attractor.strength / radius);
    emitter.Emit(pxPointf(static_cast<float>(radius), 0.0f), pxVec2f(0.0f, static_cast<float>(speed)), 0.0f);
    // One full orbit and a bit.
    const int steps = static_cast<int>(2.0 * 3.14159265 * radius / speed / kStep) + 10;
    double nearest = radius;
    double farthest = radius;
    for (int i = 0; i < steps; ++i) {
        world.Step(kStep, kNoInput);
        const double r = Length(e.Get<ParticleSystem>().particles[0].position);
        nearest = std::min(nearest, r);
        farthest = std::max(farthest, r);
    }
    CHECK(nearest > radius * 0.97 && farthest < radius * 1.03);
}

TEST(InteractionPushesOverlappingParticlesApartWhateverTheirOrder) {
    const auto run = [](bool reversed) {
        World world;
        AddParticleStep(world);
        Entity e = world.Create();
        ParticleSystem& emitter = e.Add<ParticleSystem>();
        auto& interaction = emitter.AddAffector<InteractionAffector>();
        interaction.radius = 10.0;
        std::vector<pxPointf> starts{{0.0f, 0.0f}, {4.0f, 0.0f}, {2.0f, 3.0f}};
        if (reversed) {
            std::reverse(starts.begin(), starts.end());
        }
        for (const pxPointf p : starts) {
            emitter.Emit(p, pxVec2f(0.0f), 0.0f);
        }
        for (int i = 0; i < 20; ++i) {
            world.Step(kStep, kNoInput);
        }
        std::vector<pxPointf> ends;
        for (const Particle& p : e.Get<ParticleSystem>().particles) {
            ends.push_back(p.position);
        }
        if (reversed) {
            std::reverse(ends.begin(), ends.end());
        }
        return ends;
    };
    const auto forward = run(false);
    const auto backward = run(true);
    CHECK(forward == backward);
    // The first two started 4 apart; they end farther apart.
    const float dx = forward[1].x - forward[0].x;
    const float dy = forward[1].y - forward[0].y;
    CHECK(std::sqrt(dx * dx + dy * dy) > 4.0f);
}

namespace {

// A World with an attractor of body `size` at the origin, scaled by `scale`, that does not pull,
// and one particle at rest at `distance` from it; returns how many particles are left after a step.
std::size_t ParticlesLeft(bool killOnContact, double size, double scale, float distance) {
    World world;
    AddParticleStep(world);
    Entity attractor = world.Create();
    attractor.Add<Transform2D>(Transform2D{pxPoint(0.0, 0.0), 0.0, pxVec2(scale, scale)});
    Attractor& a = attractor.Add<Attractor>();
    a.strength = 0.0;
    a.size = size;
    a.killOnContact = killOnContact;
    Entity e = world.Create();
    ParticleSystem& emitter = e.Add<ParticleSystem>();
    emitter.AddAffector<AttractorAffector>();
    emitter.Emit(pxPointf(distance, 0.0f), pxVec2f(0.0f), 0.0f);
    world.Step(kStep, kNoInput);
    return e.Get<ParticleSystem>().particles.size();
}

} // namespace

TEST(ContactKillsOnlyWithTheFlag) {
    CHECK(ParticlesLeft(true, 20.0, 1.0, 10.0f) == 0);
    CHECK(ParticlesLeft(false, 20.0, 1.0, 10.0f) == 1);
    CHECK(ParticlesLeft(true, 20.0, 1.0, 30.0f) == 1);
}

TEST(TheBodyGrowsWithTheTransformScale) {
    CHECK(ParticlesLeft(true, 20.0, 1.0, 30.0f) == 1);
    CHECK(ParticlesLeft(true, 20.0, 2.0, 30.0f) == 0);
}

TEST(AParticleFlyingIntoTheBodyIsRemovedThatStep) {
    // Pulled straight in from 100 units, the particle reaches a 15-unit body and is removed, before it
    // could be drawn inside it.
    World world;
    AddParticleStep(world);
    Entity sun = world.Create();
    sun.Add<Transform2D>();
    Attractor& a = sun.Add<Attractor>();
    a.size = 15.0;
    a.killOnContact = true;
    Entity e = world.Create();
    ParticleSystem& emitter = e.Add<ParticleSystem>();
    emitter.AddAffector<AttractorAffector>();
    emitter.Emit(pxPointf(100.0f, 0.0f), pxVec2f(0.0f), 0.0f);
    bool reached = false;
    for (int i = 0; i < 600 && !reached; ++i) {
        world.Step(kStep, kNoInput);
        const auto& particles = e.Get<ParticleSystem>().particles;
        reached = particles.empty();
        if (!reached) {
            CHECK(Length(particles[0].position) >= 15.0 * 0.5);
        }
    }
    CHECK(reached);
}
