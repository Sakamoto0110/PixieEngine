#pragma once

#include "pixie/Camera2D.hpp"
#include "pixie/Input.hpp"
#include "pixie/runtime/Project.hpp"

#include <string>
#include <string_view>

namespace pixie {

// Moves the camera the way every view of a World does: dragging with the middle button pans, and
// the wheel zooms around the cursor.
void PanAndZoom(Camera2D& camera, const Input& input);

// A project whose World is only its scene: it builds nothing in code and knows only the engine's
// component types. A project folder without code runs as one, and a project with code starts from
// one and adds what it needs.
class SceneProject : public Project {
public:
    std::string_view Name() const override { return "scene"; }

    // Pans and zooms the camera.
    void BeforeStep(World& world, Camera2D& camera, const Input& input) override;

    // Builds nothing: the session loads the scene's entities before calling it.
    void Build(World& /*world*/, Camera2D& /*camera*/) override {}
};

} // namespace pixie
