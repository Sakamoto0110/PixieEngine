#include "pixie/runtime/Acceleration2D.hpp"
#include "pixie/runtime/Attractor.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/ParticleSpawner.hpp"
#include "pixie/runtime/ParticleSystem.hpp"
#include "pixie/runtime/Path.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Sprite.hpp"
#include "pixie/runtime/Text.hpp"
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
    types.Add<Sprite>("Sprite")
        .Field<&Sprite::image>("image")
        .Asset("png")
        .Tooltip("The PNG drawn, relative to the project folder. One image pixel is one world unit, times "
                 "the transform's scale.")
        .Field<&Sprite::region>("region")
        .Tooltip("The part of the image drawn, in pixels from its top-left: x, y, width, height. Zero width "
                 "or height draws the whole image.")
        .Scrub(1.0)
        .Field<&Sprite::pivot>("pivot")
        .Tooltip("Where the position falls in the sprite, and what it turns around: (0, 0) is its top-left, "
                 "(1, 1) its bottom-right, (0.5, 0.5) its center.")
        .Range(0.0, 1.0)
        .Scrub(0.01)
        .Field<&Sprite::color>("color")
        .Tooltip("Multiplies the image: red, green, blue, and alpha. White draws it as it is.")
        .Field<&Sprite::layer>("layer")
        .Tooltip("Drawing order: higher layers are drawn on top; within a layer, older entities first. "
                 "Sprites are drawn under particles and shapes.")
        .Scrub(0.1)
        .Field<&Sprite::filter>("filter", {"Smooth", "Pixel"})
        .Tooltip("Smooth blends texels, and stays clean when drawn small; Pixel keeps pixel art crisp.");
    types.Add<Text>("Text")
        .Field<&Text::text>("text")
        .Tooltip("What it says; a line break starts a new line.")
        .Field<&Text::font>("font")
        .Asset("ttf")
        .Tooltip("A TrueType font (.ttf), relative to the project folder. Without a readable one, nothing is drawn.")
        .Field<&Text::size>("size")
        .Tooltip("The em size, in world units, before the transform's scale.")
        .Range(0.0, 10000.0)
        .Scrub(0.5)
        .Field<&Text::color>("color")
        .Tooltip("Red, green, blue, and alpha.")
        .Field<&Text::align>("align", {"Left", "Center", "Right"})
        .Tooltip("How the lines line up within the widest.")
        .Field<&Text::pivot>("pivot")
        .Tooltip("Where the position falls on the block of lines, and what it turns around: (0, 0) is its "
                 "top-left, (1, 1) its bottom-right, (0.5, 0.5) its center.")
        .Range(0.0, 1.0)
        .Scrub(0.01)
        .Field<&Text::layer>("layer")
        .Tooltip("Drawing order among sprites: higher layers on top; within a layer, older entities first. "
                 "A negative layer lies under paths.")
        .Scrub(0.1);
    types.Add<Path>("Path")
        .Field<&Path::points>("points")
        .Tooltip("The line's points, in the entity's space: moved, turned, and scaled with its transform.")
        .Field<&Path::width>("width")
        .Tooltip("Width of the line, in world units; never thinner than a pixel on screen.")
        .Range(0.0, 1000.0)
        .Scrub(0.25)
        .Field<&Path::cornerRadius>("cornerRadius")
        .Label("corner radius")
        .Tooltip("Radius of the arc that rounds each inner corner, in world units; less where the segments "
                 "beside it are too short. 0 keeps sharp corners.")
        .Range(0.0, 1000.0)
        .Scrub(0.5)
        .Field<&Path::color>("color")
        .Tooltip("The line's color after the fill.")
        .Field<&Path::fillColor>("fillColor")
        .Label("fill color")
        .Tooltip("The line's color from its first point up to the fill.")
        .Field<&Path::fill>("fill")
        .Tooltip("How much of the line, from its first point, is drawn in the fill color: 0 none, 1 all.")
        .Range(0.0, 1.0, 0.01)
        .Scrub(0.005);
    types.Add<Attractor>("Attractor")
        .Field<&Attractor::strength>("strength")
        .Tooltip("How hard it pulls, in world units cubed per second squared; negative pushes away. A "
                 "circular orbit at distance r has speed sqrt(strength / r).")
        .Scrub(10000.0)
        .Field<&Attractor::softening>("softening")
        .Tooltip("Distance below which the pull stops growing, in world units.")
        .Range(0.0, 500.0)
        .Scrub(0.5)
        .Field<&Attractor::size>("size")
        .Tooltip("Radius of the body, in world units, before the transform's scale. It does not change "
                 "the pull; with die on contact, particles that come within it are removed.")
        .Range(0.0, 10000.0)
        .Scrub(0.5)
        .Field<&Attractor::reach>("reach")
        .Tooltip("Particles farther than this, in world units, feel nothing; 0 means no limit.")
        .Range(0.0, 100000.0)
        .Scrub(1.0)
        .Field<&Attractor::killOnContact>("killOnContact")
        .Label("die on contact")
        .Tooltip("Remove a particle that comes within the body. Off, particles pass through it.");
    types.Add<ParticleSystem>("ParticleSystem")
        .Field<&ParticleSystem::capacity>("capacity")
        .Tooltip("Most particles the pool holds; spawners stop emitting while it is full.")
        .Range(0.0, 1000000.0, 1.0)
        .Scrub(10.0);
    types.Add<ParticleSpawner>("ParticleSpawner")
        .Field<&ParticleSpawner::system>("system")
        .References("ParticleSystem")
        .Tooltip("The particle system this spawner feeds.")
        .Field<&ParticleSpawner::rate>("rate")
        .Tooltip("Particles emitted per second; 0 emits nothing.")
        .Range(0.0, 100000.0)
        .Scrub(1.0)
        .Field<&ParticleSpawner::lifetime>("lifetime")
        .Tooltip("How long an emitted particle lives, in seconds; 0 means forever.")
        .Unit(" s")
        .Range(0.0, 600.0)
        .Scrub(0.05)
        .Field<&ParticleSpawner::speedMin>("speedMin")
        .Label("speed min")
        .Tooltip("Slowest emission speed, in world units per second.")
        .Range(0.0, 10000.0)
        .Scrub(1.0)
        .Field<&ParticleSpawner::speedMax>("speedMax")
        .Label("speed max")
        .Tooltip("Fastest emission speed, in world units per second.")
        .Range(0.0, 10000.0)
        .Scrub(1.0)
        .Field<&ParticleSpawner::shape>("shape", {"Point", "Circle", "Ring", "Rectangle", "Segment"})
        .Tooltip("Where particles start: the spawner's point, inside a disc, on a circle, inside a "
                 "rectangle, or on a segment.")
        .Field<&ParticleSpawner::radius>("radius")
        .Tooltip("Radius of the circle or the ring, in world units.")
        .Range(0.0, 10000.0)
        .Scrub(0.5)
        .Field<&ParticleSpawner::size>("size")
        .Tooltip("Half width and half height of the rectangle, in world units.")
        .Range(0.0, 10000.0)
        .Scrub(0.5)
        .Field<&ParticleSpawner::segmentEnd>("segmentEnd")
        .Label("segment end")
        .Tooltip("Where the segment ends, as an offset from the spawner, in world units.")
        .Scrub(0.5)
        .Field<&ParticleSpawner::directionMode>("directionMode", {"Fixed", "ShapeNormal", "TowardTarget"})
        .Label("direction mode")
        .Tooltip("What the direction is measured from: the spawner's rotation, the shape's outward "
                 "normal where the particle starts, or the target.")
        .Field<&ParticleSpawner::direction>("direction")
        .Tooltip("Center of the emission cone, in degrees, added to the direction mode's angle. Drag "
                 "the dial to turn it; Shift + drag to open or close the cone.")
        .Degrees()
        .Range(-360.0, 360.0)
        .Scrub(1.0)
        .Arc("spread")
        .Field<&ParticleSpawner::spread>("spread")
        .Tooltip("Width of the emission cone, in degrees.")
        .Degrees()
        .Range(0.0, 360.0)
        .Scrub(1.0)
        .Field<&ParticleSpawner::target>("target")
        .References("Transform2D")
        .Tooltip("The entity particles leave toward, in the TowardTarget mode.");
}

} // namespace pixie
