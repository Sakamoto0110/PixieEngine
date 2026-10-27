#include "pixie/runtime/ParticleStep.hpp"

#include "pixie/runtime/AttractorAffector.hpp"
#include "pixie/runtime/InteractionAffector.hpp"
#include "pixie/runtime/ParticleSpawner.hpp"
#include "pixie/runtime/ParticleSystem.hpp"
#include "pixie/runtime/ParticlePhysics2D.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

#include <cmath>
#include <stdexcept>
#include <unordered_map>

namespace pixie {

namespace {

// Marks a World whose particle step is installed.
struct ParticleStepInstalled {};
struct ParticlePhysicsStatus { std::vector<std::string> errors; };
struct ParticlePhysicsMetrics {
    MetricHandle time, dynamic, queries, contacts, expired, exhausted;
    explicit ParticlePhysicsMetrics(Telemetry& t)
        : time(t.RegisterTiming("particles.physics")),
          dynamic(t.RegisterGauge("particles.physics.dynamic", "particles")),
          queries(t.RegisterGauge("particles.physics.queries", "queries")),
          contacts(t.RegisterGauge("particles.physics.contacts", "contacts")),
          expired(t.RegisterGauge("particles.physics.expired", "particles")),
          exhausted(t.RegisterGauge("particles.physics.exhausted", "particles")) {}
};

// The working state (grids, buffers) of the affectors that scene components describe, by system
// entity, kept in the registry's context so it is never saved.
struct SceneAffectors {
    std::unordered_map<entt::entity, AttractorAffector> attraction;
    std::unordered_map<entt::entity, InteractionAffector> interaction;
};

SceneAffectors& SceneAffectorsOf(entt::registry& registry) {
    auto& context = registry.ctx();
    if (!context.contains<SceneAffectors>()) {
        context.emplace<SceneAffectors>();
    }
    return context.get<SceneAffectors>();
}

// The affectors a system's entity describes: its attraction, then its interaction.
void ApplySceneAffectors(World& world, Entity entity, ParticleSystem& system, const FrameTime& time) {
    entt::registry& registry = world.Registry();
    SceneAffectors& state = SceneAffectorsOf(registry);
    const entt::entity id = entity.Id();
    if (const auto* attraction = registry.try_get<ParticleAttraction>(id)) {
        if (attraction->enabled) {
            AttractorAffector& affector = state.attraction[id];
            affector.scale = attraction->scale;
            affector.Apply(world, entity, system.particles, time);
        }
    } else {
        state.attraction.erase(id);
    }
    if (const auto* interaction = registry.try_get<ParticleInteraction>(id)) {
        if (interaction->enabled) {
            InteractionAffector& affector = state.interaction[id];
            affector.radius = interaction->radius;
            affector.stiffness = interaction->stiffness;
            affector.damping = interaction->damping;
            affector.Apply(world, entity, system.particles, time);
        }
    } else {
        state.interaction.erase(id);
    }
}

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

void Ballistic(ParticleSystem& system, float dt) {
    for (Particle& particle : system.particles) {
        const float lived = (particle.flags & Particle::kDynamic) != 0 ? std::min(dt, particle.age) : dt;
        particle.position = particle.position + particle.velocity * lived;
    }
}
void StepParticles(World& world, const FrameTime& time, const Input&) {
    const auto dt = static_cast<float>(time.delta);
    ParticlePhysicsStats2D stats;
    auto& errors = world.Registry().ctx().get<ParticlePhysicsStatus>().errors; errors.clear();
    auto& telemetry = world.GetTelemetry();
    const auto& metrics = world.Registry().ctx().get<ParticlePhysicsMetrics>();
    for (auto [id, system] : world.Registry().view<ParticleSystem>().each()) {
        const Entity entity = world.Wrap(id);
        Age(system, dt);
        Spawn(world, id, system, time.delta);
        for (const auto& affector : system.affectors) {
            if (affector->IsEnabled()) {
                affector->Apply(world, entity, system.particles, time);
            }
        }
        ApplySceneAffectors(world, entity, system, time);
        RemoveExpired(system);
        if (const auto* physics = world.Registry().try_get<ParticlePhysics2D>(id)) {
            const TelemetryScope measure(telemetry, metrics.time);
            const auto* collision = SceneCollisionWorld2D(world);
            pxVec2 gravity = PhysicsSettings2D{}.gravity;
            const auto settings = world.Registry().view<const PhysicsSettings2D>();
            if (settings.begin() != settings.end()) gravity = settings.get<const PhysicsSettings2D>(*settings.begin()).gravity;
            if (collision) gravity = collision->Gravity();
            try {
                MoveDynamicParticles2D(system, *physics, collision, gravity, time.delta, stats);
            } catch (const std::invalid_argument& error) {
                errors.push_back("ParticlePhysics2D entity " + std::to_string(static_cast<std::uint32_t>(id)) + ": " + error.what());
                Ballistic(system, dt);
            }
        } else {
            Ballistic(system, dt);
        }
    }
    telemetry.SetGauge(metrics.dynamic, static_cast<double>(stats.dynamic));
    telemetry.SetGauge(metrics.queries, static_cast<double>(stats.queries));
    telemetry.SetGauge(metrics.contacts, static_cast<double>(stats.contacts));
    telemetry.SetGauge(metrics.expired, static_cast<double>(stats.expired));
    telemetry.SetGauge(metrics.exhausted, static_cast<double>(stats.exhausted));
}

} // namespace

void AddParticleStep(World& world) {
    auto& context = world.Registry().ctx();
    if (context.contains<ParticleStepInstalled>()) {
        return;
    }
    context.emplace<ParticlePhysicsMetrics>(world.GetTelemetry());
    context.emplace<ParticleStepInstalled>();
    context.emplace<ParticlePhysicsStatus>();
    world.AddSystem(SystemPhase::PostUpdate, "particles.step", &StepParticles);
}

std::vector<std::string> ParticlePhysicsErrors2D(World& world) {
    const auto* status = world.Registry().ctx().find<ParticlePhysicsStatus>();
    return status ? status->errors : std::vector<std::string>{};
}

void DrawParticles(World& world, ParticleRenderer& renderer, const Camera2D& camera, double alpha) {
    for (auto [id, system] : world.Registry().view<const ParticleSystem>().each()) {
        renderer.Draw(system.particles, system.style, alpha, camera);
    }
}

} // namespace pixie
