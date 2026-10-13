#pragma once

#include "pixie/runtime/Entity.hpp"
#include "pixie/runtime/Project.hpp"

#include <memory>

// Anchor 1 as a Project: boids that steer toward the pointer, or away from it, within a radius.
// The same project runs in the boids player and, through the C API, in the editor.
class BoidsProject final : public pixie::Project {
public:
    explicit BoidsProject(int count = 1000) : count_(count) {}

    std::string_view Name() const override { return "boids"; }
    pxColorRgba Background() const override { return pxColorRgba(14, 17, 26); }
    void Describe(pixie::ComponentTypes& types) const override;
    void Build(pixie::World& world, pixie::Camera2D& camera) override;
    void BeforeStep(pixie::World& world, pixie::Camera2D& camera, const pixie::Input& input) override;
    void DrawOverlay(pixie::World& world, pixie::ShapeRenderer& shapes,
                     const pixie::Camera2D& camera) override;

private:
    int count_;
    pixie::Entity controls_;
};
