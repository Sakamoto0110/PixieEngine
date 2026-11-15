#include "pixie/runtime/SceneProject.hpp"

#include <cmath>

namespace pixie {
namespace {

constexpr double kZoomStep = 1.15;

} // namespace

void PanAndZoom(Camera2D& camera, const Input& input) {
    // Work back from the final button state so only motion while held pans the
    // camera, even when a press and release arrive before the same update.
    auto held = input.IsDown(MouseButton::Middle);
    pxVec2 pan(0.0);
    const auto events = input.Events();
    for (auto it = events.rbegin(); it != events.rend(); ++it) {
        if (const auto* button = std::get_if<MouseButtonEvent>(&*it);
            button && button->button == MouseButton::Middle) {
            held = button->Released();
        } else if (held) {
            if (const auto* move = std::get_if<MouseMoveEvent>(&*it)) {
                pan += move->delta;
            }
        }
    }
    if (pan.x != 0.0 || pan.y != 0.0) {
        camera.Pan(pan);
    }
    if (input.Scroll().y != 0.0) {
        camera.ZoomAt(input.Cursor(), std::pow(kZoomStep, input.Scroll().y));
    }
}

void SceneProject::BeforeStep(World&, Camera2D& camera, const Input& input) {
    PanAndZoom(camera, input);
}

} // namespace pixie
