#include "pixie/render/2d/ShapeRenderer.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <cstddef>
#include <string_view>

namespace pixie {

namespace {

// The corners come from gl_VertexID: three for a triangle, a four-vertex strip for the rest. Only
// per-instance data lives in a buffer.
constexpr std::string_view kVertexShader = R"(#version 450 core
layout(location = 0) in vec2 aPosition;
layout(location = 1) in float aRotation;
layout(location = 2) in vec2 aScale;
layout(location = 3) in vec4 aColor;

layout(location = 0) uniform mat4 uViewProjection;
layout(location = 1) uniform int uKind;

out vec2 vLocal;
out vec4 vColor;

const vec2 kTriangle[3] = vec2[](vec2(1.0, 0.0), vec2(-0.7, 0.6), vec2(-0.7, -0.6));

void main() {
    vec2 local = uKind == 0
        ? kTriangle[gl_VertexID]
        : vec2((gl_VertexID & 1) == 0 ? -1.0 : 1.0, (gl_VertexID & 2) == 0 ? -1.0 : 1.0);
    vec2 scaled = local * aScale;
    float c = cos(aRotation);
    float s = sin(aRotation);
    vec2 world = aPosition + vec2(c * scaled.x - s * scaled.y, s * scaled.x + c * scaled.y);
    gl_Position = uViewProjection * vec4(world, 0.0, 1.0);
    vLocal = local;
    vColor = aColor;
}
)";

// Circles and rings are cut out of their quad by distance, with a one-pixel soft edge. A ring's line
// is kRingPixels wide on screen at any size.
constexpr std::string_view kFragmentShader = R"(#version 450 core
in vec2 vLocal;
in vec4 vColor;

layout(location = 1) uniform int uKind;

out vec4 oColor;

const float kRingPixels = 2.0;

void main() {
    float coverage = 1.0;
    if (uKind == 1 || uKind == 2) {
        float distance = length(vLocal);
        float edge = fwidth(distance);
        coverage = 1.0 - smoothstep(1.0 - edge, 1.0, distance);
        if (uKind == 2) {
            float inner = 1.0 - kRingPixels * edge;
            coverage *= smoothstep(inner - edge, inner, distance);
        }
        if (coverage <= 0.0) {
            discard;
        }
    }
    oColor = vec4(vColor.rgb, vColor.a * coverage);
}
)";

constexpr GLint kViewProjectionLocation = 0;
constexpr GLint kKindLocation = 1;

} // namespace

bool ShapeRenderer::Create() {
    if (!program_.Create("ShapeRenderer", kVertexShader, kFragmentShader)) {
        return false;
    }
    instances_.Create();
    vertexArray_.Create();
    vertexArray_.BindBuffer(0, instances_.Id(), sizeof(ShapeInstance), true);
    vertexArray_.Attribute(0, 0, 2, GL_FLOAT, offsetof(ShapeInstance, position));
    vertexArray_.Attribute(1, 0, 1, GL_FLOAT, offsetof(ShapeInstance, rotation));
    vertexArray_.Attribute(2, 0, 2, GL_FLOAT, offsetof(ShapeInstance, scale));
    vertexArray_.Attribute(3, 0, 4, GL_UNSIGNED_BYTE, offsetof(ShapeInstance, color), true);
    return true;
}

void ShapeRenderer::Destroy() {
    vertexArray_.Destroy();
    instances_.Destroy();
    program_.Destroy();
}

void ShapeRenderer::Draw(ShapeKind kind, std::span<const ShapeInstance> instances,
                         const View& view) {
    if (instances.empty()) {
        return;
    }
    instances_.Upload(instances);

    const pxMat4f viewProjection(view.ViewProjection());
    glProgramUniformMatrix4fv(program_.Id(), kViewProjectionLocation, 1, GL_FALSE,
                              glm::value_ptr(viewProjection));
    glProgramUniform1i(program_.Id(), kKindLocation, static_cast<GLint>(kind));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(program_.Id());
    glBindVertexArray(vertexArray_.Id());
    const bool isTriangle = kind == ShapeKind::Triangle;
    glDrawArraysInstanced(isTriangle ? GL_TRIANGLES : GL_TRIANGLE_STRIP, 0, isTriangle ? 3 : 4,
                          static_cast<GLsizei>(instances.size()));
}

void ShapeRenderer::Submit(ShapeKind kind, const ShapeInstance& instance) {
    pending_[static_cast<std::size_t>(kind)].push_back(instance);
}

void ShapeRenderer::Flush(const View& view) {
    for (std::size_t kind = 0; kind < pending_.size(); ++kind) {
        Draw(static_cast<ShapeKind>(kind), pending_[kind], view);
        pending_[kind].clear();
    }
}

std::size_t ShapeRenderer::PendingCount() const {
    std::size_t count = 0;
    for (const auto& kind : pending_) {
        count += kind.size();
    }
    return count;
}

} // namespace pixie
