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
// Shape, a Sprite, or a Path can be picked: what is drawn can be clicked. Particles are not entities;
// RemoveParticleAt reaches them instead. Positions are the current ones, in world units.

// The radius an entity covers: its Shape's larger half extent times the transform's larger scale.
double PickRadius(const Shape& shape, const Transform2D& transform);

// The entity under `point`, widened by `tolerance`, the one drawn on top first: a shape whose pick
// radius covers it, nearest center first; else, with `sprites` to give their sizes, a sprite of a
// layer of 0 or more whose rectangle covers it; else a path within half its width of it; else a
// sprite of a negative layer. entt::null when there is none.
entt::entity PickAt(World& world, pxPoint point, double tolerance, SpriteImages* sprites = nullptr);

// The entities with a shape or a sprite whose position lies inside `box`, and those with a path
// whose whole line does, oldest first.
std::vector<entt::entity> PickIn(World& world, const pxRect& box);

// Removes the particle nearest `point` within `radius`, from whichever ParticleSystem holds it, at
// once. Returns whether one was removed.
bool RemoveParticleAt(World& world, pxPoint point, double radius);

} // namespace pixie
