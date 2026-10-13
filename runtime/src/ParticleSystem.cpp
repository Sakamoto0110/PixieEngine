#include "pixie/runtime/ParticleSystem.hpp"

#include "pixie/runtime/ParticleEmitter.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

#include <cmath>

namespace pixie {

namespace {

void Age(ParticleEmitter& emitter, float dt) {
    auto& particles = emitter.particles;
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
void RemoveExpired(ParticleEmitter& emitter) {
    auto& particles = emitter.particles;
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

void EmitAtRate(World& world, Entity entity, ParticleEmitter& emitter, double dt) {
    if (emitter.rate <= 0.0) {
        return;
    }
    emitter.pending += emitter.rate * dt;
    const Transform2D* transform = entity.TryGet<Transform2D>();
    const pxPoint origin = transform != nullptr ? transform->position : pxPoint(0.0, 0.0);
    const double heading = transform != nullptr ? transform->rotation : 0.0;
    Random& random = world.GetRandom();
    while (emitter.pending >= 1.0) {
        emitter.pending -= 1.0;
        const double angle = heading + random.Range(-0.5, 0.5) * emitter.spread;
        const double speed = random.Range(emitter.speedMin, emitter.speedMax);
        const pxVec2f velocity(static_cast<float>(std::cos(angle) * speed),
                               static_cast<float>(std::sin(angle) * speed));
        if (!emitter.Emit(pxPointf(static_cast<float>(origin.x), static_cast<float>(origin.y)),
                          velocity, emitter.lifetime)) {
            emitter.pending = 0.0;
            break;
        }
    }
}

void StepParticles(World& world, const FrameTime& time, const Input&) {
    const auto dt = static_cast<float>(time.delta);
    for (auto [id, emitter] : world.Registry().view<ParticleEmitter>().each()) {
        const Entity entity = world.Wrap(id);
        Age(emitter, dt);
        EmitAtRate(world, entity, emitter, time.delta);
        for (const auto& affector : emitter.affectors) {
            if (affector->IsEnabled()) {
                affector->Apply(world, entity, emitter.particles, time);
            }
        }
        RemoveExpired(emitter);
        for (Particle& particle : emitter.particles) {
            particle.position = particle.position + particle.velocity * dt;
        }
    }
}

} // namespace

void AddParticleSystem(World& world) {
    world.AddSystem(SystemPhase::PostUpdate, "particles.step", &StepParticles);
}

void DrawParticles(World& world, ParticleRenderer& renderer, const Camera2D& camera, double alpha) {
    for (auto [id, emitter] : world.Registry().view<const ParticleEmitter>().each()) {
        renderer.Draw(emitter.particles, emitter.style, alpha, camera);
    }
}

} // namespace pixie
