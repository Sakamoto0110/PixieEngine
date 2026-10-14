#include "pixie/runtime/SceneProject.hpp"

#include <cmath>

namespace pixie {
namespace {

constexpr double kZoomStep = 1.15;

} // namespace

void PanAndZoom(Camera2D& camera, const Input& input) {
    if (input.IsDown(MouseButton::Middle)) {
        camera.Pan(input.CursorMoved());
    }
    if (input.Scroll().y != 0.0) {
        camera.ZoomAt(input.Cursor(), std::pow(kZoomStep, input.Scroll().y));
    }
}

void SceneProject::BeforeStep(World&, Camera2D& camera, const Input& input) {
    PanAndZoom(camera, input);
}

} // namespace pixie
