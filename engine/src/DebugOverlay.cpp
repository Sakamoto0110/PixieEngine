#include "DebugOverlay.hpp"
#include "DebugConsole.hpp"

#include <stb_easy_font.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <format>
#include <string>

namespace pixie {
namespace {

constexpr std::string_view kVertex = R"(#version 450 core
layout(location=0) in vec2 position;
layout(location=1) in vec4 color;
layout(location=0) uniform vec2 viewport;
out vec4 tint;
void main() {
    gl_Position = vec4(position.x * 2.0 / viewport.x - 1.0, 1.0 - position.y * 2.0 / viewport.y, 0.0, 1.0);
    tint = color;
}
)";
constexpr std::string_view kFragment = R"(#version 450 core
in vec4 tint;
out vec4 pixel;
void main() { pixel = tint; }
)";

// App rendering can leave a custom target, blend mode, or clipping state. The overlay draws on the
// window after it, then returns every state it changes. It neither clears nor renders the World.
class OverlayState {
public:
    OverlayState() {
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &target_);
        glGetIntegerv(GL_CURRENT_PROGRAM, &program_);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &array_);
        glGetIntegerv(GL_VIEWPORT, viewport_.data());
        glGetIntegerv(GL_POLYGON_MODE, polygon_.data());
        glGetIntegerv(GL_BLEND_SRC_RGB, &sourceRgb_);
        glGetIntegerv(GL_BLEND_DST_RGB, &destinationRgb_);
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &sourceAlpha_);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &destinationAlpha_);
        glGetIntegerv(GL_BLEND_EQUATION_RGB, &equationRgb_);
        glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &equationAlpha_);
        glGetBooleanv(GL_COLOR_WRITEMASK, colorMask_.data());
        for (std::size_t i = 0; i < kFlags.size(); ++i) enabled_[i] = glIsEnabled(kFlags[i]);
    }
    ~OverlayState() {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(target_));
        glUseProgram(static_cast<GLuint>(program_));
        glBindVertexArray(static_cast<GLuint>(array_));
        glViewport(viewport_[0], viewport_[1], viewport_[2], viewport_[3]);
        glPolygonMode(GL_FRONT_AND_BACK, static_cast<GLenum>(polygon_[0]));
        glBlendFuncSeparate(static_cast<GLenum>(sourceRgb_), static_cast<GLenum>(destinationRgb_),
                            static_cast<GLenum>(sourceAlpha_), static_cast<GLenum>(destinationAlpha_));
        glBlendEquationSeparate(static_cast<GLenum>(equationRgb_), static_cast<GLenum>(equationAlpha_));
        glColorMask(colorMask_[0], colorMask_[1], colorMask_[2], colorMask_[3]);
        for (std::size_t i = 0; i < kFlags.size(); ++i) {
            if (enabled_[i]) glEnable(kFlags[i]); else glDisable(kFlags[i]);
        }
    }
    void Apply(FramebufferSize size) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glViewport(0, 0, size.width, size.height);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        for (const auto flag : kFlags) glDisable(flag);
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }
private:
    static constexpr std::array<GLenum, 8> kFlags = {GL_BLEND, GL_DEPTH_TEST, GL_CULL_FACE, GL_SCISSOR_TEST,
        GL_STENCIL_TEST, GL_RASTERIZER_DISCARD, GL_COLOR_LOGIC_OP, GL_SAMPLE_ALPHA_TO_COVERAGE};
    std::array<GLboolean, kFlags.size()> enabled_{};
    std::array<GLint, 4> viewport_{};
    std::array<GLint, 2> polygon_{};
    std::array<GLboolean, 4> colorMask_{};
    GLint target_ = 0, program_ = 0, array_ = 0;
    GLint sourceRgb_ = 0, destinationRgb_ = 0, sourceAlpha_ = 0, destinationAlpha_ = 0;
    GLint equationRgb_ = 0, equationAlpha_ = 0;
};

} // namespace

DebugOverlay::~DebugOverlay() { Destroy(); }

bool DebugOverlay::Create() {
    static_assert(sizeof(Vertex) == 16);
    if (!program_.Create("Player debug overlay", kVertex, kFragment)) return false;
    buffer_.Create();
    array_.Create();
    array_.BindBuffer(0, buffer_.Id(), sizeof(Vertex), false);
    array_.Attribute(0, 0, 2, GL_FLOAT, offsetof(Vertex, x));
    array_.Attribute(1, 0, 4, GL_UNSIGNED_BYTE, offsetof(Vertex, color), true);
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
    OverlayState state;
    state.Apply(size);
    buffer_.Upload(std::span<const Vertex>(vertices_));
    glUseProgram(program_.Id());
    glUniform2f(0, width, static_cast<float>(size.height));
    glBindVertexArray(array_.Id());
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices_.size()));
}

} // namespace pixie
