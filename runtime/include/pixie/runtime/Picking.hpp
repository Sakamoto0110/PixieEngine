#pragma once

#include <entt/entity/entity.hpp>

#include <pixie/pxPoint.hpp>
#include <pixie/pxRect.hpp>

#include <vector>

namespace pixie {

class World;
struct Shape;
struct Transform2D;

// Finding things by where they are, for a tool's selection. Only entities with a Transform2D and a
// Shape can be picked: what is drawn can be clicked. Particles are not entities; RemoveParticleAt
// reaches them instead. Positions are the current ones, in world units.

// The radius an entity covers: its Shape's larger half extent times the transform's larger scale.
double PickRadius(const Shape& shape, const Transform2D& transform);

// The entity whose pick radius, plus `tolerance`, covers `point`, nearest center first; entt::null
// when none does.
entt::entity PickAt(World& world, pxPoint point, double tolerance);

// The entities whose center lies inside `box`, oldest first.
std::vector<entt::entity> PickIn(World& world, const pxRect& box);

// Removes the particle nearest `point` within `radius`, from whichever ParticleSystem holds it, at
// once. Returns whether one was removed.
bool RemoveParticleAt(World& world, pxPoint point, double radius);

} // namespace pixie
