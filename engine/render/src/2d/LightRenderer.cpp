#include "pixie/render/2d/LightRenderer.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <pixie/math/pxMat.hpp>
#include <pixie/math/pxVec.hpp>

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>

namespace pixie {

namespace {

// A light is four rows of the buffer: position, range and falloff; color and kind; direction and
// cone; layers, shadow layers and source size. Point and spot lights cover the square around their
// range, directional lights the whole frame.
constexpr std::string_view kLightVertexShader = R"(#version 450 core
layout(std430, binding = 0) readonly buffer Lights { vec4 rows[]; };
layout(location = 0) uniform mat4 uViewProjection;
layout(location = 3) uniform int uFirst;

flat out int vLight;

void main() {
    int i = uFirst + gl_InstanceID;
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
layout(location = 4) uniform int uShadowed;
layout(location = 7) uniform float uNormalHeight;
layout(binding = 0) uniform sampler2D uReceivers;
layout(binding = 1) uniform sampler2D uOcclusion;
layout(binding = 2) uniform sampler2D uNormals;

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
    if (uNormalHeight > 0.0) {
        // Normal maps: the light arrives from its position raised by the height (directional: from
        // its direction, raised as much as it travels), against the surface's normal; flat
        // surfaces, and those without a map, keep their light.
        vec4 normal = texelFetch(uNormals, ivec2(gl_FragCoord.xy), 0);
        if (normal.a > 0.0) {
            vec3 toLight;
            if (kind == 2u) {
                toLight = vec3(-c.xy, 1.0);
            } else {
                vec4 w = uInverseViewProjection * vec4(gl_FragCoord.xy / uSize * 2.0 - 1.0, 0.0, 1.0);
                toLight = vec3(a.xy - w.xy / w.w, uNormalHeight);
            }
            vec3 l = normalize(toLight);
            float facing = clamp(max(dot(normal.rgb * 2.0 - 1.0, l), 0.0) / max(l.z, 0.05), 0.0, 2.0);
            attenuation *= mix(1.0, facing, normal.a);
        }
    }
    if (uShadowed != 0) {
        // The mask covers the frame at full or half resolution; filtered, clamped at its edges.
        attenuation *= 1.0 - clamp(texture(uOcclusion, gl_FragCoord.xy / uSize).r, 0.0, 1.0);
    }
    // Alpha counts the lights that shade the pixel, for the overdraw view.
    oLight = vec4(b.rgb * attenuation, 1.0);
}
)";

// Shadow volumes: each occluder edge facing away from the light (toward it, for an occluder that
// shadows itself) becomes a quad over its umbra and penumbra, extruded along the tangents to the
// light's source; its fragments add the fraction of the source's angular extent the edge hides.
constexpr std::string_view kShadowCommon = R"(#version 450 core
layout(std430, binding = 0) readonly buffer Lights { vec4 rows[]; };
layout(std430, binding = 1) readonly buffer Edges { vec4 edges[]; };
layout(location = 0) uniform mat4 uViewProjection;
layout(location = 1) uniform mat4 uInverseViewProjection;
layout(location = 2) uniform vec2 uSize;
layout(location = 3) uniform int uLight;
layout(location = 5) uniform float uFar;

// The side of the edge its shadow falls on: outward, or inward for an occluder that shadows itself.
vec2 ShadowSide(vec4 edge, uint flags) {
    vec2 ab = edge.zw - edge.xy;
    return (flags & 1u) != 0u ? vec2(-ab.y, ab.x) : vec2(ab.y, -ab.x);
}
)";

constexpr std::string_view kShadowVertexShader = R"(
flat out vec4 vEdge;
flat out uint vFlags;

// The direction from vertex v away from a source of radius r at c, along the tangent on the side
// of `outward`.
vec2 Outer(vec2 v, vec2 c, float r, vec2 outward) {
    vec2 radial = normalize(v - c);
    float beta = asin(min(r / max(length(v - c), 1e-3), 0.999));
    float cb = cos(beta), sb = sin(beta);
    vec2 d1 = vec2(radial.x * cb - radial.y * sb, radial.x * sb + radial.y * cb);
    vec2 d2 = vec2(radial.x * cb + radial.y * sb, -radial.x * sb + radial.y * cb);
    return dot(d1, outward) > dot(d2, outward) ? d1 : d2;
}

vec2 Turn(vec2 d, float angle) {
    float c = cos(angle), s = sin(angle);
    return vec2(d.x * c - d.y * s, d.x * s + d.y * c);
}

void main() {
    vec4 l0 = rows[uLight * 4];
    uint kind = floatBitsToUint(rows[uLight * 4 + 1].w);
    vec2 direction = rows[uLight * 4 + 2].xy;
    vec4 l3 = rows[uLight * 4 + 3];
    uint shadowLayers = floatBitsToUint(l3.y);
    float source = l3.z;
    vec4 e = edges[gl_InstanceID * 2];
    uvec2 info = floatBitsToUint(edges[gl_InstanceID * 2 + 1].xy);
    vEdge = e;
    vFlags = info.y;
    gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
    if ((info.x & shadowLayers) == 0u) {
        return;
    }
    vec2 a = e.xy, b = e.zw, ab = b - a;
    vec2 side = ShadowSide(e, info.y);
    vec2 along = normalize(ab);
    vec2 corner;
    if (kind == 2u) {
        if (dot(side, direction) <= 0.0) {
            return;
        }
        // Outward on each end: the directions within the source's angle that open the shadow.
        vec2 da = Turn(direction, dot(Turn(direction, source), -along) > dot(Turn(direction, -source), -along) ? source : -source);
        vec2 db = Turn(direction, dot(Turn(direction, source), along) > dot(Turn(direction, -source), along) ? source : -source);
        corner = gl_VertexID == 0 ? a : gl_VertexID == 1 ? b : gl_VertexID == 2 ? a + uFar * da : b + uFar * db;
    } else {
        vec2 c = l0.xy;
        vec2 pa = c - a;
        float h = clamp(dot(pa, ab) / dot(ab, ab), 0.0, 1.0);
        if (dot(side, 0.5 * (a + b) - c) <= 0.0 || length(pa - ab * h) > l0.z) {
            return;
        }
        float far = l0.z + max(length(a - c), length(b - c));
        corner = gl_VertexID == 0 ? a : gl_VertexID == 1 ? b
               : gl_VertexID == 2 ? a + far * Outer(a, c, source, -along) : b + far * Outer(b, c, source, along);
    }
    gl_Position = uViewProjection * vec4(corner, 0.0, 1.0);
}
)";

constexpr std::string_view kShadowFragmentShader = R"(
flat in vec4 vEdge;
flat in uint vFlags;
out vec4 oOcclusion;

#define PI 3.14159265358979
float WrapPi(float x) { return x - 2.0 * PI * floor((x + PI) / (2.0 * PI)); }

void main() {
    vec4 l0 = rows[uLight * 4];
    uint kind = floatBitsToUint(rows[uLight * 4 + 1].w);
    vec2 direction = rows[uLight * 4 + 2].xy;
    float source = rows[uLight * 4 + 3].z;
    vec4 w = uInverseViewProjection * vec4(gl_FragCoord.xy / uSize * 2.0 - 1.0, 0.0, 1.0);
    vec2 p = w.xy / w.w;
    vec2 a = vEdge.xy, b = vEdge.zw;
    // Only what lies past the edge's line, on its shadow side, is shadowed by it.
    if (dot(ShadowSide(vEdge, vFlags), p - a) <= 0.0) {
        discard;
    }
    float base, alpha;
    if (kind == 2u) {
        base = atan(-direction.y, -direction.x);
        alpha = max(source, 1e-4);
    } else {
        vec2 toLight = l0.xy - p;
        float d = length(toLight);
        if (d <= source || d >= l0.z) {
            discard;
        }
        base = atan(toLight.y, toLight.x);
        alpha = max(asin(clamp(source / d, 0.0, 1.0)), 1e-4);
    }
    float ta = WrapPi(atan(a.y - p.y, a.x - p.x) - base);
    float tb = WrapPi(atan(b.y - p.y, b.x - p.x) - base);
    float lo = min(ta, tb), hi = max(ta, tb);
    if (hi - lo > PI) {
        discard;
    }
    oOcclusion = vec4(max(0.0, min(hi, alpha) - max(lo, -alpha)) / (2.0 * alpha));
}
)";

constexpr std::string_view kCombineVertexShader = R"(#version 450 core
void main() {
    gl_Position = vec4((gl_VertexID & 1) == 0 ? -1.0 : 3.0, (gl_VertexID & 2) == 0 ? -1.0 : 3.0, 0.0, 1.0);
}
)";

// The albedo target holds sRGB-encoded values, blended as the unlit renderer blends them. The
// optional effects (ambient occlusion, fog, bloom) are off at their zero values, which leaves the
// arithmetic of a plain lit frame as it is. Stage 1 writes the linear radiance alone, for bloom.
constexpr std::string_view kCombineFragmentShader = R"(#version 450 core
layout(binding = 0) uniform sampler2D uAlbedo;
layout(binding = 1) uniform sampler2D uLight;
layout(binding = 2) uniform sampler2D uReceivers;
layout(binding = 3) uniform sampler2D uEmission;
layout(binding = 4) uniform sampler2D uAmbientOcclusion;
layout(binding = 5) uniform sampler2D uBloom;
layout(location = 0) uniform float uExposure;
layout(location = 1) uniform int uToneMapping;
layout(location = 2) uniform ivec2 uOrigin;
layout(location = 3) uniform int uStage;
layout(location = 4) uniform vec3 uAmbient;
layout(location = 5) uniform float uOcclusionStrength;
layout(location = 6) uniform vec4 uFog;
layout(location = 7) uniform vec2 uFogRange;
layout(location = 8) uniform mat4 uInverseViewProjection;
layout(location = 9) uniform vec2 uSize;
layout(location = 10) uniform float uBloomIntensity;
layout(location = 11) uniform int uView;

out vec4 oColor;

vec3 Decode(vec3 c) { return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(0.04045, c)); }
vec3 Encode(vec3 c) { return mix(c * 12.92, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, step(0.0031308, c)); }

void main() {
    ivec2 p = ivec2(gl_FragCoord.xy) - uOrigin;
    vec4 albedo = texelFetch(uAlbedo, p, 0);
    vec4 accumulated = texelFetch(uLight, p, 0);
    if (uView == 2) {
        float n = accumulated.a;
        vec3 heat = n < 4.0 ? mix(vec3(0.0, 0.0, 1.0), vec3(0.0, 1.0, 0.0), (n - 1.0) / 3.0)
                            : mix(vec3(0.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0), min((n - 4.0) / 4.0, 1.0));
        oColor = vec4(n < 0.5 ? vec3(0.0) : heat, 1.0);
        return;
    }
    vec3 light = accumulated.rgb;
    if (uOcclusionStrength > 0.0) {
        // Ambient occlusion darkens only the ambient part of the light.
        light = max(light - uAmbient * uOcclusionStrength * texelFetch(uAmbientOcclusion, p, 0).r, vec3(0.0));
    }
    vec3 color = uView == 1 ? light * uExposure : (Decode(albedo.rgb) * light + texelFetch(uEmission, p, 0).rgb) * uExposure;
    if (uView == 1) {
        // The light a white surface would receive: no emission, fog, bloom or unlit receivers.
        if (uToneMapping == 1) {
            color = color * (2.51 * color + 0.03) / (color * (2.43 * color + 0.59) + 0.14);
        }
        oColor = vec4(Encode(clamp(color, 0.0, 1.0)), 1.0);
        return;
    }
    if (uFog.a > 0.0) {
        vec4 w = uInverseViewProjection * vec4((vec2(p) + 0.5) / uSize * 2.0 - 1.0, 0.0, 1.0);
        float amount = uFog.a * smoothstep(uFogRange.x, uFogRange.y, w.y / w.w);
        color = mix(color, uFog.rgb * uExposure, amount);
    }
    if (uStage == 1) {
        oColor = vec4(color, 1.0);
        return;
    }
    if (uBloomIntensity > 0.0) {
        color += texture(uBloom, (vec2(p) + 0.5) / uSize).rgb * uBloomIntensity;
    }
    if (uToneMapping == 1) {
        color = color * (2.51 * color + 0.03) / (color * (2.43 * color + 0.59) + 0.14);
    }
    vec3 lit = Encode(clamp(color, 0.0, 1.0));
    oColor = vec4(mix(lit, albedo.rgb, texelFetch(uReceivers, p, 0).g), albedo.a);
}
)";

// Ambient occlusion in 2D: around each occluder edge, on its outer side, a fragment keeps the
// strongest 1 - smoothstep(0, radius, distance to the edge); max blending merges the edges.
constexpr std::string_view kOcclusionVertexShader = R"(#version 450 core
layout(std430, binding = 1) readonly buffer Edges { vec4 edges[]; };
layout(location = 0) uniform mat4 uViewProjection;
layout(location = 3) uniform float uRadius;
flat out vec4 vEdge;
void main() {
    vec4 e = edges[gl_InstanceID * 2];
    vEdge = e;
    vec2 a = e.xy, b = e.zw;
    vec2 along = normalize(b - a) * uRadius;
    vec2 across = vec2(along.y, -along.x);
    vec2 corner = (gl_VertexID & 1) == 0 ? a - along : b + along;
    corner += (gl_VertexID & 2) == 0 ? -across : across;
    gl_Position = uViewProjection * vec4(corner, 0.0, 1.0);
}
)";

constexpr std::string_view kOcclusionFragmentShader = R"(#version 450 core
layout(location = 1) uniform mat4 uInverseViewProjection;
layout(location = 2) uniform vec2 uSize;
layout(location = 3) uniform float uRadius;
flat in vec4 vEdge;
out vec4 oOcclusion;
void main() {
    vec4 w = uInverseViewProjection * vec4(gl_FragCoord.xy / uSize * 2.0 - 1.0, 0.0, 1.0);
    vec2 p = w.xy / w.w;
    vec2 a = vEdge.xy, ab = vEdge.zw - vEdge.xy;
    if (dot(vec2(ab.y, -ab.x), p - a) <= 0.0) {
        discard;
    }
    float d = length(p - a - ab * clamp(dot(p - a, ab) / dot(ab, ab), 0.0, 1.0));
    oOcclusion = vec4(1.0 - smoothstep(0.0, uRadius, d));
}
)";

// Bloom: the radiance above a threshold, at half resolution (filtered down), blurred along one
// axis per pass with a 9-tap Gaussian.
constexpr std::string_view kBrightFragmentShader = R"(#version 450 core
layout(binding = 0) uniform sampler2D uScene;
layout(location = 0) uniform vec2 uSize;
layout(location = 1) uniform float uThreshold;
out vec4 oColor;
void main() {
    vec3 c = texture(uScene, gl_FragCoord.xy / uSize).rgb;
    oColor = vec4(max(c - vec3(uThreshold), vec3(0.0)), 1.0);
}
)";

constexpr std::string_view kBlurFragmentShader = R"(#version 450 core
layout(binding = 0) uniform sampler2D uSource;
layout(location = 0) uniform vec2 uSize;
layout(location = 1) uniform vec2 uStep;
out vec4 oColor;
const float kWeights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
void main() {
    vec2 uv = gl_FragCoord.xy / uSize;
    vec3 sum = texture(uSource, uv).rgb * kWeights[0];
    for (int i = 1; i < 5; ++i) {
        sum += texture(uSource, uv + uStep * float(i)).rgb * kWeights[i];
        sum += texture(uSource, uv - uStep * float(i)).rgb * kWeights[i];
    }
    oColor = vec4(sum, 1.0);
}
)";
constexpr GLint kViewProjectionLocation = 0;
constexpr GLint kInverseLocation = 1;
constexpr GLint kSizeLocation = 2;
constexpr GLint kFirstLocation = 3;
constexpr GLint kLightLocation = 3;
constexpr GLint kShadowedLocation = 4;
constexpr GLint kFarLocation = 5;
constexpr GLint kExposureLocation = 0;
constexpr GLint kToneMappingLocation = 1;
constexpr GLint kOriginLocation = 2;
constexpr GLint kStageLocation = 3;
constexpr GLint kAmbientLocation = 4;
constexpr GLint kOcclusionStrengthLocation = 5;
constexpr GLint kFogLocation = 6;
constexpr GLint kFogRangeLocation = 7;
constexpr GLint kCombineInverseLocation = 8;
constexpr GLint kCombineSizeLocation = 9;
constexpr GLint kBloomIntensityLocation = 10;
constexpr GLint kViewLocation = 11;
constexpr GLint kRadiusLocation = 3;
constexpr GLint kNormalHeightLocation = 7;

GLuint CreateTarget(GLenum format, FramebufferSize size, GLint filter = GL_NEAREST) {
    GLuint texture = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);
    glTextureStorage2D(texture, 1, format, size.width, size.height);
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, filter);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, filter);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return texture;
}

bool Shadowed(const LightInstance& light) {
    return light.shadowLayers != 0u;
}

// The pixels a light can reach, as a scissor box clamped to `size`; the whole frame for a
// directional light.
std::array<GLint, 4> PixelBox(const View& view, const LightInstance& light, FramebufferSize size) {
    if (light.kind == LightKind::Directional) {
        return {0, 0, size.width, size.height};
    }
    double x0 = 1e30, y0 = 1e30, x1 = -1e30, y1 = -1e30;
    for (const double sx : {-1.0, 1.0}) {
        for (const double sy : {-1.0, 1.0}) {
            const pxVec4 clip = view.ViewProjection() * pxVec4(light.x + sx * light.range, light.y + sy * light.range, 0.0, 1.0);
            const double px = (clip.x / clip.w + 1.0) * 0.5 * size.width;
            const double py = (clip.y / clip.w + 1.0) * 0.5 * size.height;
            x0 = std::min(x0, px);
            y0 = std::min(y0, py);
            x1 = std::max(x1, px);
            y1 = std::max(y1, py);
        }
    }
    const GLint left = std::clamp(static_cast<GLint>(std::floor(x0)), 0, size.width);
    const GLint bottom = std::clamp(static_cast<GLint>(std::floor(y0)), 0, size.height);
    const GLint right = std::clamp(static_cast<GLint>(std::ceil(x1)), 0, size.width);
    const GLint top = std::clamp(static_cast<GLint>(std::ceil(y1)), 0, size.height);
    return {left, bottom, right - left, top - bottom};
}

} // namespace

float DecodeSrgb(float channel) {
    return channel <= 0.04045f ? channel / 12.92f : std::pow((channel + 0.055f) / 1.055f, 2.4f);
}

float EncodeSrgb(float linear) {
    return linear <= 0.0031308f ? linear * 12.92f : 1.055f * std::pow(linear, 1.0f / 2.4f) - 0.055f;
}

bool LightRenderer::Create() {
    const std::string shadowVertex = std::string(kShadowCommon) + std::string(kShadowVertexShader);
    const std::string shadowFragment = std::string(kShadowCommon) + std::string(kShadowFragmentShader);
    if (!lightProgram_.Create("LightRenderer", kLightVertexShader, kLightFragmentShader) ||
        !shadowProgram_.Create("LightRenderer shadows", shadowVertex, shadowFragment) ||
        !combineProgram_.Create("LightRenderer combine", kCombineVertexShader, kCombineFragmentShader) ||
        !occlusionProgram_.Create("LightRenderer occlusion", kOcclusionVertexShader, kOcclusionFragmentShader) ||
        !brightProgram_.Create("LightRenderer bloom", kCombineVertexShader, kBrightFragmentShader) ||
        !blurProgram_.Create("LightRenderer blur", kCombineVertexShader, kBlurFragmentShader)) {
        return false;
    }
    lightBuffer_.Create();
    edgeBuffer_.Create();
    glCreateVertexArrays(1, &vertexArray_);
    glCreateFramebuffers(1, &worldFramebuffer_);
    glCreateFramebuffers(1, &lightFramebuffer_);
    glCreateFramebuffers(1, &maskFramebuffer_);
    glCreateFramebuffers(1, &halfMaskFramebuffer_);
    glCreateFramebuffers(1, &effectFramebuffer_);
    for (auto& frame : queries_) {
        glCreateQueries(GL_TIMESTAMP, static_cast<GLsizei>(frame.size()), frame.data());
    }
    issued_ = {};
    return true;
}

void LightRenderer::Destroy() {
    DestroyTargets();
    for (auto& frame : queries_) {
        if (frame[0] != 0) {
            glDeleteQueries(static_cast<GLsizei>(frame.size()), frame.data());
            frame = {};
        }
    }
    for (GLuint* framebuffer : {&effectFramebuffer_, &halfMaskFramebuffer_, &maskFramebuffer_, &lightFramebuffer_, &worldFramebuffer_}) {
        if (*framebuffer != 0) {
            glDeleteFramebuffers(1, framebuffer);
            *framebuffer = 0;
        }
    }
    if (vertexArray_ != 0) {
        glDeleteVertexArrays(1, &vertexArray_);
        vertexArray_ = 0;
    }
    edgeBuffer_.Destroy();
    lightBuffer_.Destroy();
    blurProgram_.Destroy();
    brightProgram_.Destroy();
    occlusionProgram_.Destroy();
    combineProgram_.Destroy();
    shadowProgram_.Destroy();
    lightProgram_.Destroy();
}

void LightRenderer::DestroyTargets() {
    for (Target* target : {&albedo_, &receivers_, &emission_, &normals_, &light_, &mask_, &halfMask_, &occlusion_, &scene_, &bloom_[0], &bloom_[1]}) {
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
    emission_ = {CreateTarget(GL_RGBA16F, size), GL_RGBA16F};
    normals_ = {CreateTarget(GL_RGBA8, size), GL_RGBA8};
    light_ = {CreateTarget(GL_RGBA16F, size), GL_RGBA16F};
    // Filtered: a half-resolution mask is sampled between its texels.
    mask_ = {CreateTarget(GL_R32F, size, GL_LINEAR), GL_R32F};
    glNamedFramebufferTexture(worldFramebuffer_, GL_COLOR_ATTACHMENT0, albedo_.texture, 0);
    glNamedFramebufferTexture(worldFramebuffer_, GL_COLOR_ATTACHMENT1, receivers_.texture, 0);
    glNamedFramebufferTexture(worldFramebuffer_, GL_COLOR_ATTACHMENT2, emission_.texture, 0);
    glNamedFramebufferTexture(worldFramebuffer_, GL_COLOR_ATTACHMENT3, normals_.texture, 0);
    glNamedFramebufferTexture(lightFramebuffer_, GL_COLOR_ATTACHMENT0, light_.texture, 0);
    glNamedFramebufferTexture(maskFramebuffer_, GL_COLOR_ATTACHMENT0, mask_.texture, 0);
    halfMask_ = {CreateTarget(GL_R32F, {(size.width + 1) / 2, (size.height + 1) / 2}, GL_LINEAR), GL_R32F};
    glNamedFramebufferTexture(halfMaskFramebuffer_, GL_COLOR_ATTACHMENT0, halfMask_.texture, 0);
    occlusion_ = {CreateTarget(GL_R16F, size), GL_R16F};
    scene_ = {CreateTarget(GL_RGBA16F, size, GL_LINEAR), GL_RGBA16F};
    for (Target& bloom : bloom_) {
        bloom = {CreateTarget(GL_RGBA16F, {(size.width + 1) / 2, (size.height + 1) / 2}, GL_LINEAR), GL_RGBA16F};
    }
    size_ = size;
    return true;
}

void LightRenderer::BeginWorld(FramebufferSize size, pxColorRgba background) {
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &outputFramebuffer_);
    glGetIntegerv(GL_VIEWPORT, outputViewport_.data());
    if (!Resize(size)) {
        return;
    }
    const std::array<GLenum, 4> all{GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
    glNamedFramebufferDrawBuffers(worldFramebuffer_, 4, all.data());
    const std::array<GLfloat, 4> clear{background.r / 255.0f, background.g / 255.0f, background.b / 255.0f, 1.0f};
    const std::array<GLfloat, 4> everyLayerLit{1.0f, 0.0f, 0.0f, 0.0f};
    glClearNamedFramebufferfv(worldFramebuffer_, GL_COLOR, 0, clear.data());
    glClearNamedFramebufferfv(worldFramebuffer_, GL_COLOR, 1, everyLayerLit.data());
    const std::array<GLfloat, 4> dark{0.0f, 0.0f, 0.0f, 0.0f};
    glClearNamedFramebufferfv(worldFramebuffer_, GL_COLOR, 2, dark.data());
    const std::array<GLfloat, 4> flat{0.5f, 0.5f, 1.0f, 0.0f};
    glClearNamedFramebufferfv(worldFramebuffer_, GL_COLOR, 3, flat.data());
    glNamedFramebufferDrawBuffer(worldFramebuffer_, GL_COLOR_ATTACHMENT0);
    glBindFramebuffer(GL_FRAMEBUFFER, worldFramebuffer_);
    glViewport(0, 0, size.width, size.height);
}

void LightRenderer::BeginReceivers() {
    glNamedFramebufferDrawBuffer(worldFramebuffer_, GL_COLOR_ATTACHMENT1);
}

void LightRenderer::BeginEmission() {
    glNamedFramebufferDrawBuffer(worldFramebuffer_, GL_COLOR_ATTACHMENT2);
}

void LightRenderer::BeginNormals() {
    glNamedFramebufferDrawBuffer(worldFramebuffer_, GL_COLOR_ATTACHMENT3);
}

void LightRenderer::Finish(const View& view, const LightingParameters& parameters,
                           std::span<const LightInstance> lights, std::span<const OccluderEdge> occluders) {
    lastLightCount_ = 0;
    lastShadowedCount_ = 0;
    if (albedo_.texture == 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(outputFramebuffer_));
        return;
    }
    // Read the oldest frame's timestamps if they have arrived; then this frame reuses its queries.
    std::array<GLuint, 4>& queries = queries_[static_cast<std::size_t>(queryFrame_)];
    if (issued_[static_cast<std::size_t>(queryFrame_)]) {
        GLint available = 0;
        glGetQueryObjectiv(queries[3], GL_QUERY_RESULT_AVAILABLE, &available);
        if (available != 0) {
            std::array<GLuint64, 4> stamps{};
            for (std::size_t i = 0; i < stamps.size(); ++i) {
                glGetQueryObjectui64v(queries[i], GL_QUERY_RESULT, &stamps[i]);
            }
            gpuTimes_.lights = static_cast<double>(stamps[1] - stamps[0]) / 1.0e6;
            gpuTimes_.shadows = static_cast<double>(stamps[2] - stamps[1]) / 1.0e6;
            gpuTimes_.combine = static_cast<double>(stamps[3] - stamps[2]) / 1.0e6;
            ++gpuTimes_.sample;
        }
    }
    issued_[static_cast<std::size_t>(queryFrame_)] = true;
    queryFrame_ = (queryFrame_ + 1) % kQueryFrames;
    glQueryCounter(queries[0], GL_TIMESTAMP);
    glNamedFramebufferDrawBuffer(worldFramebuffer_, GL_COLOR_ATTACHMENT0);
    glBindFramebuffer(GL_FRAMEBUFFER, lightFramebuffer_);
    glViewport(0, 0, size_.width, size_.height);
    const std::array<GLfloat, 4> ambient{parameters.ambient[0], parameters.ambient[1], parameters.ambient[2], 0.0f};
    glClearNamedFramebufferfv(lightFramebuffer_, GL_COLOR, 0, ambient.data());
    glBindVertexArray(vertexArray_);
    if (!lights.empty()) {
        // Unshadowed lights first, all in one draw; then each shadowed one with its own mask.
        ordered_.assign(lights.begin(), lights.end());
        if (occluders.empty()) {
            for (LightInstance& light : ordered_) {
                light.shadowLayers = 0u;
            }
        }
        const auto firstShadowed = std::stable_partition(ordered_.begin(), ordered_.end(),
                                                         [](const LightInstance& light) { return !Shadowed(light); });
        const auto unshadowed = static_cast<GLsizei>(firstShadowed - ordered_.begin());
        lightBuffer_.Upload(std::span<const LightInstance>(ordered_));
        if (!occluders.empty()) {
            edgeBuffer_.Upload(occluders);
        }
        const pxMat4f viewProjection(view.ViewProjection());
        const pxMat4f inverse(pxInverse(view.ViewProjection()));
        const auto width = static_cast<float>(size_.width);
        const auto height = static_cast<float>(size_.height);
        for (const GLuint program : {lightProgram_.Id(), shadowProgram_.Id()}) {
            glProgramUniformMatrix4fv(program, kViewProjectionLocation, 1, GL_FALSE, glm::value_ptr(viewProjection));
            glProgramUniformMatrix4fv(program, kInverseLocation, 1, GL_FALSE, glm::value_ptr(inverse));
            glProgramUniform2f(program, kSizeLocation, width, height);
        }
        // Far enough past any occluder in view to leave the view for a directional light.
        const pxVec4 lowCorner = pxInverse(view.ViewProjection()) * pxVec4(-1.0, -1.0, 0.0, 1.0);
        const pxVec4 highCorner = pxInverse(view.ViewProjection()) * pxVec4(1.0, 1.0, 0.0, 1.0);
        const double diagonal = std::hypot(highCorner.x - lowCorner.x, highCorner.y - lowCorner.y);
        glProgramUniform1f(shadowProgram_.Id(), kFarLocation, static_cast<float>(4.0 * diagonal));
        // The mask covers the frame at full or half resolution, from its lower-left corner.
        const FramebufferSize mask = parameters.halfResolutionShadows
                                         ? FramebufferSize{(size_.width + 1) / 2, (size_.height + 1) / 2}
                                         : size_;
        glProgramUniform2f(shadowProgram_.Id(), kSizeLocation, static_cast<float>(mask.width), static_cast<float>(mask.height));
        const GLuint maskFramebuffer = parameters.halfResolutionShadows ? halfMaskFramebuffer_ : maskFramebuffer_;
        glBindTextureUnit(1, parameters.halfResolutionShadows ? halfMask_.texture : mask_.texture);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightBuffer_.Id());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, edgeBuffer_.Id());
        glBindTextureUnit(0, receivers_.texture);
        glBindTextureUnit(2, normals_.texture);
        glProgramUniform1f(lightProgram_.Id(), kNormalHeightLocation, parameters.normalMaps ? std::max(parameters.normalHeight, 0.001f) : 0.0f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        if (unshadowed > 0) {
            glProgramUniform1i(lightProgram_.Id(), kFirstLocation, 0);
            glProgramUniform1i(lightProgram_.Id(), kShadowedLocation, 0);
            glUseProgram(lightProgram_.Id());
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, unshadowed);
        }
        glQueryCounter(queries[1], GL_TIMESTAMP);
        const std::array<GLfloat, 4> none{0.0f, 0.0f, 0.0f, 0.0f};
        glEnable(GL_SCISSOR_TEST);
        for (auto i = static_cast<GLint>(unshadowed); i < static_cast<GLint>(ordered_.size()); ++i) {
            const std::array<GLint, 4> box = PixelBox(view, ordered_[static_cast<std::size_t>(i)], size_);
            if (box[2] <= 0 || box[3] <= 0) {
                continue;
            }
            // The mask's box, a texel wider at half resolution for the filtering.
            const GLint pad = mask.width < size_.width ? 1 : 0;
            const GLint mx0 = std::max(0, box[0] * mask.width / size_.width - pad);
            const GLint my0 = std::max(0, box[1] * mask.height / size_.height - pad);
            const GLint mx1 = std::min(mask.width, (box[0] + box[2]) * mask.width / size_.width + 1 + pad);
            const GLint my1 = std::min(mask.height, (box[1] + box[3]) * mask.height / size_.height + 1 + pad);
            glBindFramebuffer(GL_FRAMEBUFFER, maskFramebuffer);
            glViewport(0, 0, mask.width, mask.height);
            glScissor(mx0, my0, mx1 - mx0, my1 - my0);
            glClearNamedFramebufferfv(maskFramebuffer, GL_COLOR, 0, none.data());
            glProgramUniform1i(shadowProgram_.Id(), kLightLocation, i);
            glUseProgram(shadowProgram_.Id());
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, static_cast<GLsizei>(occluders.size()));
            glBindFramebuffer(GL_FRAMEBUFFER, lightFramebuffer_);
            glViewport(0, 0, size_.width, size_.height);
            glScissor(box[0], box[1], box[2], box[3]);
            glProgramUniform1i(lightProgram_.Id(), kFirstLocation, i);
            glProgramUniform1i(lightProgram_.Id(), kShadowedLocation, 1);
            glUseProgram(lightProgram_.Id());
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 1);
            ++lastShadowedCount_;
        }
        glDisable(GL_SCISSOR_TEST);
        lastLightCount_ = static_cast<int>(lights.size());
    } else {
        glQueryCounter(queries[1], GL_TIMESTAMP);
    }
    const pxMat4f inverse(pxInverse(view.ViewProjection()));
    const auto width = static_cast<float>(size_.width);
    const auto height = static_cast<float>(size_.height);
    if (parameters.occlusionStrength > 0.0f) {
        // Ambient occlusion around the occluders' edges, kept at its strongest per pixel.
        glNamedFramebufferTexture(effectFramebuffer_, GL_COLOR_ATTACHMENT0, occlusion_.texture, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, effectFramebuffer_);
        glViewport(0, 0, size_.width, size_.height);
        const std::array<GLfloat, 4> none{0.0f, 0.0f, 0.0f, 0.0f};
        glClearNamedFramebufferfv(effectFramebuffer_, GL_COLOR, 0, none.data());
        if (!occluders.empty()) {
            edgeBuffer_.Upload(occluders);
            const pxMat4f viewProjection(view.ViewProjection());
            const GLuint program = occlusionProgram_.Id();
            glProgramUniformMatrix4fv(program, kViewProjectionLocation, 1, GL_FALSE, glm::value_ptr(viewProjection));
            glProgramUniformMatrix4fv(program, kInverseLocation, 1, GL_FALSE, glm::value_ptr(inverse));
            glProgramUniform2f(program, kSizeLocation, width, height);
            glProgramUniform1f(program, kRadiusLocation, std::max(parameters.occlusionRadius, 0.001f));
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, edgeBuffer_.Id());
            glEnable(GL_BLEND);
            glBlendEquation(GL_MAX);
            glBlendFunc(GL_ONE, GL_ONE);
            glUseProgram(program);
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, static_cast<GLsizei>(occluders.size()));
            glBlendEquation(GL_FUNC_ADD);
        }
    }
    glQueryCounter(queries[2], GL_TIMESTAMP);

    glDisable(GL_BLEND);
    const GLuint combine = combineProgram_.Id();
    glProgramUniform1f(combine, kExposureLocation, parameters.exposure);
    glProgramUniform1i(combine, kToneMappingLocation, parameters.toneMapping == ToneMapping::Aces ? 1 : 0);
    glProgramUniform3f(combine, kAmbientLocation, parameters.ambient[0], parameters.ambient[1], parameters.ambient[2]);
    glProgramUniform1f(combine, kOcclusionStrengthLocation, std::max(parameters.occlusionStrength, 0.0f));
    glProgramUniform4f(combine, kFogLocation, parameters.fogColor[0], parameters.fogColor[1], parameters.fogColor[2],
                       std::clamp(parameters.fogDensity, 0.0f, 1.0f));
    glProgramUniform2f(combine, kFogRangeLocation, parameters.fogTop, parameters.fogBottom);
    glProgramUniformMatrix4fv(combine, kCombineInverseLocation, 1, GL_FALSE, glm::value_ptr(inverse));
    glProgramUniform2f(combine, kCombineSizeLocation, width, height);
    glBindTextureUnit(0, albedo_.texture);
    glBindTextureUnit(1, light_.texture);
    glBindTextureUnit(2, receivers_.texture);
    glBindTextureUnit(3, emission_.texture);
    glBindTextureUnit(4, occlusion_.texture);
    glBindTextureUnit(5, bloom_[0].texture);
    const bool bloom = parameters.bloomIntensity > 0.0f;
    if (bloom) {
        // The linear radiance, then what exceeds the threshold at half size, blurred across and down.
        glNamedFramebufferTexture(effectFramebuffer_, GL_COLOR_ATTACHMENT0, scene_.texture, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, effectFramebuffer_);
        glViewport(0, 0, size_.width, size_.height);
        glProgramUniform1i(combine, kStageLocation, 1);
        glProgramUniform1i(combine, kViewLocation, 0);
        glProgramUniform2i(combine, kOriginLocation, 0, 0);
        glUseProgram(combine);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        const FramebufferSize half{(size_.width + 1) / 2, (size_.height + 1) / 2};
        const auto halfWidth = static_cast<float>(half.width);
        const auto halfHeight = static_cast<float>(half.height);
        glViewport(0, 0, half.width, half.height);
        glNamedFramebufferTexture(effectFramebuffer_, GL_COLOR_ATTACHMENT0, bloom_[0].texture, 0);
        glProgramUniform2f(brightProgram_.Id(), 0, halfWidth, halfHeight);
        glProgramUniform1f(brightProgram_.Id(), 1, parameters.bloomThreshold);
        glUseProgram(brightProgram_.Id());
        glBindTextureUnit(0, scene_.texture);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glProgramUniform2f(blurProgram_.Id(), 0, halfWidth, halfHeight);
        glUseProgram(blurProgram_.Id());
        for (const int pass : {0, 1}) {
            glNamedFramebufferTexture(effectFramebuffer_, GL_COLOR_ATTACHMENT0, bloom_[static_cast<std::size_t>(1 - pass)].texture, 0);
            glProgramUniform2f(blurProgram_.Id(), 1, pass == 0 ? 1.0f / halfWidth : 0.0f, pass == 0 ? 0.0f : 1.0f / halfHeight);
            glBindTextureUnit(0, bloom_[static_cast<std::size_t>(pass)].texture);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
        glBindTextureUnit(0, albedo_.texture);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(outputFramebuffer_));
    glViewport(outputViewport_[0], outputViewport_[1], outputViewport_[2], outputViewport_[3]);
    glProgramUniform1i(combine, kStageLocation, 0);
    glProgramUniform1i(combine, kViewLocation, static_cast<GLint>(parameters.view));
    glProgramUniform2i(combine, kOriginLocation, outputViewport_[0], outputViewport_[1]);
    glProgramUniform1f(combine, kBloomIntensityLocation, bloom ? parameters.bloomIntensity : 0.0f);
    glUseProgram(combine);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glQueryCounter(queries[3], GL_TIMESTAMP);
    // Leave the usual blending, and texture units 1 to 5 free.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (GLuint unit = 1; unit <= 5; ++unit) {
        glBindTextureUnit(unit, 0);
    }
}
} // namespace pixie
