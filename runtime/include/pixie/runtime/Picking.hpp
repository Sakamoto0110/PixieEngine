#pragma once

#include <entt/entity/entity.hpp>

#include <pixie/pxPoint.hpp>
#include <pixie/pxRect.hpp>

#include <vector>

namespace pixie {

class SpriteImages;
class World;
struct Shape;
struct Transform2D;

// Finding things by where they are, for a tool's selection. Only entities with a Transform2D and a
// Shape or a Sprite can be picked: what is drawn can be clicked. Particles are not entities;
// RemoveParticleAt reaches them instead. Positions are the current ones, in world units.

// The radius an entity covers: its Shape's larger half extent times the transform's larger scale.
double PickRadius(const Shape& shape, const Transform2D& transform);

// The entity under `point`, widened by `tolerance`: a shape whose pick radius covers it, nearest
// center first; else, with `sprites` to give their sizes, the sprite whose rectangle covers it and
// is drawn on top (shapes are drawn over sprites). entt::null when there is none.
entt::entity PickAt(World& world, pxPoint point, double tolerance, SpriteImages* sprites = nullptr);

// The entities with a shape or a sprite whose position lies inside `box`, oldest first.
std::vector<entt::entity> PickIn(World& world, const pxRect& box);

// Removes the particle nearest `point` within `radius`, from whichever ParticleSystem holds it, at
// once. Returns whether one was removed.
bool RemoveParticleAt(World& world, pxPoint point, double radius);

} // namespace pixie
