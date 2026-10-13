#include "pixie/runtime/Acceleration2D.hpp"
#include "pixie/runtime/Attractor.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/ParticleEmitter.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/Velocity2D.hpp"

namespace pixie {

void DescribeRuntimeComponents(ComponentTypes& types) {
    types.Add<Transform2D>("Transform2D")
        .Field<&Transform2D::position>("position")
        .Tooltip("Where the entity is, in world units; Y points down.")
        .Scrub(1.0)
        .Field<&Transform2D::rotation>("rotation")
        .Tooltip("Turn from +X toward +Y, clockwise on screen, in degrees.")
        .Degrees()
        .Range(-360.0, 360.0)
        .Scrub(1.0)
        .Field<&Transform2D::scale>("scale")
        .Tooltip("Multiplies the shape's size on each axis.")
        .Range(0.0, 100.0, 0.01)
        .Scrub(0.01);
    types.Add<Velocity2D>("Velocity2D")
        .Field<&Velocity2D::linear>("linear")
        .Tooltip("How fast the entity moves, in world units per second. The World adds it to the position "
                 "after every Behaviour has run.")
        .Scrub(1.0)
        .Field<&Velocity2D::angular>("angular")
        .Tooltip("How fast the entity turns, in degrees per second.")
        .Degrees("°/s")
        .Scrub(1.0);
    types.Add<Acceleration2D>("Acceleration2D")
        .Field<&Acceleration2D::linear>("linear")
        .Tooltip("How fast the velocity changes, in world units per second squared. A steering Behaviour "
                 "rewrites it every step, so an edit there lasts one step.")
        .Scrub(1.0);
    types.Add<Shape>("Shape")
        .Field<&Shape::kind>("kind", {"Triangle", "Circle", "Ring", "Quad"})
        .Tooltip("Which shape the renderer draws.")
        .Field<&Shape::size>("size")
        .Tooltip("Half width and half height, in world units, before the transform's scale.")
        .Range(0.0, 10000.0)
        .Scrub(0.5)
        .Field<&Shape::color>("color")
        .Tooltip("Red, green, blue, and alpha.");
    types.Add<Attractor>("Attractor")
        .Field<&Attractor::strength>("strength")
        .Tooltip("How hard it pulls, in world units cubed per second squared; negative pushes away. A "
                 "circular orbit at distance r has speed sqrt(strength / r).")
        .Scrub(10000.0)
        .Field<&Attractor::softening>("softening")
        .Tooltip("Distance below which the pull stops growing, in world units.")
        .Range(0.0, 500.0)
        .Scrub(0.5)
        .Field<&Attractor::radius>("radius")
        .Tooltip("Particles farther than this, in world units, feel nothing; 0 means no limit.")
        .Range(0.0, 100000.0)
        .Scrub(1.0);
    types.Add<ParticleEmitter>("ParticleEmitter")
        .Field<&ParticleEmitter::rate>("rate")
        .Tooltip("Particles emitted per second at the entity; 0 emits nothing.")
        .Range(0.0, 100000.0)
        .Scrub(1.0)
        .Field<&ParticleEmitter::lifetime>("lifetime")
        .Tooltip("How long an emitted particle lives, in seconds; 0 means forever.")
        .Unit(" s")
        .Range(0.0, 600.0)
        .Scrub(0.05)
        .Field<&ParticleEmitter::speedMin>("speedMin")
        .Label("speed min")
        .Tooltip("Slowest emission speed, in world units per second.")
        .Range(0.0, 10000.0)
        .Scrub(1.0)
        .Field<&ParticleEmitter::speedMax>("speedMax")
        .Label("speed max")
        .Tooltip("Fastest emission speed, in world units per second.")
        .Range(0.0, 10000.0)
        .Scrub(1.0)
        .Field<&ParticleEmitter::spread>("spread")
        .Tooltip("Width of the emission cone around the entity's rotation, in degrees.")
        .Degrees()
        .Range(0.0, 360.0)
        .Scrub(1.0);
}

} // namespace pixie
