#pragma once

#include "pixie/runtime/World.hpp"
#include "pixie/runtime/ParticleSystem.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include <cmath>
#include <numbers>

// Call once from Project::Build for each newly constructed World.
// Normal ProjectSession hosts install scene movement and the particle step.
// This demonstration emits at entity A's center, not at a measured contact point.
inline void InstallContactBursts(pixie::World& world) {
    const auto poolEntity = world.Create("contact feedback");
    auto& pool = poolEntity.Add<pixie::ParticleSystem>();
    pool.capacity = 512;
    pool.style.color = {255, 190, 80};
    pool.style.size = 3.0f;
    world.AddSystem(pixie::SystemPhase::PostUpdate, "Contact feedback",
        [poolEntity](pixie::World& current, const pixie::FrameTime&, const pixie::Input&) {
            if (!poolEntity.IsAlive()) return;
            auto* particles = poolEntity.TryGet<pixie::ParticleSystem>();
            if (!particles) return;
            for (const auto& contact : pixie::ScenePhysicsEvents2D(current)) {
                if (contact.event.kind != pixie::PhysicsEventKind2D::Begin
                    || contact.event.pair != pixie::PhysicsPairKind2D::Contact) continue;
                const auto entity = current.Wrap(contact.entityA);
                if (!entity.IsAlive()) continue;
                const auto* transform = entity.TryGet<pixie::Transform2D>();
                if (!transform) continue;
                const auto position = transform->position;
                for (int i = 0; i < 12; ++i) {
                    const float angle = 2.0f * std::numbers::pi_v<float> * float(i) / 12.0f;
                    if (!particles->Emit({float(position.x), float(position.y)},
                        {40.0f * std::cos(angle), 40.0f * std::sin(angle)}, 0.4f)) break;
                }
            }
        });
}
