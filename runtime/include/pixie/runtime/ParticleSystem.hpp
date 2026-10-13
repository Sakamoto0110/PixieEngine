#pragma once

#include "pixie/Camera2D.hpp"
#include "pixie/ParticleRenderer.hpp"

namespace pixie {

class World;

// Adds the PostUpdate system that updates every ParticleEmitter's pool once per step, after the
// Behaviours and the World's integration, so emitters and attractors are where this step left them:
//
// 1. each particle's previous position becomes its current one, and its age grows; particles
//    past their lifetime leave the pool (the last one takes the freed place);
// 2. rate emission adds particles at the entity's Transform2D, within the spread;
// 3. the emitter's enabled affectors run, in the order added;
// 4. each particle moves by its velocity.
void AddParticleSystem(World& world);

// Draws every ParticleEmitter's pool with its style, interpolated `alpha` of the way between steps.
void DrawParticles(World& world, ParticleRenderer& renderer, const Camera2D& camera, double alpha);

} // namespace pixie
