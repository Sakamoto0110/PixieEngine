#include "ViewScale.hpp"
#include "pixie/render/2d/PathRenderer.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <string_view>

namespace pixie {

namespace {

// Each segment is a quad around it, a pixel and a half wider than the line, so the antialiased edge
// fits; the fragment shader keeps what lies within halfWidth of the segment.
constexpr std::string_view kVertexShader = R"(#version 450 core
layout(location = 0) in vec2 aFrom;
layout(location = 1) in vec2 aTo;
layout(location = 2) in float aHalfWidth;
layout(location = 3) in float aStart;
layout(location = 4) in float aFillTo;
layout(location = 5) in vec4 aColor;
layout(location = 6) in vec4 aFillColor;

layout(location = 0) uniform mat4 uViewProjection;
layout(location = 1) uniform float uPixel;

out vec2 vWorld;
flat out vec2 vFrom;
flat out vec2 vTo;
flat out float vHalfWidth;
flat out float vStart;
flat out float vFillTo;
flat out vec4 vColor;
flat out vec4 vFillColor;

void main() {
    float halfWidth = max(aHalfWidth, 0.5 * uPixel);
    float reach = halfWidth + 1.5 * uPixel;
    vec2 along = aTo - aFrom;
    float size = length(along);
    vec2 direction = size > 0.0 ? along / size : vec2(1.0, 0.0);
    vec2 normal = vec2(-direction.y, direction.x);
    float u = float(gl_VertexID & 1);
    float v = (gl_VertexID & 2) == 0 ? -1.0 : 1.0;
    vec2 world = mix(aFrom, aTo, u) + direction * (u * 2.0 - 1.0) * reach + normal * v * reach;
    gl_Position = uViewProjection * vec4(world, 0.0, 1.0);
    vWorld = world;
    vFrom = aFrom;
    vTo = aTo;
    vHalfWidth = halfWidth;
    vStart = aStart;
    vFillTo = aFillTo;
    vColor = aColor;
    vFillColor = aFillColor;
}
)";

constexpr std::string_view kFragmentShader = R"(#version 450 core
in vec2 vWorld;
flat in vec2 vFrom;
flat in vec2 vTo;
flat in float vHalfWidth;
flat in float vStart;
flat in float vFillTo;
flat in vec4 vColor;
flat in vec4 vFillColor;

layout(location = 1) uniform float uPixel;

out vec4 oColor;

void main() {
    vec2 along = vTo - vFrom;
    float squared = dot(along, along);
    float t = squared > 0.0 ? clamp(dot(vWorld - vFrom, along) / squared, 0.0, 1.0) : 0.0;
    float distance = length(vWorld - (vFrom + along * t));
    float coverage = 1.0 - smoothstep(vHalfWidth - 0.5 * uPixel, vHalfWidth + 0.5 * uPixel, distance);
    if (coverage <= 0.0) {
        discard;
    }
    float position = vStart + t * sqrt(squared);
    vec4 color = position < vFillTo ? vFillColor : vColor;
    oColor = vec4(color.rgb, color.a * coverage);
}
)";

constexpr GLint kViewProjectionLocation = 0;
constexpr GLint kPixelLocation = 1;

pxPoint Add(pxPoint p, double x, double y) {
    return pxPoint(p.x + x, p.y + y);
}

} // namespace

std::vector<pxPoint> RoundCorners(std::span<const pxPoint> points, double radius) {
    std::vector<pxPoint> out;
    if (points.size() < 3 || radius <= 0.0) {
        out.assign(points.begin(), points.end());
        return out;
    }
    out.push_back(points.front());
    for (std::size_t i = 1; i + 1 < points.size(); ++i) {
        const pxPoint corner = points[i];
        const double ax = points[i - 1].x - corner.x;
        const double ay = points[i - 1].y - corner.y;
        const double bx = points[i + 1].x - corner.x;
        const double by = points[i + 1].y - corner.y;
        const double la = std::hypot(ax, ay);
        const double lb = std::hypot(bx, by);
        if (la <= 0.0 || lb <= 0.0) {
            out.push_back(corner);
            continue;
        }
        const double cosine = std::clamp((ax * bx + ay * by) / (la * lb), -1.0, 1.0);
        const double angle = std::acos(cosine);
        // Nearly straight on (angle near pi): no arc. Nearly folded back (angle near 0): no arc either.
        if (angle > std::numbers::pi - 1e-3 || angle < 1e-3) {
            out.push_back(corner);
            continue;
        }
        // The arc touches both segments `trim` away from the corner; each segment lends at most half
        // its length, the other half belonging to the corner at its other end.
        const double half = angle / 2.0;
        double trim = radius / std::tan(half);
        trim = std::min({trim, la / 2.0, lb / 2.0});
        const double r = trim * std::tan(half);
        const pxPoint from = Add(corner, ax / la * trim, ay / la * trim);
        const pxPoint to = Add(corner, bx / lb * trim, by / lb * trim);
        const double bisectorX = ax / la + bx / lb;
        const double bisectorY = ay / la + by / lb;
        const double bisector = std::hypot(bisectorX, bisectorY);
        const double centerDistance = r / std::sin(half);
        const pxPoint center = Add(corner, bisectorX / bisector * centerDistance, bisectorY / bisector * centerDistance);

        const double a0 = std::atan2(from.y - center.y, from.x - center.x);
        double sweep = std::atan2(to.y - center.y, to.x - center.x) - a0;
        sweep = std::remainder(sweep, 2.0 * std::numbers::pi);
        const int steps = std::max(2, static_cast<int>(std::ceil(std::abs(sweep) / (std::numbers::pi / 16.0))));
        for (int s = 0; s <= steps; ++s) {
            const double a = a0 + sweep * s / steps;
            out.push_back(pxPoint(center.x + r * std::cos(a), center.y + r * std::sin(a)));
        }
    }
    out.push_back(points.back());
    return out;
}

double PolylineLength(std::span<const pxPoint> points) {
    double length = 0.0;
    for (std::size_t i = 1; i < points.size(); ++i) {
        length += std::hypot(points[i].x - points[i - 1].x, points[i].y - points[i - 1].y);
    }
    return length;
}

bool PathRenderer::Create() {
    if (!program_.Create("PathRenderer", kVertexShader, kFragmentShader)) {
        return false;
    }
    segments_.Create();
    vertexArray_.Create();
    vertexArray_.BindBuffer(0, segments_.Id(), sizeof(PathSegment), true);
    vertexArray_.Attribute(0, 0, 2, GL_FLOAT, offsetof(PathSegment, from));
    vertexArray_.Attribute(1, 0, 2, GL_FLOAT, offsetof(PathSegment, to));
    vertexArray_.Attribute(2, 0, 1, GL_FLOAT, offsetof(PathSegment, halfWidth));
    vertexArray_.Attribute(3, 0, 1, GL_FLOAT, offsetof(PathSegment, start));
    vertexArray_.Attribute(4, 0, 1, GL_FLOAT, offsetof(PathSegment, fillTo));
    vertexArray_.Attribute(5, 0, 4, GL_UNSIGNED_BYTE, offsetof(PathSegment, color), true);
    vertexArray_.Attribute(6, 0, 4, GL_UNSIGNED_BYTE, offsetof(PathSegment, fillColor), true);
    return true;
}

void PathRenderer::Destroy() {
    vertexArray_.Destroy();
    segments_.Destroy();
    program_.Destroy();
    pending_.clear();
}

void PathRenderer::Submit(std::span<const pxPoint> points, double width, pxColorRgba color, pxColorRgba fillColor,
                          double fill) {
    if (points.size() < 2) {
        return;
    }
    const double fillTo = std::clamp(fill, 0.0, 1.0) * PolylineLength(points);
    double start = 0.0;
    for (std::size_t i = 1; i < points.size(); ++i) {
        PathSegment segment;
        segment.from = pxPointf(static_cast<float>(points[i - 1].x), static_cast<float>(points[i - 1].y));
        segment.to = pxPointf(static_cast<float>(points[i].x), static_cast<float>(points[i].y));
        segment.halfWidth = static_cast<float>(width / 2.0);
        segment.start = static_cast<float>(start);
        // A path filled whole is filled to its very end, round cap included.
        segment.fillTo = fill >= 1.0 ? 3.4e38f : static_cast<float>(fillTo);
        segment.color = color;
        segment.fillColor = fillColor;
        pending_.push_back(segment);
        start += std::hypot(points[i].x - points[i - 1].x, points[i].y - points[i - 1].y);
    }
}

void PathRenderer::Flush(const View& view) {
    if (pending_.empty()) {
        return;
    }
    segments_.Upload(std::span<const PathSegment>(pending_));
    const pxMat4f viewProjection(view.ViewProjection());
    glProgramUniformMatrix4fv(program_.Id(), kViewProjectionLocation, 1, GL_FALSE, glm::value_ptr(viewProjection));
    glProgramUniform1f(program_.Id(), kPixelLocation, static_cast<float>(1.0 / PixelsPerWorldUnit(view)));
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(program_.Id());
    glBindVertexArray(vertexArray_.Id());
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, static_cast<GLsizei>(pending_.size()));
    pending_.clear();
}

} // namespace pixie
