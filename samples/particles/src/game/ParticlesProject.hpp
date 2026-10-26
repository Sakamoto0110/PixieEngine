#pragma once

#include "pixie/runtime/Entity.hpp"
#include "pixie/runtime/Project.hpp"

#include <memory>

// What the particles scene is built from: a component on the scene's "scenario" entity, which a
// tool edits and the scene saves. The project keeps a copy, read back before the scene is rebuilt,
// for a World built without a scene. The attractors and spawners are entities of the scene.
struct ParticlesScenario {
    int count = 10000;
    bool interaction = true;
};

// Anchor 3 as a Project: particles in orbit around attractors, and spawners feeding them, that the
// user places, drags, and removes. The scene (samples/particles/scenes/main.scene) holds the
// scenario, the particle system with its look, attraction, and interaction, the attractors, and the
// spawners; the particles
// are generated from the scenario's count. The same project runs in the particles player and,
// through the C API, in the editor. Its screen layer shows the counts, and labels each spawner.
class ParticlesProject final : public pixie::Project {
public:
    explicit ParticlesProject(ParticlesScenario scenario = {}) : scenario_(scenario) {}

    std::string_view Name() const override { return "particles"; }
    pxColorRgba Background() const override { return pxColorRgba(6, 8, 14); }
    void Describe(pixie::ComponentTypes& types) const override;
    void Build(pixie::World& world, pixie::Camera2D& camera) override;
    void Capture(pixie::World& world) override;
    void BeforeStep(pixie::World& world, pixie::Camera2D& camera, const pixie::Input& input) override;
    bool SpawnAt(pixie::World& world, pxPoint at) override;
    void DrawScreen(pixie::World& world, pixie::ScreenCanvas& screen) override;

private:
    void HandleMouse(pixie::World& world, pixie::Camera2D& camera, const pixie::Input& input);
    void HandleKeys(pixie::World& world, const pixie::Input& input);
    void Respawn(pixie::World& world, int count);
    // The attractor or spawner nearest `screen` within reach of the pointer, or none.
    pixie::Entity HandleAt(pixie::World& world, const pixie::Camera2D& camera, pxPoint screen);

    ParticlesScenario scenario_;
    pixie::Entity scenarioEntity_;
    int count_ = 0;
    pixie::Entity emitter_;
    pixie::Entity dragged_;
};
