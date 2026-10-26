#include "pixie/ParticleRenderer.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <cstddef>
#include <string_view>

namespace pixie {

namespace {

constexpr std::string_view kVertexShader = R"(#version 450 core
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aPrevious;
layout(location = 2) in float aAge;
layout(location = 3) in float aLifetime;
layout(location = 4) in vec2 aVelocity;

layout(location = 0) uniform mat4 uViewProjection;
layout(location = 1) uniform float uAlpha;
layout(location = 2) uniform float uRadius;
layout(location = 3) uniform vec4 uColor;
layout(location = 4) uniform int uFade;
layout(location = 5) uniform int uShape;
layout(location = 6) uniform float uStreakSeconds;

const int kStreak = 3;

// In units of the radius: x along the particle's axis, y across it.
out vec2 vLocal;
out vec4 vColor;
flat out int vShape;
// A Streak's tail length, in radii, behind the particle.
flat out float vTail;

void main() {
    vec2 corner = vec2((gl_VertexID & 1) == 0 ? -1.0 : 1.0, (gl_VertexID & 2) == 0 ? -1.0 : 1.0);
    vec2 center = mix(aPrevious, aPosition, uAlpha);
    vec2 axis = vec2(1.0, 0.0);
    float tail = 0.0;
    if (uShape == kStreak) {
        float speed = length(aVelocity);
        if (speed > 1e-6) {
            axis = aVelocity / speed;
        }
        tail = speed * uStreakSeconds / uRadius;
    }
    // The quad reaches back over the tail; the across direction keeps its width.
    vec2 local = vec2(corner.x < 0.0 ? -(1.0 + tail) : 1.0, corner.y);
    vec2 across = vec2(-axis.y, axis.x);
    gl_Position = uViewProjection * vec4(center + (axis * local.x + across * local.y) * uRadius, 0.0, 1.0);
    vLocal = local;
    vShape = uShape;
    vTail = tail;
    float life = (uFade != 0 && aLifetime > 0.0) ? clamp(1.0 - aAge / aLifetime, 0.0, 1.0) : 1.0;
    vColor = vec4(uColor.rgb, uColor.a * life);
}
)";

constexpr std::string_view kFragmentShader = R"(#version 450 core
in vec2 vLocal;
in vec4 vColor;
flat in int vShape;
flat in float vTail;

out vec4 oColor;

const int kSquare = 1;
const int kRing = 2;
const int kStreak = 3;
// A Ring's hole, as a fraction of its radius.
const float kRingHole = 0.6;

// Coverage of the edge where `distance` crosses 1, antialiased over a pixel.
float Inside(float distance) {
    float edge = max(fwidth(distance), 1e-4);
    return 1.0 - smoothstep(1.0 - edge, 1.0, distance);
}

void main() {
    float coverage;
    if (vShape == kSquare) {
        coverage = Inside(max(abs(vLocal.x), abs(vLocal.y)));
    } else if (vShape == kRing) {
        float distance = length(vLocal);
        float edge = max(fwidth(distance), 1e-4);
        coverage = Inside(distance) * smoothstep(kRingHole - edge, kRingHole, distance);
    } else if (vShape == kStreak) {
        // Distance to the segment from the tail's end to the particle, faded toward the tail's end.
        float along = clamp(vLocal.x, -vTail, 0.0);
        coverage = Inside(length(vec2(vLocal.x - along, vLocal.y)));
        coverage *= vTail > 0.0 ? 1.0 - 0.75 * (-along / vTail) : 1.0;
    } else {
        coverage = Inside(length(vLocal));
    }
    if (coverage <= 0.0) {
        discard;
    }
    oColor = vec4(vColor.rgb, vColor.a * coverage);
}
)";

constexpr GLint kViewProjection = 0;
constexpr GLint kAlpha = 1;
constexpr GLint kRadius = 2;
constexpr GLint kColor = 3;
constexpr GLint kFade = 4;
constexpr GLint kShape = 5;
constexpr GLint kStreakTime = 6;

} // namespace

bool ParticleRenderer::Create() {
    if (!program_.Create("ParticleRenderer", kVertexShader, kFragmentShader)) {
        return false;
    }
    particles_.Create();
    vertexArray_.Create();
    vertexArray_.BindBuffer(0, particles_.Id(), sizeof(Particle), true);
    vertexArray_.Attribute(0, 0, 2, GL_FLOAT, offsetof(Particle, position));
    vertexArray_.Attribute(1, 0, 2, GL_FLOAT, offsetof(Particle, previous));
    vertexArray_.Attribute(2, 0, 1, GL_FLOAT, offsetof(Particle, age));
    vertexArray_.Attribute(3, 0, 1, GL_FLOAT, offsetof(Particle, lifetime));
    vertexArray_.Attribute(4, 0, 2, GL_FLOAT, offsetof(Particle, velocity));
    return true;
}

void ParticleRenderer::Destroy() {
    vertexArray_.Destroy();
    particles_.Destroy();
    program_.Destroy();
}

void ParticleRenderer::Draw(std::span<const Particle> particles, const ParticleStyle& style,
                            double alpha, const Camera2D& camera) {
    if (particles.empty()) {
        return;
    }
    particles_.Upload(particles);

    const GLuint id = program_.Id();
    const pxMat4f viewProjection = camera.ViewProjection();
    const float minRadius = static_cast<float>(style.minPixels / camera.Zoom());
    glProgramUniformMatrix4fv(id, kViewProjection, 1, GL_FALSE, glm::value_ptr(viewProjection));
    glProgramUniform1f(id, kAlpha, static_cast<float>(alpha));
    glProgramUniform1f(id, kRadius, style.size > minRadius ? style.size : minRadius);
    glProgramUniform4f(id, kColor, style.color.r / 255.0f, style.color.g / 255.0f,
                       style.color.b / 255.0f, style.color.a / 255.0f);
    glProgramUniform1i(id, kFade, style.fade ? 1 : 0);
    glProgramUniform1i(id, kShape, static_cast<GLint>(style.shape));
    glProgramUniform1f(id, kStreakTime, kStreakSeconds);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, style.additive ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(id);
    glBindVertexArray(vertexArray_.Id());
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, static_cast<GLsizei>(particles.size()));
    // Leave the usual blending for whoever draws next.
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

} // namespace pixie
