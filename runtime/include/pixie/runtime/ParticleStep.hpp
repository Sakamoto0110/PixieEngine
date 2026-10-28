#pragma once

#include "pixie/runtime/Export.hpp"

#include "pixie/render/2d/Camera2D.hpp"
#include "pixie/render/2d/ParticleRenderer.hpp"

#include <string>
#include <vector>

namespace pixie {

class World;

// Adds the PostUpdate system that updates every ParticleSystem's pool once per step, after the
// Behaviours and the World's integration, so emitters and attractors are where this step left them:
//
// 1. each particle's previous position becomes its current one, and its age grows; particles
//    past their lifetime leave the pool (the last one takes the freed place);
// 2. every ParticleSpawner pointing at the system emits into it;
// 3. the system's enabled affectors run, in the order added, then the ParticleAttraction and
//    ParticleInteraction on its entity, and the particles they killed leave;
// 4. each particle moves by its velocity.
//
// It installs once per World: later calls do nothing. Every World a ProjectSession builds gets it,
// after the project's Build, so a scene's particle systems run without project code, and a project
// that adds it itself, earlier in its own order of systems, keeps that place.
PIXIE_RUNTIME_API void AddParticleStep(World& world);

// Per-pool errors from the last particle tick; invalid settings fall back to ballistic motion.
PIXIE_RUNTIME_API std::vector<std::string> ParticlePhysicsErrors2D(World& world);

// Draws every ParticleSystem's pool with its style, interpolated `alpha` of the way between steps.
PIXIE_RUNTIME_API void DrawParticles(World& world, ParticleRenderer& renderer, const Camera2D& camera, double alpha);

} // namespace pixie
