#include "pixie/runtime/ParticleStep.hpp"

#include "pixie/runtime/ParticleSpawner.hpp"
#include "pixie/runtime/ParticleSystem.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

#include <cmath>

namespace pixie {

namespace {

void Age(ParticleSystem& system, float dt) {
    auto& particles = system.particles;
    for (std::size_t i = 0; i < particles.size();) {
        Particle& particle = particles[i];
        particle.previous = particle.position;
        particle.age += dt;
        if (particle.lifetime > 0.0f && particle.age >= particle.lifetime) {
            particle = particles.back();
            particles.pop_back();
            continue; // the particle moved into this place has not been aged yet
        }
        ++i;
    }
}

// Removes the particles an affector killed during this step (KillParticle).
void RemoveExpired(ParticleSystem& system) {
    auto& particles = system.particles;
    for (std::size_t i = 0; i < particles.size();) {
        const Particle& particle = particles[i];
        if (particle.lifetime > 0.0f && particle.age >= particle.lifetime) {
            particles[i] = particles.back();
            particles.pop_back();
            continue;
        }
        ++i;
    }
}

// Every spawner that feeds `id` emits into it, in storage order.
void Spawn(World& world, entt::entity id, ParticleSystem& system, double delta) {
    entt::registry& registry = world.Registry();
    for (auto [spawnerId, spawner, transform] : registry.view<ParticleSpawner, const Transform2D>().each()) {
        if (spawner.system != id) {
            continue;
        }
        const Transform2D* target = registry.valid(spawner.target) ? registry.try_get<Transform2D>(spawner.target)
                                                                    : nullptr;
        EmitFromSpawner(spawner, transform, target, system, world.GetRandom(), delta);
    }
}

void StepParticles(World& world, const FrameTime& time, const Input&) {
    const auto dt = static_cast<float>(time.delta);
    for (auto [id, system] : world.Registry().view<ParticleSystem>().each()) {
        const Entity entity = world.Wrap(id);
        Age(system, dt);
        Spawn(world, id, system, time.delta);
        for (const auto& affector : system.affectors) {
            if (affector->IsEnabled()) {
                affector->Apply(world, entity, system.particles, time);
            }
        }
        RemoveExpired(system);
        for (Particle& particle : system.particles) {
            particle.position = particle.position + particle.velocity * dt;
        }
    }
}

} // namespace

void AddParticleStep(World& world) {
    world.AddSystem(SystemPhase::PostUpdate, "particles.step", &StepParticles);
}

void DrawParticles(World& world, ParticleRenderer& renderer, const Camera2D& camera, double alpha) {
    for (auto [id, system] : world.Registry().view<const ParticleSystem>().each()) {
        renderer.Draw(system.particles, system.style, alpha, camera);
    }
}

} // namespace pixie
