#include "pixie/render/2d/LightRenderer.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <pixie/math/pxMat.hpp>

#include <cmath>
#include <string_view>

namespace pixie {

namespace {

// A light is four rows of the buffer: position, range and falloff; color and kind; direction and
// cone; layers. Point and spot lights cover the square around their range, directional lights the
// whole frame.
constexpr std::string_view kLightVertexShader = R"(#version 450 core
layout(std430, binding = 0) readonly buffer Lights { vec4 rows[]; };
layout(location = 0) uniform mat4 uViewProjection;

flat out int vLight;

void main() {
    int i = gl_InstanceID;
    vec4 a = rows[i * 4];
    uint kind = floatBitsToUint(rows[i * 4 + 1].w);
    vec2 corner = vec2((gl_VertexID & 1) == 0 ? -1.0 : 1.0, (gl_VertexID & 2) == 0 ? -1.0 : 1.0);
    gl_Position = kind == 2u ? vec4(corner, 0.0, 1.0) : uViewProjection * vec4(a.xy + corner * a.z, 0.0, 1.0);
    vLight = i;
}
)";

constexpr std::string_view kLightFragmentShader = R"(#version 450 core
layout(std430, binding = 0) readonly buffer Lights { vec4 rows[]; };
layout(location = 1) uniform mat4 uInverseViewProjection;
layout(location = 2) uniform vec2 uSize;
layout(binding = 0) uniform sampler2D uReceivers;

flat in int vLight;
out vec4 oLight;

void main() {
    vec4 a = rows[vLight * 4];
    vec4 b = rows[vLight * 4 + 1];
    vec4 c = rows[vLight * 4 + 2];
    uint layers = floatBitsToUint(rows[vLight * 4 + 3].x);
    uint receiver = uint(texelFetch(uReceivers, ivec2(gl_FragCoord.xy), 0).r * 255.0 + 0.5);
    if ((receiver & layers) == 0u) {
        discard;
    }
    uint kind = floatBitsToUint(b.w);
    float attenuation = 1.0;
    if (kind != 2u) {
        vec4 w = uInverseViewProjection * vec4(gl_FragCoord.xy / uSize * 2.0 - 1.0, 0.0, 1.0);
        vec2 offset = w.xy / w.w - a.xy;
        float d = length(offset);
        if (d >= a.z) {
            discard;
        }
        float x = 1.0 - (d / a.z) * (d / a.z);
        attenuation = a.w > 0.0 ? pow(x, a.w) : 1.0;
        if (kind == 1u) {
            float cosine = d > 0.0 ? dot(offset / d, c.xy) : 1.0;
            attenuation *= c.w > c.z ? smoothstep(c.z, c.w, cosine) : step(c.z, cosine);
        }
    }
    oLight = vec4(b.rgb * attenuation, 0.0);
}
)";

constexpr std::string_view kCombineVertexShader = R"(#version 450 core
void main() {
    gl_Position = vec4((gl_VertexID & 1) == 0 ? -1.0 : 3.0, (gl_VertexID & 2) == 0 ? -1.0 : 3.0, 0.0, 1.0);
}
)";

// The albedo target holds sRGB-encoded values, blended as the unlit renderer blends them.
constexpr std::string_view kCombineFragmentShader = R"(#version 450 core
layout(binding = 0) uniform sampler2D uAlbedo;
layout(binding = 1) uniform sampler2D uLight;
layout(binding = 2) uniform sampler2D uReceivers;
layout(location = 0) uniform float uExposure;
layout(location = 1) uniform int uToneMapping;
layout(location = 2) uniform ivec2 uOrigin;

out vec4 oColor;

vec3 Decode(vec3 c) { return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(0.04045, c)); }
vec3 Encode(vec3 c) { return mix(c * 12.92, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, step(0.0031308, c)); }

void main() {
    ivec2 p = ivec2(gl_FragCoord.xy) - uOrigin;
    vec4 albedo = texelFetch(uAlbedo, p, 0);
    vec3 color = Decode(albedo.rgb) * texelFetch(uLight, p, 0).rgb * uExposure;
    if (uToneMapping == 1) {
        color = color * (2.51 * color + 0.03) / (color * (2.43 * color + 0.59) + 0.14);
    }
    vec3 lit = Encode(clamp(color, 0.0, 1.0));
    oColor = vec4(mix(lit, albedo.rgb, texelFetch(uReceivers, p, 0).g), albedo.a);
}
)";

constexpr GLint kViewProjectionLocation = 0;
constexpr GLint kInverseLocation = 1;
constexpr GLint kSizeLocation = 2;
constexpr GLint kExposureLocation = 0;
constexpr GLint kToneMappingLocation = 1;
constexpr GLint kOriginLocation = 2;

GLuint CreateTarget(GLenum format, FramebufferSize size) {
    GLuint texture = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);
    glTextureStorage2D(texture, 1, format, size.width, size.height);
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return texture;
}

} // namespace

float DecodeSrgb(float channel) {
    return channel <= 0.04045f ? channel / 12.92f : std::pow((channel + 0.055f) / 1.055f, 2.4f);
}

float EncodeSrgb(float linear) {
    return linear <= 0.0031308f ? linear * 12.92f : 1.055f * std::pow(linear, 1.0f / 2.4f) - 0.055f;
}

bool LightRenderer::Create() {
    if (!lightProgram_.Create("LightRenderer", kLightVertexShader, kLightFragmentShader) ||
        !combineProgram_.Create("LightRenderer combine", kCombineVertexShader, kCombineFragmentShader)) {
        return false;
    }
    lightBuffer_.Create();
    glCreateVertexArrays(1, &vertexArray_);
    glCreateFramebuffers(1, &worldFramebuffer_);
    glCreateFramebuffers(1, &lightFramebuffer_);
    return true;
}

void LightRenderer::Destroy() {
    DestroyTargets();
    if (lightFramebuffer_ != 0) {
        glDeleteFramebuffers(1, &lightFramebuffer_);
        lightFramebuffer_ = 0;
    }
    if (worldFramebuffer_ != 0) {
        glDeleteFramebuffers(1, &worldFramebuffer_);
        worldFramebuffer_ = 0;
    }
    if (vertexArray_ != 0) {
        glDeleteVertexArrays(1, &vertexArray_);
        vertexArray_ = 0;
    }
    lightBuffer_.Destroy();
    combineProgram_.Destroy();
    lightProgram_.Destroy();
}

void LightRenderer::DestroyTargets() {
    for (Target* target : {&albedo_, &receivers_, &light_}) {
        if (target->texture != 0) {
            glDeleteTextures(1, &target->texture);
            target->texture = 0;
        }
    }
    size_ = {};
}

bool LightRenderer::Resize(FramebufferSize size) {
    if (albedo_.texture != 0 && size.width == size_.width && size.height == size_.height) {
        return true;
    }
    DestroyTargets();
    if (size.width <= 0 || size.height <= 0) {
        return false;
    }
    albedo_ = {CreateTarget(GL_RGBA8, size), GL_RGBA8};
    receivers_ = {CreateTarget(GL_RGBA8, size), GL_RGBA8};
    light_ = {CreateTarget(GL_RGBA16F, size), GL_RGBA16F};
    glNamedFramebufferTexture(worldFramebuffer_, GL_COLOR_ATTACHMENT0, albedo_.texture, 0);
    glNamedFramebufferTexture(worldFramebuffer_, GL_COLOR_ATTACHMENT1, receivers_.texture, 0);
    glNamedFramebufferTexture(lightFramebuffer_, GL_COLOR_ATTACHMENT0, light_.texture, 0);
    size_ = size;
    return true;
}

void LightRenderer::BeginWorld(FramebufferSize size, pxColorRgba background) {
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &outputFramebuffer_);
    glGetIntegerv(GL_VIEWPORT, outputViewport_.data());
    if (!Resize(size)) {
        return;
    }
    const std::array<GLenum, 2> both{GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glNamedFramebufferDrawBuffers(worldFramebuffer_, 2, both.data());
    const std::array<GLfloat, 4> clear{background.r / 255.0f, background.g / 255.0f, background.b / 255.0f, 1.0f};
    const std::array<GLfloat, 4> everyLayerLit{1.0f, 0.0f, 0.0f, 0.0f};
    glClearNamedFramebufferfv(worldFramebuffer_, GL_COLOR, 0, clear.data());
    glClearNamedFramebufferfv(worldFramebuffer_, GL_COLOR, 1, everyLayerLit.data());
    glNamedFramebufferDrawBuffer(worldFramebuffer_, GL_COLOR_ATTACHMENT0);
    glBindFramebuffer(GL_FRAMEBUFFER, worldFramebuffer_);
    glViewport(0, 0, size.width, size.height);
}

void LightRenderer::BeginReceivers() {
    glNamedFramebufferDrawBuffer(worldFramebuffer_, GL_COLOR_ATTACHMENT1);
}

void LightRenderer::Finish(const View& view, const LightingParameters& parameters,
                           std::span<const LightInstance> lights) {
    lastLightCount_ = 0;
    if (albedo_.texture == 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(outputFramebuffer_));
        return;
    }
    glNamedFramebufferDrawBuffer(worldFramebuffer_, GL_COLOR_ATTACHMENT0);
    glBindFramebuffer(GL_FRAMEBUFFER, lightFramebuffer_);
    glViewport(0, 0, size_.width, size_.height);
    const std::array<GLfloat, 4> ambient{parameters.ambient[0], parameters.ambient[1], parameters.ambient[2], 0.0f};
    glClearNamedFramebufferfv(lightFramebuffer_, GL_COLOR, 0, ambient.data());
    glBindVertexArray(vertexArray_);
    if (!lights.empty()) {
        lightBuffer_.Upload(lights);
        const pxMat4f viewProjection(view.ViewProjection());
        const pxMat4f inverse(pxInverse(view.ViewProjection()));
        const GLuint program = lightProgram_.Id();
        glProgramUniformMatrix4fv(program, kViewProjectionLocation, 1, GL_FALSE, glm::value_ptr(viewProjection));
        glProgramUniformMatrix4fv(program, kInverseLocation, 1, GL_FALSE, glm::value_ptr(inverse));
        glProgramUniform2f(program, kSizeLocation, static_cast<float>(size_.width), static_cast<float>(size_.height));
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glUseProgram(program);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightBuffer_.Id());
        glBindTextureUnit(0, receivers_.texture);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, static_cast<GLsizei>(lights.size()));
        lastLightCount_ = static_cast<int>(lights.size());
    }

    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(outputFramebuffer_));
    glViewport(outputViewport_[0], outputViewport_[1], outputViewport_[2], outputViewport_[3]);
    glDisable(GL_BLEND);
    const GLuint combine = combineProgram_.Id();
    glProgramUniform1f(combine, kExposureLocation, parameters.exposure);
    glProgramUniform1i(combine, kToneMappingLocation, parameters.toneMapping == ToneMapping::Aces ? 1 : 0);
    glProgramUniform2i(combine, kOriginLocation, outputViewport_[0], outputViewport_[1]);
    glUseProgram(combine);
    glBindTextureUnit(0, albedo_.texture);
    glBindTextureUnit(1, light_.texture);
    glBindTextureUnit(2, receivers_.texture);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    // Leave the usual blending, and texture units 1 and 2 free.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTextureUnit(1, 0);
    glBindTextureUnit(2, 0);
}

} // namespace pixie
