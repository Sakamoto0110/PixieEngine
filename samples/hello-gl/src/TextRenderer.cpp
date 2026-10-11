#include "TextRenderer.hpp"

#include "Shader.hpp"

#include <stb_easy_font.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace {

// stb_easy_font writes 16-byte vertices: x, y, z as floats, then 4 color bytes.
constexpr std::size_t kVertexStride = 16;
constexpr std::size_t kVerticesPerQuad = 4;

constexpr std::string_view kVertexShader = R"(#version 450 core
layout(location = 0) in vec2 aPosition;

layout(location = 0) uniform vec2 uViewport;
layout(location = 1) uniform vec2 uOrigin;
layout(location = 2) uniform float uScale;

void main() {
    vec2 pixel = uOrigin + aPosition * uScale;
    vec2 ndc = pixel / uViewport * 2.0 - 1.0;
    gl_Position = vec4(ndc.x, -ndc.y, 0.0, 1.0);
}
)";

constexpr std::string_view kFragmentShader = R"(#version 450 core
layout(location = 3) uniform vec4 uColor;

out vec4 oColor;

void main() {
    oColor = uColor;
}
)";

} // namespace

bool TextRenderer::Create() {
    program_ = CompileProgram(kVertexShader, kFragmentShader);
    if (program_ == 0) {
        return false;
    }
    glCreateBuffers(1, &vertexBuffer_);
    glCreateBuffers(1, &indexBuffer_);
    glCreateVertexArrays(1, &vertexArray_);
    glVertexArrayVertexBuffer(vertexArray_, 0, vertexBuffer_, 0, kVertexStride);
    glVertexArrayElementBuffer(vertexArray_, indexBuffer_);
    glEnableVertexArrayAttrib(vertexArray_, 0);
    glVertexArrayAttribFormat(vertexArray_, 0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vertexArray_, 0, 0);
    return true;
}

void TextRenderer::Destroy() {
    glDeleteVertexArrays(1, &vertexArray_);
    glDeleteBuffers(1, &vertexBuffer_);
    glDeleteBuffers(1, &indexBuffer_);
    glDeleteProgram(program_);
    vertexArray_ = 0;
    vertexBuffer_ = 0;
    indexBuffer_ = 0;
    program_ = 0;
    indexCount_ = 0;
}

void TextRenderer::SetText(std::string_view text) {
    // stb_easy_font takes a mutable, null-terminated string.
    std::string mutableText(text);
    std::vector<char> vertices(text.size() * 300 + 64);
    const int quadCount = stb_easy_font_print(0.0f, 0.0f, mutableText.data(), nullptr,
                                              vertices.data(), static_cast<int>(vertices.size()));
    const auto vertexCount = static_cast<std::size_t>(quadCount) * kVerticesPerQuad;

    min_ = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    max_ = {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};
    for (std::size_t i = 0; i < vertexCount; ++i) {
        float xy[2];
        std::memcpy(xy, vertices.data() + i * kVertexStride, sizeof(xy));
        min_ = {std::min(min_.x, xy[0]), std::min(min_.y, xy[1])};
        max_ = {std::max(max_.x, xy[0]), std::max(max_.y, xy[1])};
    }

    // Core profile has no quads: two triangles per quad, corners in stb's order.
    std::vector<std::uint32_t> indices;
    indices.reserve(static_cast<std::size_t>(quadCount) * 6);
    for (std::uint32_t quad = 0; quad < static_cast<std::uint32_t>(quadCount); ++quad) {
        const std::uint32_t base = quad * 4;
        indices.insert(indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
    }
    indexCount_ = static_cast<GLsizei>(indices.size());

    glNamedBufferData(vertexBuffer_, static_cast<GLsizeiptr>(vertexCount * kVertexStride),
                      vertices.data(), GL_DYNAMIC_DRAW);
    glNamedBufferData(indexBuffer_, static_cast<GLsizeiptr>(indices.size() * sizeof(std::uint32_t)),
                      indices.data(), GL_DYNAMIC_DRAW);
}

void TextRenderer::DrawCentered(pixie::FramebufferSize viewport, float widthFraction,
                                Color color) {
    const Vec2 size = max_ - min_;
    const Vec2 screen{static_cast<float>(viewport.width), static_cast<float>(viewport.height)};
    const float scale = std::max(1.0f, screen.x * widthFraction / size.x);
    Draw(viewport, (screen - size * scale) * 0.5f - min_ * scale, scale, color);
}

void TextRenderer::DrawAt(pixie::FramebufferSize viewport, Vec2 topLeft, float scale,
                          Color color) {
    Draw(viewport, topLeft - min_ * scale, scale, color);
}

void TextRenderer::Draw(pixie::FramebufferSize viewport, Vec2 origin, float scale, Color color) {
    if (indexCount_ == 0) {
        return;
    }
    glProgramUniform2f(program_, 0, static_cast<float>(viewport.width),
                       static_cast<float>(viewport.height));
    glProgramUniform2f(program_, 1, origin.x, origin.y);
    glProgramUniform1f(program_, 2, scale);
    glProgramUniform4f(program_, 3, color.r, color.g, color.b, color.a);
    glUseProgram(program_);
    glBindVertexArray(vertexArray_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
}
