#pragma once

#include "pixie/render/2d/Camera2D.hpp"
#include "pixie/platform/Input.hpp"
#include "pixie/render/2d/ShapeRenderer.hpp"

#include <pixie/pxColorRgba.hpp>
#include <pixie/pxPoint.hpp>

#include <memory>
#include <string_view>

namespace pixie {

class ComponentTypes;
class Entity;
class ScreenCanvas;
class World;

struct AuthoringCircle2D {
    pxPoint center;
    double radius = 0.0;             // world units
    double worldUnitsPerMeter = 1.0;
};

// A game's code as the engine sees it: how to build its World, what to do with the host's view and
// input before each step, what to draw on top and on the screen, and which of its components a tool may inspect. The
// same Project runs in its player executable (through ProjectApp) and in an editor (through the C
// API's session), so a game is written once.
class Project {
public:
    virtual ~Project() = default;

    virtual std::string_view Name() const = 0;

    // The color the view is cleared to.
    virtual pxColorRgba Background() const { return pxColorRgba(9, 11, 19); }

    // Adds the game's own component types; the runtime's are already there.
    virtual void Describe(ComponentTypes& /*types*/) const {}

    /// Builds runtime state in a fresh World, after any authored scene has been read into it.
    /// The camera's viewport is set and Build may move or zoom it. A factory-qualified authoring
    /// candidate recreates derived/runtime state and treats supplied authored data as authoritative.
    virtual void Build(World& world, Camera2D& camera) = 0;

    // Runs before the World is thrown away to be built again: read back what the next Build should
    // start from, such as a scenario component a tool has edited.
    virtual void Capture(World& /*world*/) {}

    // Runs before each step, outside it: copy the pointer and the view into the World, react to
    // keys, move the camera. Structural changes are allowed here.
    virtual void BeforeStep(World& /*world*/, Camera2D& /*camera*/, const Input& /*input*/) {}

    // Creates something at `at`, in world units, when a tool asks (an editor's "Ask the game to create one here" on empty
    // space): a boid, a particle. Called outside a step. Returns false when the game creates nothing.
    virtual bool SpawnAt(World& /*world*/, pxPoint /*at*/) { return false; }

    // Submits what is drawn over the World's shapes and particles, such as a pointer's ring. The
    // session flushes the renderer afterwards.
    virtual void DrawOverlay(World& /*world*/, ShapeRenderer& /*shapes*/, const Camera2D& /*camera*/) {}

    // Draws the game's interface over everything else, in pixels of the view (see ScreenCanvas): in
    // its player and in an editor's viewport, in play and in edit alike.
    virtual void DrawScreen(World& /*world*/, ScreenCanvas& /*screen*/) {}

    // Updates derived previews after authored edits, outside a simulation step.
    virtual void RefreshAuthoringPreview(World& /*world*/) {}
    virtual bool IsAuthoringFieldLocked(const Entity& /*entity*/, std::string_view /*type*/,
                                        std::string_view /*field*/) const { return false; }
    virtual bool AuthoringCircle(const Entity& /*entity*/, AuthoringCircle2D& /*circle*/) const { return false; }
};

// Every game library defines this: the project with its default settings. The C API's library for
// the game calls it, so each game's library hosts exactly that game.
std::unique_ptr<Project> CreateProject();

} // namespace pixie
