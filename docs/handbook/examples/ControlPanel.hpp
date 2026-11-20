#pragma once

#include "pixie/runtime/ScreenCanvas.hpp"
#include "pixie/platform/Input.hpp"
#include <algorithm>
#include <variant>

// A game-owned screen panel, not an Engine widget or a reflected component.
// Call Read from Project::BeforeStep, Draw from Project::DrawScreen.
struct ControlPanel {
    struct Rect {
        double x, y, width, height;
        bool Contains(pxPoint p) const {
            return p.x >= x && p.y >= y && p.x < x + width && p.y < y + height;
        }
    };
    struct Layout { Rect panel, left, right, toggle; };
    bool active = false;
    int direction = 0; // -1, 0, +1; game code decides what this intent means.

    static Layout Arrange(pxVec2 size) {
        const double width = std::max(0.0, size.x);
        const double height = std::max(0.0, size.y);
        const double panelHeight = std::min(160.0, height * 0.35);
        const double y = height - panelHeight;
        const double button = std::min(56.0, std::min(width / 5.0, panelHeight * 0.55));
        const double gap = button * 0.2;
        return {{0, y, width, panelHeight},
                {gap, y + gap, button, button},
                {button + 2 * gap, y + gap, button, button},
                {width - button - gap, y + gap, button, button}};
    }

    void Read(const pixie::Input& input, pxVec2 size) {
        const auto layout = Arrange(size);
        direction = 0;
        if (input.IsDown(pixie::MouseButton::Left)) {
            if (layout.left.Contains(input.Cursor())) direction = -1;
            if (layout.right.Contains(input.Cursor())) direction = 1;
        }
        // Use each press's position, not the final cursor after several events.
        for (const auto& event : input.Events()) {
            const auto* button = std::get_if<pixie::MouseButtonEvent>(&event);
            if (button && button->button == pixie::MouseButton::Left && button->Pressed()
                && layout.toggle.Contains(button->position)) active = !active;
        }
    }

    void Draw(pixie::ScreenCanvas& canvas) const {
        const auto layout = Arrange(canvas.Size());
        const auto rectangle = [&canvas](Rect r, pxColorRgba color) {
            canvas.Rect({r.x, r.y}, {r.width, r.height}, color);
        };
        rectangle(layout.panel, {24, 28, 38});
        rectangle(layout.left, direction < 0 ? pxColorRgba{70, 170, 255} : pxColorRgba{65, 75, 95});
        rectangle(layout.right, direction > 0 ? pxColorRgba{70, 170, 255} : pxColorRgba{65, 75, 95});
        rectangle(layout.toggle, active ? pxColorRgba{70, 230, 140} : pxColorRgba{110, 70, 65});
        const auto arrow = [&canvas](Rect r, double sign) {
            const double cx = r.x + r.width * 0.5, cy = r.y + r.height * 0.5;
            const pxPoint points[] = {{cx - sign * r.width * 0.15, cy - r.height * 0.22},
                                      {cx + sign * r.width * 0.15, cy},
                                      {cx - sign * r.width * 0.15, cy + r.height * 0.22}};
            canvas.Path(points, 3.0, {240, 245, 255});
        };
        arrow(layout.left, -1.0);
        arrow(layout.right, 1.0);
        canvas.Circle({layout.toggle.x + layout.toggle.width * 0.5,
                       layout.toggle.y + layout.toggle.height * 0.5},
                      layout.toggle.width * 0.12, {235, 245, 255});
    }
};
