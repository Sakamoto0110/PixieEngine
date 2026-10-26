#include "Check.hpp"

#include "pixie/runtime/Attractor.hpp"
#include "pixie/runtime/AttractorAffector.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
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

TEST(AParticleIs40BytesWithItsSizeAsAHalfFloat) {
    CHECK(sizeof(Particle) == 40);
    CHECK(ToHalf(2.0f) == 0x4000 && ToHalf(-1.0f) == 0xBC00 && ToHalf(0.0f) == 0);
    for (const float value : {0.5f, 1.0f, 2.0f, 3.3f, 6.0f, 123.4f, 1000.0f, 65504.0f, 0.001f}) {
        CHECK(std::abs(FromHalf(ToHalf(value)) - value) <= value / 2048.0f);
    }
    // Ties round to even: 1 + 2^-11 lies halfway between 1 and the next half float.
    CHECK(ToHalf(1.0f + 1.0f / 2048.0f) == ToHalf(1.0f));
    CHECK(std::isinf(FromHalf(ToHalf(1.0e6f))));
    CHECK(FromHalf(ToHalf(1.0e-6f)) > 0.0f); // a subnormal half survives
    const Particle made = MakeParticle(pxPointf(1.0f, 2.0f), pxVec2f(3.0f, 4.0f), 5.0f, pxColorRgba(10, 20, 30, 40),
                                       2.5f, ParticleShape::Ring);
    CHECK(made.previous == made.position && made.age == 0.0f && made.lifetime == 5.0f);
    CHECK(made.color == pxColorRgba(10, 20, 30, 40) && made.Size() == 2.5f && made.Shape() == ParticleShape::Ring);
    CHECK((made.flags & Particle::kDynamic) == 0);
}

TEST(ParticlesTakeTheSystemsLookOrTheirSpawnersOwn) {
    World world;
    Fed fed = MakeFed(world, pxPoint(0.0, 0.0));
    ParticleSystem& system = fed.system.Get<ParticleSystem>();
    system.style.color = pxColorRgba(255, 0, 0);
    system.style.size = 3.0f;
    system.style.shape = ParticleShape::Ring;
    fed.spawner.Get<ParticleSpawner>().rate = 60.0;
    // A second spawner, 100 units away, with a look of its own, feeding the same pool.
    Entity other = world.Create();
    other.Add<Transform2D>(Transform2D{pxPoint(100.0, 0.0)});
    ParticleSpawner& own = other.Add<ParticleSpawner>();
    own.system = fed.system.Id();
    own.rate = 60.0;
    own.ownLook = true;
    own.particleColor = pxColorRgba(0, 255, 0);
    own.particleSize = 5.0;
    own.particleShape = ParticleShape::Streak;
    for (int i = 0; i < 3; ++i) {
        world.Step(kStep, kNoInput);
    }
    int systems = 0;
    int owns = 0;
    for (const Particle& p : fed.system.Get<ParticleSystem>().particles) {
        if (p.position.x == 0.0f) {
            systems += (p.color == pxColorRgba(255, 0, 0) && p.Size() == 3.0f && p.Shape() == ParticleShape::Ring) ? 1 : 0;
        } else {
            owns += (p.color == pxColorRgba(0, 255, 0) && p.Size() == 5.0f && p.Shape() == ParticleShape::Streak) ? 1 : 0;
        }
    }
    CHECK(systems == 3 && owns == 3);
    // A particle keeps the look it was born with.
    fed.system.Get<ParticleSystem>().style.color = pxColorRgba(0, 0, 255);
    world.Step(kStep, kNoInput);
    int red = 0;
    for (const Particle& p : fed.system.Get<ParticleSystem>().particles) {
        red += p.color == pxColorRgba(255, 0, 0) ? 1 : 0;
    }
    CHECK(red == 3);
}

TEST(AStepsParticlesAreSpacedAlongTheirPath) {
    World world;
    Fed fed = MakeFed(world, pxPoint(0.0, 0.0));
    ParticleSpawner& spawner = fed.spawner.Get<ParticleSpawner>();
    spawner.rate = 600.0; // ten per step
    spawner.speedMin = spawner.speedMax = 60.0; // one unit per step
    spawner.spread = 0.0;
    world.Step(kStep, kNoInput);
    const auto& particles = fed.system.Get<ParticleSystem>().particles;
    CHECK(particles.size() == 10);
    // The k-th came due at (k + 1) / 10 of the step: it ends 0.9, 0.8, ... 0 units out, all born at
    // the spawner, each as old as the part of the step it lived.
    std::vector<float> distances;
    for (const Particle& p : particles) {
        CHECK(p.previous == pxPointf(0.0f, 0.0f));
        CHECK(std::abs(p.position.y) < 1e-6f);
        CHECK(std::abs(p.age * 60.0f - p.position.x) < 1e-4f);
        distances.push_back(p.position.x);
    }
    std::sort(distances.begin(), distances.end());
    for (std::size_t k = 0; k < distances.size(); ++k) {
        CHECK(std::abs(distances[k] - 0.1f * static_cast<float>(k)) < 1e-4f);
    }
}

TEST(JitterScattersStartsAroundTheShapesPoint) {
    const std::vector<Particle> particles = Emitted(4000, 0.0, [](World&, ParticleSpawner& s) { s.jitter = 10.0; });
    CHECK(particles.size() == 4000);
    std::size_t within = 0;
    double total = 0.0;
    for (const Particle& p : particles) {
        const double distance = Distance(p.position, 50.0, 50.0);
        within += distance <= 10.0 ? 1 : 0;
        total += distance;
    }
    // A Gaussian of standard deviation 5 per axis: 86.5% within 10, a mean distance of 6.27.
    const double share = static_cast<double>(within) / static_cast<double>(particles.size());
    const double mean = total / static_cast<double>(particles.size());
    CHECK(share > 0.84 && share < 0.89);
    CHECK(mean > 6.0 && mean < 6.55);
}

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

namespace {

// A system entity with one particle at rest at `at`, with the particle step installed.
Entity SystemWithParticleAt(World& world, pxPointf at) {
    AddParticleStep(world);
    Entity system = world.Create("system");
    system.Add<ParticleSystem>().Emit(at, pxVec2f(0.0f), 0.0f);
    return system;
}

// The x velocity of the first particle after one step, with an attractor at the origin and the
// system's ParticleAttraction set by `setUp`.
template<typename SetUp>
float PullAfterOneStep(SetUp&& setUp) {
    World world;
    Entity system = SystemWithParticleAt(world, pxPointf(100.0f, 0.0f));
    setUp(system.Add<ParticleAttraction>());
    Entity attractor = world.Create("attractor");
    attractor.Add<Transform2D>();
    attractor.Add<Attractor>();
    world.Step(kStep, kNoInput);
    return system.Get<ParticleSystem>().particles.front().velocity.x;
}

} // namespace

TEST(SceneAttractionPullsTowardTheAttractorsWithItsScale) {
    const float pulled = PullAfterOneStep([](ParticleAttraction&) {});
    CHECK(pulled < 0.0f);
    CHECK(PullAfterOneStep([](ParticleAttraction& a) { a.enabled = false; }) == 0.0f);
    CHECK(PullAfterOneStep([](ParticleAttraction& a) { a.scale = -1.0; }) == -pulled);
    CHECK(std::abs(PullAfterOneStep([](ParticleAttraction& a) { a.scale = 2.0; }) - 2.0f * pulled) < 1e-4f * std::abs(pulled));
    // Without the component, nothing pulls: attractors act only on systems that ask for them.
    World world;
    Entity system = SystemWithParticleAt(world, pxPointf(100.0f, 0.0f));
    Entity attractor = world.Create();
    attractor.Add<Transform2D>();
    attractor.Add<Attractor>();
    world.Step(kStep, kNoInput);
    CHECK(system.Get<ParticleSystem>().particles.front().velocity.x == 0.0f);
}

TEST(SceneInteractionPushesOverlappingParticlesApart) {
    for (const bool enabled : {true, false}) {
        World world;
        Entity system = SystemWithParticleAt(world, pxPointf(0.0f, 0.0f));
        system.Get<ParticleSystem>().Emit(pxPointf(1.0f, 0.0f), pxVec2f(0.0f), 0.0f);
        ParticleInteraction& interaction = system.Add<ParticleInteraction>();
        interaction.enabled = enabled;
        interaction.radius = 4.0;
        world.Step(kStep, kNoInput);
        const auto& particles = system.Get<ParticleSystem>().particles;
        const float left = std::min(particles[0].velocity.x, particles[1].velocity.x);
        const float right = std::max(particles[0].velocity.x, particles[1].velocity.x);
        CHECK(enabled ? (left < 0.0f && right > 0.0f) : (left == 0.0f && right == 0.0f));
    }
}

TEST(SceneAffectorsAreAddedOnlyToAParticleSystem) {
    ComponentTypes types;
    DescribeRuntimeComponents(types);
    World world;
    Entity bare = world.Create();
    Entity system = world.Create();
    system.Add<ParticleSystem>();
    for (const char* name : {"ParticleAttraction", "ParticleInteraction"}) {
        const int index = types.IndexOf(name);
        CHECK(index >= 0);
        if (index < 0) {
            continue;
        }
        const ComponentDescriptor& type = types.All()[static_cast<std::size_t>(index)];
        CHECK(!type.AdditionError(world, bare.Id()).empty());
        CHECK(type.AdditionError(world, system.Id()).empty());
    }
}

TEST(JitterKeepsADenseInteractingStreamFromExploding) {
    // 3000 particles a second from one point, pushing each other apart: the mean speed after 1.5 s.
    const auto meanSpeed = [](double jitter) {
        World world;
        Fed fed = MakeFed(world, pxPoint(0.0, 0.0));
        ParticleSpawner& spawner = fed.spawner.Get<ParticleSpawner>();
        spawner.rate = 3000.0;
        spawner.lifetime = 1.5f;
        spawner.speedMin = 40.0;
        spawner.speedMax = 80.0;
        spawner.jitter = jitter;
        fed.system.Add<ParticleInteraction>();
        for (int i = 0; i < 90; ++i) {
            world.Step(kStep, kNoInput);
        }
        double sum = 0.0;
        const auto& particles = fed.system.Get<ParticleSystem>().particles;
        for (const Particle& p : particles) {
            sum += std::hypot(p.velocity.x, p.velocity.y);
        }
        return particles.empty() ? 0.0 : sum / static_cast<double>(particles.size());
    };
    const double stacked = meanSpeed(0.0);
    const double scattered = meanSpeed(4.0);
    // Measured: about 540 without jitter, 200 with it; the rest is the pile's own pressure.
    CHECK(scattered < 0.5 * stacked);
    CHECK(scattered < 4.0 * 80.0);
}
