#include "pixie/render/2d/SpriteRenderer.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cstddef>
#include <span>
#include <string>
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

// The lighting's receiver pass (LightRenderer): the instance color carries a receiver instead of a
// tint, red its light layers, green whether it is unlit, blue the sprite's own alpha. A pixel at
// least half covered replaces the receiver below it; the rest leave it.
constexpr std::string_view kReceiverFragmentShader = R"(#version 450 core
in vec2 vUv;
in vec4 vColor;

layout(binding = 0) uniform sampler2D uImage;

out vec4 oColor;

void main() {
    oColor = vec4(vColor.rg, 0.0, 1.0) * step(0.5, texture(uImage, vUv).a * vColor.b);
}
)";

// The lighting's emission pass: the instance color carries an emissive tint in RGB and intensity / 16
// in alpha (the vertex shader premultiplied the tint by it). The image times the tint glows as linear
// radiance; where nothing glows, the image still covers what glows below it.
constexpr std::string_view kEmissionFragmentShader = R"(#version 450 core
in vec2 vUv;
in vec4 vColor;

layout(binding = 0) uniform sampler2D uImage;

out vec4 oColor;

vec3 Decode(vec3 c) { return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(0.04045, c)); }

void main() {
    vec4 image = texture(uImage, vUv);
    vec3 straight = image.a > 0.0 ? image.rgb / image.a : vec3(0.0);
    vec3 tint = vColor.a > 0.0 ? vColor.rgb / vColor.a : vec3(0.0);
    oColor = vec4(Decode(straight) * Decode(tint) * (vColor.a * 16.0) * image.a, image.a);
}
)";

// The lighting's normal pass: the sprite's texture is its normal map (tangent space, green toward the
// image's top), turned into the world (Y down) with the sprite's rotation and mirroring. Without a
// map (green 0) it is flat and has no strength, but still covers the normals below it.
std::string NormalVertexShader() {
    std::string source(kVertexShader);
    source.replace(source.find("out vec2 vUv;"), 13, "out vec2 vUv;\nflat out float vRotation;\nflat out vec2 vMirror;");
    source.replace(source.find("    vUv = mix"), 13, "    vRotation = aRotation;\n    vMirror = vec2(aSize.x < 0.0 ? -1.0 : 1.0, aSize.y < 0.0 ? -1.0 : 1.0);\n    vUv = mix");
    return source;
}

constexpr std::string_view kNormalFragmentShader = R"(#version 450 core
in vec2 vUv;
in vec4 vColor;
flat in float vRotation;
flat in vec2 vMirror;

layout(binding = 0) uniform sampler2D uImage;

out vec4 oNormal;

void main() {
    vec4 image = texture(uImage, vUv);
    if (image.a * vColor.b < 0.5) {
        discard;
    }
    vec3 n = vec3(0.0, 0.0, 1.0);
    float strength = 0.0;
    if (vColor.g > 0.5) {
        n = normalize(image.rgb / image.a * 2.0 - 1.0);
        n.y = -n.y;
        n.xy *= vMirror;
        float c = cos(vRotation), s = sin(vRotation);
        n.xy = vec2(c * n.x - s * n.y, s * n.x + c * n.y);
        strength = vColor.r;
    }
    oNormal = vec4(n * 0.5 + 0.5, strength);
}
)";

constexpr GLint kViewProjectionLocation = 0;

} // namespace

bool SpriteRenderer::Create() {
    if (!program_.Create("SpriteRenderer", kVertexShader, kFragmentShader) ||
        !receiverProgram_.Create("SpriteRenderer receivers", kVertexShader, kReceiverFragmentShader) ||
        !emissionProgram_.Create("SpriteRenderer emission", kVertexShader, kEmissionFragmentShader) ||
        !normalProgram_.Create("SpriteRenderer normals", NormalVertexShader(), kNormalFragmentShader)) {
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
    normalProgram_.Destroy();
    emissionProgram_.Destroy();
    receiverProgram_.Destroy();
    program_.Destroy();
    pending_.clear();
}

void SpriteRenderer::Submit(unsigned texture, SpriteFilter filter, int layer, const SpriteInstance& instance) {
    pending_.push_back({layer, texture, filter, instance});
}

void SpriteRenderer::Flush(const View& view, int belowLayer) {
    lastDrawCalls_ = 0;
    // Preserve painter order among sprites on the same layer, even across partial flushes.
    std::stable_sort(pending_.begin(), pending_.end(),
                     [](const Pending& a, const Pending& b) { return a.layer < b.layer; });
    const auto drawn = static_cast<std::size_t>(
        std::partition_point(pending_.begin(), pending_.end(), [&](const Pending& p) { return p.layer < belowLayer; }) -
        pending_.begin());
    if (drawn == 0) {
        return;
    }
    ordered_.clear();
    for (std::size_t i = 0; i < drawn; ++i) {
        ordered_.push_back(pending_[i].instance);
    }
    instances_.Upload(std::span<const SpriteInstance>(ordered_));

    const GLuint program = pass_ == LightingPass::Receivers ? receiverProgram_.Id()
                          : pass_ == LightingPass::Emission ? emissionProgram_.Id()
                          : pass_ == LightingPass::Normals  ? normalProgram_.Id()
                                                            : program_.Id();
    const pxMat4f viewProjection(view.ViewProjection());
    glProgramUniformMatrix4fv(program, kViewProjectionLocation, 1, GL_FALSE, glm::value_ptr(viewProjection));
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    if (pass_ == LightingPass::Normals) {
        // Normals replace what is below them where the sprite covers; they do not blend.
        glDisable(GL_BLEND);
    }
    glUseProgram(program);
    glBindVertexArray(vertexArray_.Id());

    std::size_t first = 0;
    while (first < drawn) {
        std::size_t end = first + 1;
        while (end < drawn && pending_[end].texture == pending_[first].texture &&
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
    glEnable(GL_BLEND);
    glBindSampler(0, 0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    pending_.erase(pending_.begin(), pending_.begin() + static_cast<std::ptrdiff_t>(drawn));
}

} // namespace pixie
