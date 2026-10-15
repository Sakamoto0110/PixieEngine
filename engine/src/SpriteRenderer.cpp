#include "pixie/SpriteRenderer.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cstddef>
#include <span>
#include <string_view>

namespace pixie {

namespace {

// The four corners come from gl_VertexID, as a strip: (0, 0), (1, 0), (0, 1), (1, 1) of the
// rectangle, top-left first. Only per-instance data lives in a buffer.
constexpr std::string_view kVertexShader = R"(#version 450 core
layout(location = 0) in vec2 aPosition;
layout(location = 1) in float aRotation;
layout(location = 2) in vec2 aSize;
layout(location = 3) in vec2 aPivot;
layout(location = 4) in vec4 aUv;
layout(location = 5) in vec4 aColor;

layout(location = 0) uniform mat4 uViewProjection;

out vec2 vUv;
out vec4 vColor;

void main() {
    vec2 corner = vec2(gl_VertexID & 1, (gl_VertexID >> 1) & 1);
    vec2 local = (corner - aPivot) * aSize;
    float c = cos(aRotation);
    float s = sin(aRotation);
    vec2 world = aPosition + vec2(c * local.x - s * local.y, s * local.x + c * local.y);
    gl_Position = uViewProjection * vec4(world, 0.0, 1.0);
    vUv = mix(aUv.xy, aUv.zw, corner);
    vColor = vec4(aColor.rgb * aColor.a, aColor.a);
}
)";

// The texture's alpha is premultiplied, and so is the tint, so the result blends with ONE.
constexpr std::string_view kFragmentShader = R"(#version 450 core
in vec2 vUv;
in vec4 vColor;

layout(binding = 0) uniform sampler2D uImage;

out vec4 oColor;

void main() {
    oColor = texture(uImage, vUv) * vColor;
}
)";

constexpr GLint kViewProjectionLocation = 0;

} // namespace

bool SpriteRenderer::Create() {
    if (!program_.Create("SpriteRenderer", kVertexShader, kFragmentShader)) {
        return false;
    }
    instances_.Create();
    vertexArray_.Create();
    vertexArray_.BindBuffer(0, instances_.Id(), sizeof(SpriteInstance), true);
    vertexArray_.Attribute(0, 0, 2, GL_FLOAT, offsetof(SpriteInstance, position));
    vertexArray_.Attribute(1, 0, 1, GL_FLOAT, offsetof(SpriteInstance, rotation));
    vertexArray_.Attribute(2, 0, 2, GL_FLOAT, offsetof(SpriteInstance, size));
    vertexArray_.Attribute(3, 0, 2, GL_FLOAT, offsetof(SpriteInstance, pivot));
    vertexArray_.Attribute(4, 0, 4, GL_FLOAT, offsetof(SpriteInstance, uv));
    vertexArray_.Attribute(5, 0, 4, GL_UNSIGNED_BYTE, offsetof(SpriteInstance, color), true);

    glCreateSamplers(static_cast<GLsizei>(samplers_.size()), samplers_.data());
    for (const unsigned sampler : samplers_) {
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    const unsigned smooth = samplers_[static_cast<std::size_t>(SpriteFilter::Smooth)];
    glSamplerParameteri(smooth, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(smooth, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    const unsigned pixel = samplers_[static_cast<std::size_t>(SpriteFilter::Pixel)];
    glSamplerParameteri(pixel, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glSamplerParameteri(pixel, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return true;
}

void SpriteRenderer::Destroy() {
    if (samplers_[0] != 0) {
        glDeleteSamplers(static_cast<GLsizei>(samplers_.size()), samplers_.data());
        samplers_ = {};
    }
    vertexArray_.Destroy();
    instances_.Destroy();
    program_.Destroy();
    pending_.clear();
}

void SpriteRenderer::Submit(unsigned texture, SpriteFilter filter, int layer, const SpriteInstance& instance) {
    pending_.push_back({layer, texture, filter, instance});
}

void SpriteRenderer::Flush(const Camera2D& camera) {
    lastDrawCalls_ = 0;
    if (pending_.empty()) {
        return;
    }
    std::stable_sort(pending_.begin(), pending_.end(),
                     [](const Pending& a, const Pending& b) { return a.layer < b.layer; });
    ordered_.clear();
    for (const Pending& sprite : pending_) {
        ordered_.push_back(sprite.instance);
    }
    instances_.Upload(std::span<const SpriteInstance>(ordered_));

    const pxMat4f viewProjection = camera.ViewProjection();
    glProgramUniformMatrix4fv(program_.Id(), kViewProjectionLocation, 1, GL_FALSE, glm::value_ptr(viewProjection));
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(program_.Id());
    glBindVertexArray(vertexArray_.Id());

    std::size_t first = 0;
    while (first < pending_.size()) {
        std::size_t end = first + 1;
        while (end < pending_.size() && pending_[end].texture == pending_[first].texture &&
               pending_[end].filter == pending_[first].filter) {
            ++end;
        }
        glBindTextureUnit(0, pending_[first].texture);
        glBindSampler(0, samplers_[static_cast<std::size_t>(pending_[first].filter)]);
        glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, static_cast<GLsizei>(end - first),
                                          static_cast<GLuint>(first));
        ++lastDrawCalls_;
        first = end;
    }

    // Leave texture unit 0 as others expect it: no sampler, and the usual blending.
    glBindSampler(0, 0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    pending_.clear();
}

} // namespace pixie
