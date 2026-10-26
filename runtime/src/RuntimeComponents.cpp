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
#include "pixie/runtime/SceneMovement2D.hpp"

namespace pixie {
namespace {

// Version 1 saved the motor's own downward acceleration as `gravity`. Version 2 takes the World's
// gravity, which is 1100 down without PhysicsSettings2D: the old acceleration over 1100 keeps a motor
// falling as it did without settings, and as the version 1 bridge made it fall with them.
void UpgradeCharacterMotor(CharacterMotor2D& motor, int savedVersion, LegacyFields& legacy) {
    const auto gravity = legacy.find("gravity");
    if (savedVersion < 2 && gravity != legacy.end()) {
        motor.gravityScale = gravity->second.x / 1100.0;
        legacy.erase(gravity);
    }
}

} // namespace

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
        .Tooltip("Red, green, blue, and alpha.")
        .Field<&Shape::visible>("visible")
        .Tooltip("Whether it is drawn and picked; a hidden one keeps its place and is still saved.");
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
        .Tooltip("Smooth blends texels, and stays clean when drawn small; Pixel keeps pixel art crisp.")
        .Field<&Sprite::visible>("visible")
        .Tooltip("Whether it is drawn and picked; a hidden one keeps its place and is still saved.");
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
        .Scrub(0.1)
        .Field<&Text::visible>("visible")
        .Tooltip("Whether it is drawn and picked; a hidden one keeps its place and is still saved.");
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
        .Scrub(0.005)
        .Field<&Path::visible>("visible")
        .Tooltip("Whether it is drawn and picked; a hidden one keeps its place and is still saved.");
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
        .Scrub(10.0)
        .Field<&ParticleSystem::style, &ParticleStyle::color>("color")
        .Tooltip("The particles' color; a lower alpha makes them fainter.")
        .Field<&ParticleSystem::style, &ParticleStyle::size>("size")
        .Tooltip("A particle's radius, in world units; it is never drawn smaller than a pixel.")
        .Range(0.0, 1000.0)
        .Scrub(0.05)
        .Field<&ParticleSystem::style, &ParticleStyle::additive>("additive")
        .Tooltip("Adds the light of overlapping particles, so dense places glow; otherwise they "
                 "cover each other.")
        .Field<&ParticleSystem::style, &ParticleStyle::fade>("fade")
        .Tooltip("Fades each particle out over its lifetime; particles that live forever do not fade.");
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

    types.Add<Collider2D>("Collider2D")
        .CanAdd([](const World& world, entt::entity entity) -> std::string {
            if (world.Registry().all_of<CompoundCollider2D>(entity)) return "CompoundCollider2D already supplies geometry. Edit its shapes or remove it first.";
            return world.Registry().all_of<CharacterMotor2D>(entity)
                ? "CharacterMotor2D already owns a capsule. Remove the motor before adding Collider2D." : "";
        })
        .Field<&Collider2D::kind>("kind", {"Box", "Circle", "Polygon"}).Tooltip("Collision geometry: box, circle or convex polygon; separate from the visible Shape.")
        .Field<&Collider2D::halfSize>("halfSize").Tooltip("Full box width (X) and height (Y), before Transform2D scale. Saved half extents stay compatible.").Label("size").Unit("", 2).Range(0.02, 200000).Scrub(1)
        .Field<&Collider2D::radius>("radius").Tooltip("Circle radius in local world units, before uniform scale.").Range(0.01, 100000).Scrub(1)
        .Field<&Collider2D::offset>("offset").Tooltip("Local offset, transformed with the collider.").Scrub(1)
        .Field<&Collider2D::points>("points").Tooltip("3 to 8 convex hull points for Polygon.")
        .Field<&Collider2D::category>("category").Label("belongs to").Tooltip("Membership layers. At least one is required. Both sides must allow a collision.").Editor(FieldEditor::LayerMask).References("CollisionLayers2D").Range(1, 2147483647)
        .Field<&Collider2D::mask>("mask").Label("collides with").Tooltip("Allowed membership layers of other colliders; none ignores all layers. Both sides must allow the pair.").Editor(FieldEditor::LayerMask).References("CollisionLayers2D").Range(0, 2147483647)
        .Field<&Collider2D::enabled>("enabled").Tooltip("Whether this collider participates in queries and simulation.")
        .Field<&Collider2D::sensor>("sensor").Tooltip("Detects overlap without blocking motion. Sensors contribute no mass; dynamic bodies need a solid shape.")
        .Field<&Collider2D::density>("density").Tooltip("Mass per square meter; positive density is required for dynamic geometry.").Unit("kg/m^2").Range(0, 1000000).Scrub(0.1)
        .Field<&Collider2D::friction>("friction").Tooltip("Nonnegative surface friction; pairs use the geometric mean.").Range(0, 1000000).Scrub(0.01)
        .Field<&Collider2D::restitution>("restitution").Tooltip("Bounciness from zero to one; pairs use the greater value.").Range(0, 1).Scrub(0.01)
        .Field<&Collider2D::rotation>("rotation").Tooltip("Local shape angle, applied before the entity's scale and placement.").Degrees().Scrub(1);
    types.Add<CompoundCollider2D>("CompoundCollider2D")
        .CanAdd([](const World& world, entt::entity entity) -> std::string {
            if (world.Registry().all_of<CharacterMotor2D>(entity)) return "CharacterMotor2D supplies its own capsule. Remove the motor first.";
            if (world.Registry().all_of<Collider2D>(entity)) return "Convert Collider2D with Make compound, or remove it first.";
            return {};
        })
        .Field<&CompoundCollider2D::enabled>("enabled").Tooltip("Enable all active shapes on this body.")
        .Field<&CompoundCollider2D::shapes>("shapes").Tooltip("Up to 256 shapes sharing one body. Each has its own local pose, material and filter.");
    types.Add<PhysicsSettings2D>("PhysicsSettings2D")
        .Singleton()
        .Field<&PhysicsSettings2D::gravity>("gravity").Tooltip("World gravity in world units/s^2; Y points down. One settings entity per scene.").Scrub(10)
        .Field<&PhysicsSettings2D::unitsPerMeter>("unitsPerMeter").Label("units per meter").Tooltip("World units per physical meter. Change only while editing; default 32.").Range(0.001, 1000000)
        .Field<&PhysicsSettings2D::substeps>("substeps").Tooltip("Solver subdivisions per fixed World step, from 1 to 32.").Range(1, 32);
    types.Add<CollisionLayer2D>("CollisionLayer2D").ValueType()
        .Field<&CollisionLayer2D::slot>("slot").Range(1, 31).Tooltip("Stable 1-based bit position; never an index into the name list.")
        .Field<&CollisionLayer2D::name>("name").Tooltip("Display name only; empty means unnamed. Renaming does not change collision filters.");
    types.Add<CollisionLayers2D>("CollisionLayers2D").Singleton()
        .Field<&CollisionLayers2D::layers>("layers").Label("collision layer names")
        .Editor(FieldEditor::LayerNames).Tooltip("Scene-wide names for 31 fixed slots. Empty names keep their collision bits. This component does not change world gravity.");
    types.Add<RigidBody2D>("RigidBody2D")
        .CanAdd([](const World& world, entt::entity entity) -> std::string {
            return world.Registry().all_of<CharacterMotor2D>(entity)
                ? "CharacterMotor2D owns movement. Remove the motor before adding RigidBody2D; use a dynamic box for a tumbling object." : "";
        })
        .Field<&RigidBody2D::type>("type", {"Static", "Kinematic", "Dynamic"}).Tooltip("Static stays fixed; kinematic follows velocity; dynamic responds to gravity and contacts.")
        .Field<&RigidBody2D::initialVelocity>("initialVelocity").Label("initial velocity").Tooltip("Authored starting velocity in world units/s. Live velocity is runtime state.").Scrub(1)
        .Field<&RigidBody2D::initialAngularVelocity>("initialAngularVelocity").Label("initial angular velocity").Tooltip("Authored starting rotation speed, displayed in degrees/s.").Degrees().Scrub(1)
        .Field<&RigidBody2D::gravityScale>("gravityScale").Label("gravity scale").Tooltip("Multiplier of world gravity; zero ignores gravity.").Range(-100, 100).Scrub(0.01)
        .Field<&RigidBody2D::linearDamping>("linearDamping").Label("linear damping").Tooltip("Nonnegative velocity damping in inverse seconds.").Range(0, 10000).Scrub(0.01)
        .Field<&RigidBody2D::angularDamping>("angularDamping").Label("angular damping").Tooltip("Nonnegative angular velocity damping in inverse seconds.").Range(0, 10000).Scrub(0.01)
        .Field<&RigidBody2D::massOverride>("massOverride").Label("mass override").Tooltip("Kilograms. Zero computes mass from shape density; positive scales the computed distribution.").Unit("kg").Range(0, 1000000).Scrub(0.1)
        .Field<&RigidBody2D::fixedRotation>("fixedRotation").Label("lock rotation").Tooltip("Prevents solver-driven angular movement.")
        .Field<&RigidBody2D::continuous>("continuous").Label("fast body").Tooltip("Enables additional backend continuous collision handling; supported speeds still need qualification.");
    types.Add<CharacterMotor2D>("CharacterMotor2D")
        .Version(2)
        .Upgrade<&UpgradeCharacterMotor>()
        .CanAdd([](const World& world, entt::entity entity) -> std::string {
            if (world.Registry().all_of<RigidBody2D>(entity))
                return "RigidBody2D owns movement. Remove it before adding CharacterMotor2D.";
            if (world.Registry().all_of<Collider2D>(entity))
                return "CharacterMotor2D supplies its own capsule. Remove Collider2D first.";
            if (world.Registry().all_of<CompoundCollider2D>(entity))
                return "CharacterMotor2D supplies its own capsule. Remove CompoundCollider2D first.";
            return {};
        })
        .Field<&CharacterMotor2D::radius>("radius").Tooltip("Motor capsule radius in world units; must exceed 0.01 times the world's units per meter (0.32 at the default scale).").Range(0.01, 10000).Scrub(0.5)
        .Field<&CharacterMotor2D::halfSegment>("halfSegment").Tooltip("Half the upright capsule segment length in world units.").Label("half segment").Range(0, 10000).Scrub(0.5)
        .Field<&CharacterMotor2D::maxSpeed>("maxSpeed").Tooltip("Maximum commanded horizontal speed in world units/s.").Label("max speed").Range(0, 100000).Scrub(1)
        .Field<&CharacterMotor2D::acceleration>("acceleration").Tooltip("Horizontal acceleration in world units/s^2.").Range(0, 100000).Scrub(10)
        .Field<&CharacterMotor2D::braking>("braking").Tooltip("Horizontal braking acceleration in world units/s^2.").Range(0, 100000).Scrub(10)
        .Field<&CharacterMotor2D::gravityScale>("gravityScale").Label("gravity scale").Tooltip("Multiplier of world gravity (PhysicsSettings2D; 1100 down without one); zero ignores gravity.").Range(0, 100).Scrub(0.01)
        .Field<&CharacterMotor2D::terminalSpeed>("terminalSpeed").Tooltip("Maximum downward speed in world units/s.").Label("terminal speed").Range(0, 100000).Scrub(10)
        .Field<&CharacterMotor2D::jumpSpeed>("jumpSpeed").Tooltip("Upward takeoff speed in world units/s.").Label("jump speed").Range(0, 100000).Scrub(1)
        .Field<&CharacterMotor2D::maxSlopeDegrees>("maxSlopeDegrees").Tooltip("Maximum angle of walkable ground in degrees.").Label("max slope").Range(0, 89).Scrub(1)
        .Field<&CharacterMotor2D::coyoteSeconds>("coyoteSeconds").Tooltip("Grace interval for jumping after leaving support.").Label("ledge grace").Unit("s").Range(0, 1).Scrub(0.01)
        .Field<&CharacterMotor2D::jumpBufferSeconds>("jumpBufferSeconds").Tooltip("Time an early jump command remains pending.").Label("jump buffer").Unit("s").Range(0, 1).Scrub(0.01);
    types.Add<KeyboardControl2D>("KeyboardControl2D")
        .Field<&KeyboardControl2D::enabled>("enabled")
        .Tooltip("A/D or arrows move; Space jumps. Remove this component when game code supplies commands.");
}

} // namespace pixie
