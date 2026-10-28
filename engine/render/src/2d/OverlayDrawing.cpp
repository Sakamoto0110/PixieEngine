#include "pixie/render/2d/OverlayDrawing.hpp"
#include <cstddef>
#include <string_view>

namespace pixie {
namespace {
using Vertex = OverlayVertex;
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

bool CreateOverlayDrawing(ShaderProgram& program_, GlBuffer& buffer_, GlVertexArray& array_) {
    static_assert(sizeof(Vertex) == 16);
    if (!program_.Create("Player debug overlay", kVertex, kFragment)) return false;
    buffer_.Create();
    array_.Create();
    array_.BindBuffer(0, buffer_.Id(), sizeof(Vertex), false);
    array_.Attribute(0, 0, 2, GL_FLOAT, offsetof(Vertex, x));
    array_.Attribute(1, 0, 4, GL_UNSIGNED_BYTE, offsetof(Vertex, color), true);
    return true;
}

void DrawOverlay(std::span<const OverlayVertex> vertices_, FramebufferSize size,
                 GlBuffer& buffer_, const GlVertexArray& array_, const ShaderProgram& program_) {
    OverlayState state;
    state.Apply(size);
    buffer_.Upload(std::span<const Vertex>(vertices_));
    glUseProgram(program_.Id());
    glUniform2f(0, static_cast<float>(size.width), static_cast<float>(size.height));
    glBindVertexArray(array_.Id());
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices_.size()));
}

} // namespace pixie
