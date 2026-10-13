#pragma once

#include "pixie/runtime/Entity.hpp"
#include "pixie/runtime/Project.hpp"

#include <memory>

// Anchor 2 as a Project: boids that flock with their neighbours and react to the pointer. The same
// project runs in the flocks player and, through the C API, in the editor.
class FlocksProject final : public pixie::Project {
public:
    explicit FlocksProject(int count = 5000) : count_(count) {}

    std::string_view Name() const override { return "flocks"; }
    pxColorRgba Background() const override { return pxColorRgba(12, 14, 22); }
    void Describe(pixie::ComponentTypes& types) const override;
    void Build(pixie::World& world, pixie::Camera2D& camera) override;
    void BeforeStep(pixie::World& world, pixie::Camera2D& camera, const pixie::Input& input) override;
    void DrawOverlay(pixie::World& world, pixie::ShapeRenderer& shapes,
                     const pixie::Camera2D& camera) override;

private:
    int count_;
    pixie::Entity flock_;
};
