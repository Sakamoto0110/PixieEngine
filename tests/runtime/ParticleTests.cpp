#include "Check.hpp"

#include "pixie/runtime/Attractor.hpp"
#include "pixie/runtime/AttractorAffector.hpp"
#include "pixie/runtime/InteractionAffector.hpp"
#include "pixie/runtime/ParticleEmitter.hpp"
#include "pixie/runtime/ParticleSystem.hpp"
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

TEST(RateEmissionAgingAndExpiry) {
    World world;
    AddParticleSystem(world);
    Entity e = world.Create();
    e.Add<Transform2D>(Transform2D{pxPoint(5.0, 5.0)});
    ParticleEmitter& emitter = e.Add<ParticleEmitter>();
    emitter.rate = 60.0;      // one per step
    emitter.lifetime = 0.25f; // 15 steps
    std::vector<std::size_t> seen;
    emitter.AddAffector<Probe>(&seen);
    for (int i = 0; i < 10; ++i) {
        world.Step(kStep, kNoInput);
    }
    CHECK(e.Get<ParticleEmitter>().particles.size() == 10);
    // Affectors see the pool after this step's emission.
    CHECK(seen.front() == 1 && seen.back() == 10);
    for (int i = 0; i < 30; ++i) {
        world.Step(kStep, kNoInput);
    }
    // A particle lives 15 steps: the pool settles at 15.
    const auto& particles = e.Get<ParticleEmitter>().particles;
    CHECK(particles.size() == 15);
    for (const Particle& p : particles) {
        CHECK(p.age < 0.25f + 1e-4f);
        CHECK(p.position == pxPointf(5.0f, 5.0f)); // emitted at the entity, with speed 0
    }
}

TEST(EmissionStopsAtCapacityAndDisabledAffectorsDoNotRun) {
    World world;
    AddParticleSystem(world);
    Entity e = world.Create();
    ParticleEmitter& emitter = e.Add<ParticleEmitter>();
    emitter.rate = 6000.0;
    emitter.capacity = 250;
    std::vector<std::size_t> seen;
    emitter.AddAffector<Probe>(&seen).SetEnabled(false);
    for (int i = 0; i < 5; ++i) {
        world.Step(kStep, kNoInput);
    }
    CHECK(e.Get<ParticleEmitter>().particles.size() == 250);
    CHECK(!e.Get<ParticleEmitter>().Emit(pxPointf(0.0f, 0.0f), pxVec2f(0.0f), 0.0f));
    CHECK(seen.empty());
}

TEST(CircularOrbitKeepsItsRadius) {
    World world;
    AddParticleSystem(world);
    Entity sun = world.Create();
    sun.Add<Transform2D>();
    Attractor& attractor = sun.Add<Attractor>();
    attractor.softening = 0.0;
    Entity e = world.Create();
    ParticleEmitter& emitter = e.Add<ParticleEmitter>();
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
        const double r = Length(e.Get<ParticleEmitter>().particles[0].position);
        nearest = std::min(nearest, r);
        farthest = std::max(farthest, r);
    }
    CHECK(nearest > radius * 0.97 && farthest < radius * 1.03);
}

TEST(InteractionPushesOverlappingParticlesApartWhateverTheirOrder) {
    const auto run = [](bool reversed) {
        World world;
        AddParticleSystem(world);
        Entity e = world.Create();
        ParticleEmitter& emitter = e.Add<ParticleEmitter>();
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
        for (const Particle& p : e.Get<ParticleEmitter>().particles) {
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
    AddParticleSystem(world);
    Entity attractor = world.Create();
    attractor.Add<Transform2D>(Transform2D{pxPoint(0.0, 0.0), 0.0, pxVec2(scale, scale)});
    Attractor& a = attractor.Add<Attractor>();
    a.strength = 0.0;
    a.size = size;
    a.killOnContact = killOnContact;
    Entity e = world.Create();
    ParticleEmitter& emitter = e.Add<ParticleEmitter>();
    emitter.AddAffector<AttractorAffector>();
    emitter.Emit(pxPointf(distance, 0.0f), pxVec2f(0.0f), 0.0f);
    world.Step(kStep, kNoInput);
    return e.Get<ParticleEmitter>().particles.size();
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
    AddParticleSystem(world);
    Entity sun = world.Create();
    sun.Add<Transform2D>();
    Attractor& a = sun.Add<Attractor>();
    a.size = 15.0;
    a.killOnContact = true;
    Entity e = world.Create();
    ParticleEmitter& emitter = e.Add<ParticleEmitter>();
    emitter.AddAffector<AttractorAffector>();
    emitter.Emit(pxPointf(100.0f, 0.0f), pxVec2f(0.0f), 0.0f);
    bool reached = false;
    for (int i = 0; i < 600 && !reached; ++i) {
        world.Step(kStep, kNoInput);
        const auto& particles = e.Get<ParticleEmitter>().particles;
        reached = particles.empty();
        if (!reached) {
            CHECK(Length(particles[0].position) >= 15.0 * 0.5);
        }
    }
    CHECK(reached);
}
