#pragma once

#include "pixie/runtime/Entity.hpp"
#include "pixie/runtime/Project.hpp"

#include <memory>

// What the flocks scene is built from: owned by the project, mirrored in a component on the
// "scenario" entity so a tool can edit it, and read back before the scene is rebuilt.
struct FlocksScenario {
    int count = 5000;
};

// Anchor 2 as a Project: boids that flock with their neighbours and react to the pointer. The same
// project runs in the flocks player and, through the C API, in the editor.
class FlocksProject final : public pixie::Project {
public:
    explicit FlocksProject(FlocksScenario scenario = {}) : scenario_(scenario) {}

    std::string_view Name() const override { return "flocks"; }
    pxColorRgba Background() const override { return pxColorRgba(12, 14, 22); }
    void Describe(pixie::ComponentTypes& types) const override;
    void Build(pixie::World& world, pixie::Camera2D& camera) override;
    void Capture(pixie::World& world) override;
    void BeforeStep(pixie::World& world, pixie::Camera2D& camera, const pixie::Input& input) override;
    void DrawOverlay(pixie::World& world, pixie::ShapeRenderer& shapes,
                     const pixie::Camera2D& camera) override;

private:
    FlocksScenario scenario_;
    pixie::Entity scenarioEntity_;
    pixie::Entity flock_;
};
