#pragma once

#include "pixie/runtime/Entity.hpp"
#include "pixie/runtime/Project.hpp"

#include <memory>

// What the boids scene is built from: owned by the project, mirrored in a component on the
// "scenario" entity so a tool can edit it, and read back before the scene is rebuilt.
struct BoidsScenario {
    int count = 1000;
};

// Anchor 1 as a Project: boids that steer toward the pointer, or away from it, within a radius.
// The same project runs in the boids player and, through the C API, in the editor.
class BoidsProject final : public pixie::Project {
public:
    explicit BoidsProject(BoidsScenario scenario = {}) : scenario_(scenario) {}

    std::string_view Name() const override { return "boids"; }
    pxColorRgba Background() const override { return pxColorRgba(14, 17, 26); }
    void Describe(pixie::ComponentTypes& types) const override;
    void Build(pixie::World& world, pixie::Camera2D& camera) override;
    void Capture(pixie::World& world) override;
    void BeforeStep(pixie::World& world, pixie::Camera2D& camera, const pixie::Input& input) override;
    void DrawOverlay(pixie::World& world, pixie::ShapeRenderer& shapes,
                     const pixie::Camera2D& camera) override;

private:
    BoidsScenario scenario_;
    pixie::Entity scenarioEntity_;
    pixie::Entity controls_;
};
