#include "DebugOverlay.hpp"
#include "DebugConsole.hpp"

#include <stb_easy_font.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <format>
#include <string>

namespace pixie {


DebugOverlay::~DebugOverlay() { Destroy(); }

bool DebugOverlay::Create() {
    if (!CreateOverlayDrawing(program_, buffer_, array_)) return false;
    vertices_.reserve(8192);
    return true;
}

void DebugOverlay::Destroy() {
    array_.Destroy();
    buffer_.Destroy();
    program_.Destroy();
}

void DebugOverlay::Rect(float x, float y, float width, float height, Color color) {
    const Vertex a{x, y, 0, color}, b{x + width, y, 0, color};
    const Vertex c{x + width, y + height, 0, color}, d{x, y + height, 0, color};
    vertices_.insert(vertices_.end(), {a, b, c, a, c, d});
}

void DebugOverlay::Text(std::string_view text, float x, float y, Color color, float scale) {
    std::string copy(text);
    const int quads = stb_easy_font_print(0, 0, copy.data(), color.data(), glyphs_.data(), static_cast<int>(sizeof glyphs_));
    constexpr std::array<std::size_t, 6> kCorners = {0, 1, 2, 0, 2, 3};
    for (int quad = 0; quad < quads; ++quad) {
        for (const auto corner : kCorners) {
            auto vertex = glyphs_[static_cast<std::size_t>(quad) * 4 + corner];
            vertex.x = x + vertex.x * scale;
            vertex.y = y + vertex.y * scale;
            vertices_.push_back(vertex);
        }
    }
}

void DebugOverlay::Draw(const DebugConsole& console, FramebufferSize size, double fps, double ups, double seconds,
                        std::span<const MetricSnapshot> host, std::span<const MetricSnapshot> app,
                        bool hostEnabled, bool appEnabled) {
    if ((!console.ShowsRates() && !console.IsOpen()) || size.width <= 0 || size.height <= 0) return;
    vertices_.clear();
    const float width = static_cast<float>(size.width);
    constexpr Color kText{235, 239, 245, 255};
    constexpr Color kHint{180, 197, 214, 255};
    if (console.IsOpen()) {
        Rect(0, 0, width, 90, {17, 22, 31, 250});
        Text("ENGINE COMMANDS  |  Enter: run  Esc: close", 10, 8, kHint);
        Rect(8, 31, std::max(0.0f, width - 16), 27, {53, 69, 89, 255});
        Rect(9, 32, std::max(0.0f, width - 18), 25, {25, 33, 45, 255});
        // Keep the caret in view in a narrow window. Commands are ASCII, so these are byte positions.
        const auto room = static_cast<std::size_t>(std::max(1.0f, (width - 46) / 14));
        const auto start = console.Cursor() > room ? console.Cursor() - room : 0;
        const std::string visible = console.Line().substr(start, room);
        Text("> " + visible, 13, 37, kText);
        if (std::fmod(seconds, 1.0) < 0.65) {
            std::string before = "> " + console.Line().substr(start, console.Cursor() - start);
            const float caret = 13 + static_cast<float>(stb_easy_font_width(before.data())) * 2;
            Rect(caret, 35, 2, 20, kText);
        }
        Text(console.Feedback(), 10, 67, kHint);
    }
    if (console.ShowsRates()) {
        const float top = console.IsOpen() ? 100.0f : 10.0f;
        Rect(10, top, 210, 57, {17, 22, 31, 230});
        Text(std::format("FPS: {:.1f}", fps), 19, top + 8, kText);
        Text(std::format("UPS: {:.1f}", ups), 19, top + 31, kText);
        const int capacity = std::max(0,(size.height-static_cast<int>(top)-116)/18);
        const auto count = std::min(static_cast<std::size_t>(capacity),host.size()+app.size());
        if (count > 0) {
            const float start = top+64;
            Rect(10,start,std::min(width-20,660.0f),47+18*static_cast<float>(count),{17,22,31,230});
            Text(std::format("CPU telemetry: host {} / app {}",hostEnabled?"on":"off",appEnabled?"on":"off"),19,start+7,kHint,1.5f);
            std::size_t row = 0;
            const auto rows = [&](std::span<const MetricSnapshot> metrics) {
                for (const auto& metric : metrics) {
                    if (row >= count) break;
                    std::string line = metric.name.substr(0,34)+": ";
                    if (metric.kind == MetricKind::Timing)
                        line += std::format("{:.3f} avg / {:.3f} peak ms",metric.timing.meanMs,metric.timing.peakMs);
                    else if (metric.kind == MetricKind::Gauge) line += std::format("{:.2f} {}",metric.gauge,metric.unit);
                    else line += std::format("{} {}",metric.counter,metric.unit);
                    Text(line,19,start+27+18*static_cast<float>(row++),kText,1.5f);
                }
            };
            rows(host); rows(app);
            if (count < host.size()+app.size()) Text("More metrics available through Snapshot / CSV",19,start+27+18*static_cast<float>(count),kHint,1.0f);
        }
    }
    DrawOverlay(vertices_, size, buffer_, array_, program_);
}

} // namespace pixie
