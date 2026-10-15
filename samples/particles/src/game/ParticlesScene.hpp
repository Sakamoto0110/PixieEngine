#pragma once

#include "pixie/runtime/Entity.hpp"

#include <pixie/pxPoint.hpp>

namespace pixie {
class World;
}

inline constexpr int kMaxAttractors = 16;
inline constexpr int kMaxSpawners = 16;

// Builds what the scene does not hold: the particle step, and the pool of `system` (the
// "particles" entity, with a ParticleSystem) with its look, an AttractorAffector, and an
// InteractionAffector. With `defaults`, for a World built without a scene, it also gives the pool
// its capacity and adds a spawner, "spawner", that feeds it from a ring on the left.
void SetUpParticles(pixie::World& world, pixie::Entity system, bool defaults);

// An attractor entity of the scene at `position`: Transform2D, Attractor, and a ring Shape.
pixie::Entity AddAttractor(pixie::World& world, pxPoint position);
int AttractorCount(pixie::World& world);

// A spawner entity of the scene at `position` feeding `system`: Transform2D, a ParticleSpawner like the scene's
// (a ring of radius 16, 8-second lifetime, speed 60 to 110, outward, spread 0.6) with a rate of 0,
// and a ring Shape.
pixie::Entity AddSpawner(pixie::World& world, pixie::Entity system, pxPoint position);
int SpawnerCount(pixie::World& world);

// Replaces the pool with `count` particles in roughly circular orbits, spread over the attractors
// in turn (around the origin if there is none). Call it outside a step.
void SpawnOrbits(pixie::World& world, pixie::Entity emitter, int count);

// Turns the particle-to-particle interaction on or off.
void SetInteraction(pixie::Entity emitter, bool on);
bool InteractionOn(pixie::Entity emitter);
