#pragma once

#include "pixie/runtime/Entity.hpp"

#include <pixie/pxPoint.hpp>

namespace pixie {
class World;
}

inline constexpr int kMaxAttractors = 16;
inline constexpr int kMaxSpawners = 16;

// Builds the particles World in code: the particle step and one particle system entity,
// "particles", whose pool has an AttractorAffector and an InteractionAffector, and a spawner,
// "spawner", that feeds it from a ring on the left. Returns the system's entity.
pixie::Entity SetUpParticles(pixie::World& world);

// An attractor entity at `position`: Transform2D, Attractor, and a ring Shape.
pixie::Entity AddAttractor(pixie::World& world, pxPoint position);
int AttractorCount(pixie::World& world);

// A spawner entity at `position` feeding `system`: Transform2D, a ParticleSpawner like the scene's
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
