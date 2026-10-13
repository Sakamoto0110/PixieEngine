#pragma once

#include "pixie/runtime/Entity.hpp"
#include "pixie/runtime/Project.hpp"

#include <memory>

// What the particles scene is built from: owned by the project, mirrored in a component on the
// "scenario" entity so a tool can edit it, and read back before the scene is rebuilt.
struct ParticlesScenario {
    int count = 10000;
    int attractors = 1;
    bool interaction = true;
};

// The interaction's tunable values, on the emitter entity, so a tool can change them; the project
// copies them into the InteractionAffector before each step.
struct ParticleControls {
    bool interaction = true;
    double interactionRadius = 4.0;
    double stiffness = 1200.0;
    double damping = 3.0;
};

// Anchor 3 as a Project: particles in orbit around attractors that the user places, drags, and
// removes. The same project runs in the particles player and, through the C API, in the editor.
class ParticlesProject final : public pixie::Project {
public:
    explicit ParticlesProject(ParticlesScenario scenario = {}) : scenario_(scenario) {}

    std::string_view Name() const override { return "particles"; }
    pxColorRgba Background() const override { return pxColorRgba(6, 8, 14); }
    void Describe(pixie::ComponentTypes& types) const override;
    void Build(pixie::World& world, pixie::Camera2D& camera) override;
    void Capture(pixie::World& world) override;
    void BeforeStep(pixie::World& world, pixie::Camera2D& camera, const pixie::Input& input) override;

private:
    void HandleMouse(pixie::World& world, pixie::Camera2D& camera, const pixie::Input& input);
    void HandleKeys(pixie::World& world, const pixie::Input& input);
    void Respawn(pixie::World& world, int count);
    pixie::Entity AttractorAt(pixie::World& world, const pixie::Camera2D& camera, pxPoint screen);

    ParticlesScenario scenario_;
    pixie::Entity scenarioEntity_;
    int count_ = 0;
    pixie::Entity emitter_;
    pixie::Entity dragged_;
};
