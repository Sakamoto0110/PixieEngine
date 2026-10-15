#pragma once

#include "pixie/runtime/Entity.hpp"
#include "pixie/runtime/Project.hpp"

#include <memory>

// What the flocks scene is built from: a component on the scene's "scenario" entity, which a tool
// edits and the scene saves. The project keeps a copy, read back before the scene is rebuilt, for a
// World built without a scene.
struct FlocksScenario {
    int count = 5000;
};

// Anchor 2 as a Project: boids that flock with their neighbours and react to the pointer. The scene
// (samples/flocks/scenes/main.scene) holds the scenario and the flock's controls; the boids are
// generated from the scenario's count. The same project runs in the flocks player and, through the
// C API, in the editor.
class FlocksProject final : public pixie::Project {
public:
    explicit FlocksProject(FlocksScenario scenario = {}) : scenario_(scenario) {}

    std::string_view Name() const override { return "flocks"; }
    pxColorRgba Background() const override { return pxColorRgba(12, 14, 22); }
    void Describe(pixie::ComponentTypes& types) const override;
    void Build(pixie::World& world, pixie::Camera2D& camera) override;
    void Capture(pixie::World& world) override;
    void BeforeStep(pixie::World& world, pixie::Camera2D& camera, const pixie::Input& input) override;
    bool SpawnAt(pixie::World& world, pxPoint at) override;
    void DrawOverlay(pixie::World& world, pixie::ShapeRenderer& shapes,
                     const pixie::Camera2D& camera) override;

private:
    FlocksScenario scenario_;
    pixie::Entity scenarioEntity_;
    pixie::Entity flock_;
};
