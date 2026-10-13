#pragma once

#include "pixie/Camera2D.hpp"
#include "pixie/ParticleRenderer.hpp"

namespace pixie {

class World;

// Adds the PostUpdate system that updates every ParticleSystem's pool once per step, after the
// Behaviours and the World's integration, so emitters and attractors are where this step left them:
//
// 1. each particle's previous position becomes its current one, and its age grows; particles
//    past their lifetime leave the pool (the last one takes the freed place);
// 2. every ParticleSpawner pointing at the system emits into it;
// 3. the system's enabled affectors run, in the order added, and the particles they killed leave;
// 4. each particle moves by its velocity.
void AddParticleStep(World& world);

// Draws every ParticleSystem's pool with its style, interpolated `alpha` of the way between steps.
void DrawParticles(World& world, ParticleRenderer& renderer, const Camera2D& camera, double alpha);

} // namespace pixie
