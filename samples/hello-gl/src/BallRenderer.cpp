#include "BallRenderer.hpp"

#include "Shader.hpp"

#include <cstddef>

namespace {

// The quad's corners come from gl_VertexID (a 4-vertex triangle strip), so only per-instance
// data lives in a buffer.
constexpr std::string_view kVertexShader = R"(#version 450 core
layout(location = 0) in vec2 aCenter;
layout(location = 1) in float aRadius;
layout(location = 2) in vec4 aColor;

layout(location = 0) uniform vec2 uViewport;

out vec2 vLocal;
out vec4 vColor;

void main() {
    vec2 corner = vec2((gl_VertexID & 1) == 0 ? -1.0 : 1.0, (gl_VertexID & 2) == 0 ? -1.0 : 1.0);
    vec2 pixel = aCenter + corner * aRadius;
    vec2 ndc = pixel / uViewport * 2.0 - 1.0;
    gl_Position = vec4(ndc.x, -ndc.y, 0.0, 1.0);
    vLocal = corner;
    vColor = aColor;
}
)";

constexpr std::string_view kFragmentShader = R"(#version 450 core
in vec2 vLocal;
in vec4 vColor;

out vec4 oColor;

void main() {
    float distance = length(vLocal);
    float edge = fwidth(distance);
    float coverage = 1.0 - smoothstep(1.0 - edge, 1.0, distance);
    if (coverage <= 0.0) {
        discard;
    }
    oColor = vec4(vColor.rgb, vColor.a * coverage);
}
)";

void Attribute(GLuint vertexArray, GLuint location, GLint components, std::size_t offset) {
    glEnableVertexArrayAttrib(vertexArray, location);
    glVertexArrayAttribFormat(vertexArray, location, components, GL_FLOAT, GL_FALSE,
                              static_cast<GLuint>(offset));
    glVertexArrayAttribBinding(vertexArray, location, 0);
}

} // namespace

bool BallRenderer::Create() {
    program_ = CompileProgram(kVertexShader, kFragmentShader);
    if (program_ == 0) {
        return false;
    }
    glCreateBuffers(1, &instanceBuffer_);
    glCreateVertexArrays(1, &vertexArray_);
    glVertexArrayVertexBuffer(vertexArray_, 0, instanceBuffer_, 0, sizeof(BallInstance));
    glVertexArrayBindingDivisor(vertexArray_, 0, 1);
    Attribute(vertexArray_, 0, 2, offsetof(BallInstance, center));
    Attribute(vertexArray_, 1, 1, offsetof(BallInstance, radius));
    Attribute(vertexArray_, 2, 4, offsetof(BallInstance, color));
    return true;
}

void BallRenderer::Destroy() {
    glDeleteVertexArrays(1, &vertexArray_);
    glDeleteBuffers(1, &instanceBuffer_);
    glDeleteProgram(program_);
    vertexArray_ = 0;
    instanceBuffer_ = 0;
    program_ = 0;
}

void BallRenderer::Draw(std::span<const BallInstance> balls, pixie::FramebufferSize viewport) {
    if (balls.empty()) {
        return;
    }
    // Orphan and refill every frame; the data is small and changes every frame.
    glNamedBufferData(instanceBuffer_, static_cast<GLsizeiptr>(balls.size_bytes()), balls.data(),
                      GL_STREAM_DRAW);
    glProgramUniform2f(program_, 0, static_cast<float>(viewport.width),
                       static_cast<float>(viewport.height));
    glUseProgram(program_);
    glBindVertexArray(vertexArray_);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, static_cast<GLsizei>(balls.size()));
}
