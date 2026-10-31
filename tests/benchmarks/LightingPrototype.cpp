// The L0 lighting prototype (PIXIE-TASK-213): three soft-shadow techniques for 2D lights, compared
// against an exact reference on the same scenes and timed with GPU timer queries.
//
//   volumes  geometric shadow volumes from occluder edges, soft by analytic penumbra coverage
//   polar    per-light 1D polar shadow maps, soft by a blocker search and filtering
//   sdf      a signed distance field built per frame by jump flooding, soft by sphere tracing
//
// The reference sees each light as a disk of `source` radius and counts, per pixel, the fraction
// of the disk's angular extent that no occluder hides (1024 directions, exact ray tests). Every
// technique follows the same self-shadowing rule: an occluder never shadows its own interior.
//
// This is an explicit measurement tool, not a CTest test and not engine code. Usage:
//   lighting-prototype --out <folder> [--quick] [--commit <hash>]
// It writes environment.txt, quality.csv, flicker.csv, timing.csv and a few PNG images there.

#include "pixie/core/Image.hpp"
#include "pixie/render/gl/Gl.hpp"
#include "pixie/render/gl/GlLoader.hpp"
#include "pixie/render/gl/ShaderProgram.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numbers>
#include <string>
#include <vector>

namespace {

constexpr float kPi = std::numbers::pi_v<float>;
using Clock = std::chrono::steady_clock;

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

// Every occluder is a box: a convex polygon, counter-clockwise with Y up.
struct Box {
    Vec2 center;
    Vec2 half;
    float angle = 0.0f;
};

struct Light {
    Vec2 position;
    float range = 300.0f;
    float source = 10.0f;
};

enum class Technique { None, Volumes, Polar, Sdf, Reference };

struct Variant {
    const char* name;
    Technique technique;
    // Polar: the angular resolution. Sdf: world units per texel.
    int parameter = 0;
};

constexpr std::array<Variant, 8> kVariants{{
    {"none", Technique::None, 0},
    {"volumes", Technique::Volumes, 0},
    {"polar512", Technique::Polar, 512},
    {"polar1024", Technique::Polar, 1024},
    {"polar2048", Technique::Polar, 2048},
    {"sdf1", Technique::Sdf, 1},
    {"sdf2", Technique::Sdf, 2},
    {"sdf4", Technique::Sdf, 4},
}};

constexpr int kMaxLights = 64;
constexpr int kMaxBoxes = 512;
constexpr int kSdfMargin = 320;

// ---------------------------------------------------------------------------------------------
// Shaders.

constexpr const char* kCommon = R"(
#define PI 3.14159265358979
layout(std430, binding = 0) readonly buffer Lights { vec4 lights[]; };
layout(std430, binding = 1) readonly buffer Edges { vec4 edges[]; };
layout(std430, binding = 2) readonly buffer Polys { vec4 polys[]; };
layout(location = 0) uniform vec2 uCamera;
layout(location = 1) uniform vec2 uViewSize;
layout(location = 2) uniform int uLightBase;
layout(location = 3) uniform int uEdgeCount;
layout(location = 4) uniform int uLightCount;
layout(location = 5) uniform int uMapWidth;
layout(location = 6) uniform int uBoxCount;
layout(location = 7) uniform int uOutputVisibility;
layout(location = 8) uniform vec2 uSdfOrigin;
layout(location = 9) uniform vec2 uSdfSize;
layout(location = 10) uniform float uSdfTexel;
vec4 toClip(vec2 w) { return vec4((w - uCamera) / uViewSize * 2.0 - 1.0, 0.0, 1.0); }
float wrapPi(float x) { return x - 2.0 * PI * floor((x + PI) / (2.0 * PI)); }
float falloff(float d, float range) { float x = clamp(d / range, 0.0, 1.0); float a = 1.0 - x * x; return a * a; }
const vec4 kOffscreen = vec4(2.0, 2.0, 2.0, 1.0);
)";

constexpr const char* kLightVertex = R"(
flat out int vLight;
void main() {
    int li = uLightBase + gl_InstanceID;
    vec4 L = lights[li * 2];
    vec2 corner = vec2((gl_VertexID & 1) == 0 ? -1.0 : 1.0, (gl_VertexID & 2) == 0 ? -1.0 : 1.0);
    gl_Position = toClip(L.xy + corner * L.z);
    vLight = li;
}
)";

// The body of every light's fragment shader; TECH selects visibility().
constexpr const char* kLightFragment = R"(
flat in int vLight;
out vec4 oColor;

#if TECH == 0
float visibility(vec2 p, vec4 L, int row) { return 1.0; }
#elif TECH == 1
layout(binding = 0) uniform sampler2D uMask;
float visibility(vec2 p, vec4 L, int row) {
    return 1.0 - clamp(texelFetch(uMask, ivec2(gl_FragCoord.xy), 0).r, 0.0, 1.0);
}
#elif TECH == 2
layout(binding = 0) uniform sampler2D uPolar;
float depthAt(int row, int level, int i) {
    int w = uMapWidth >> level;
    i = ((i % w) + w) % w;
    return texelFetch(uPolar, ivec2(2 * uMapWidth - 2 * w + i, row), 0).r;
}
// The nearest occluder over the angles [lo, hi], from the min pyramid: a superset of the window.
float windowMin(int row, float lo, float hi) {
    float texel0 = 2.0 * PI / float(uMapWidth);
    int level = clamp(int(ceil(log2(max((hi - lo) / texel0 / 4.0, 1.0)))), 0, findMSB(uMapWidth) - 2);
    float texel = texel0 * float(1 << level);
    int i0 = int(floor((lo + PI) / texel));
    int i1 = int(floor((hi + PI) / texel));
    float m = 1e20;
    for (int i = i0; i <= min(i1, i0 + 6); ++i) m = min(m, depthAt(row, level, i));
    return m;
}
float litTap(int row, float theta, float d, float bias) {
    float x = (theta + PI) / (2.0 * PI) * float(uMapWidth) - 0.5;
    float i0 = floor(x);
    float a = depthAt(row, 0, int(i0)) >= d - bias ? 1.0 : 0.0;
    float b = depthAt(row, 0, int(i0) + 1) >= d - bias ? 1.0 : 0.0;
    return mix(a, b, x - i0);
}
float visibility(vec2 p, vec4 L, int row) {
    vec2 v = p - L.xy;
    float d = length(v);
    float r = L.w;
    if (d <= r) return 1.0;
    float theta = atan(v.y, v.x);
    float texel0 = 2.0 * PI / float(uMapWidth);
    // The map holds back faces, so no lit surface lies on a stored depth: no slope bias is needed,
    // and a larger one would light a band behind every occluder.
    float bias = 0.5;
    // The widest window through which a blocker at least 2r from the light can shadow p, then
    // narrowed to the window of the nearest blocker found.
    float w = clamp((d - 2.0 * r) / (2.0 * d), texel0, 0.75);
    float z = windowMin(row, theta - w, theta + w);
    for (int it = 0; it < 3; ++it) {
        if (z >= d - bias) return 1.0;
        w = clamp(r * (d - z) / (d * max(z, r)), texel0, 0.75);
        z = windowMin(row, theta - w, theta + w);
    }
    if (z >= d - bias) return 1.0;
    w = clamp(r * (d - z) / (d * max(z, r)), texel0, 0.75);
    const int kTaps = 24;
    float sum = 0.0;
    for (int i = 0; i < kTaps; ++i) {
        sum += litTap(row, theta + w * ((float(i) + 0.5) / float(kTaps) * 2.0 - 1.0), d, bias);
    }
    return sum / float(kTaps);
}
#elif TECH == 3
layout(binding = 0) uniform sampler2D uSdf;
float sdfAt(vec2 w) { return texture(uSdf, (w - uSdfOrigin) / uSdfSize).r; }
float visibility(vec2 p, vec4 L, int row) {
    vec2 toL = L.xy - p;
    float D = length(toL);
    float r = L.w;
    if (D <= r) return 1.0;
    vec2 dir = toL / D;
    float tEnd = D - r;
    float t = 0.0;
    // An occluder does not shadow its own interior: leave it first.
    for (int i = 0; i < 32; ++i) {
        float s = sdfAt(p + dir * t);
        if (s >= 0.0 || t >= tEnd) break;
        t += max(-s, 0.5 * uSdfTexel);
    }
    t += 0.5 * uSdfTexel;
    float res = 1.0;
    for (int i = 0; i < 64; ++i) {
        if (t >= tEnd) break;
        float s = sdfAt(p + dir * t);
        // A ray that went through an occluder is blocked: the depth inside a thin wall says nothing
        // about how much of the light it hides, and taking it as clearance leaks through the wall.
        if (s < 0.0) return 0.0;
        // Signed clearance as a fraction of the light's half-width at that distance.
        res = min(res, clamp(0.5 + 0.5 * s * D / (r * max(t, 1.0)), 0.0, 1.0));
        if (res < 0.002) break;
        t += clamp(abs(s), 0.5 * uSdfTexel, 64.0);
    }
    return res;
}
#elif TECH == 4
const int S = 1024;
const int WORDS = S / 32;
uint gMask[WORDS];
vec2 gPoly[4];
vec2 gP;
float gD, gR, gAlpha, gBase;
// Entry distance of a ray into the current convex polygon, or 1e20.
float entryDistance(vec2 o, vec2 dir) {
    float tIn = 0.0, tOut = 1e20;
    for (int k = 0; k < 4; ++k) {
        vec2 a = gPoly[k];
        vec2 e = gPoly[(k + 1) & 3] - a;
        vec2 n = vec2(e.y, -e.x);
        float num = dot(n, a - o);
        float den = dot(n, dir);
        if (abs(den) < 1e-9) { if (num < 0.0) return 1e20; continue; }
        float t = num / den;
        if (den < 0.0) tIn = max(tIn, t); else tOut = min(tOut, t);
    }
    return tIn <= tOut ? tIn : 1e20;
}
void markBits(int i0, int i1) {
    for (int w = i0 >> 5; w <= (i1 >> 5); ++w) {
        int lo = max(i0 - w * 32, 0), hi = min(i1 - w * 32, 31);
        uint bits = hi - lo == 31 ? 0xFFFFFFFFu : ((1u << uint(hi - lo + 1)) - 1u) << uint(lo);
        gMask[w] |= bits;
    }
}
void markInterval(float lo, float hi, bool exact) {
    lo = max(lo, -gAlpha);
    hi = min(hi, gAlpha);
    if (lo > hi) return;
    float scale = float(S) / (2.0 * gAlpha);
    int i0 = clamp(int(ceil((lo + gAlpha) * scale - 0.5)), 0, S - 1);
    int i1 = clamp(int(floor((hi + gAlpha) * scale - 0.5)), 0, S - 1);
    if (i0 > i1) return;
    if (!exact) { markBits(i0, i1); return; }
    for (int i = i0; i <= i1; ++i) {
        float phi = -gAlpha + (float(i) + 0.5) / scale;
        float sp = sin(phi);
        float tEnd = gD * cos(phi) - sqrt(max(gR * gR - gD * gD * sp * sp, 0.0));
        if (entryDistance(gP, vec2(cos(gBase + phi), sin(gBase + phi))) < tEnd) gMask[i >> 5] |= 1u << uint(i & 31);
    }
}
float visibility(vec2 p, vec4 L, int row) {
    vec2 toL = L.xy - p;
    float D = length(toL);
    if (D <= L.w) return 1.0;
    gP = p; gD = D; gR = L.w; gAlpha = asin(L.w / D); gBase = atan(toL.y, toL.x);
    for (int w = 0; w < WORDS; ++w) gMask[w] = 0u;
    for (int o = 0; o < uBoxCount; ++o) {
        gPoly[0] = polys[o * 2].xy; gPoly[1] = polys[o * 2].zw;
        gPoly[2] = polys[o * 2 + 1].xy; gPoly[3] = polys[o * 2 + 1].zw;
        bool inside = true;
        float rel[4];
        float maxD = 0.0;
        for (int k = 0; k < 4; ++k) {
            vec2 e = gPoly[(k + 1) & 3] - gPoly[k];
            if (e.x * (p.y - gPoly[k].y) - e.y * (p.x - gPoly[k].x) < 0.0) inside = false;
            vec2 q = gPoly[k] - p;
            rel[k] = wrapPi(atan(q.y, q.x) - gBase);
            maxD = max(maxD, length(q));
        }
        if (inside) continue;
        bool exact = maxD >= D - L.w;
        float lo = min(min(rel[0], rel[1]), min(rel[2], rel[3]));
        float hi = max(max(rel[0], rel[1]), max(rel[2], rel[3]));
        if (hi - lo > PI) {
            float lo2 = 1e20, hi2 = -1e20;
            for (int k = 0; k < 4; ++k) {
                float a = rel[k] < 0.0 ? rel[k] + 2.0 * PI : rel[k];
                lo2 = min(lo2, a); hi2 = max(hi2, a);
            }
            markInterval(lo2 - 2.0 * PI, hi2 - 2.0 * PI, exact);
            markInterval(lo2, hi2, exact);
        } else {
            markInterval(lo, hi, exact);
        }
    }
    int count = 0;
    for (int w = 0; w < WORDS; ++w) count += bitCount(gMask[w]);
    return 1.0 - float(count) / float(S);
}
#endif

void main() {
    vec4 L = lights[vLight * 2];
    vec4 C = lights[vLight * 2 + 1];
    vec2 p = uCamera + gl_FragCoord.xy;
    float d = distance(p, L.xy);
    if (d >= L.z) discard;
    float v = visibility(p, L, vLight);
    oColor = uOutputVisibility != 0 ? vec4(v, v * falloff(d, L.z), 0.0, 1.0)
                                    : vec4(C.rgb * C.a * falloff(d, L.z) * v, 1.0);
}
)";

// Volumes: one quad per edge that faces away from the light, covering its umbra and penumbra;
// the fragment adds the fraction of the light's angular extent the edge hides.
constexpr const char* kVolumeVertex = R"(
flat out vec4 vEdge;
vec2 outerDirection(vec2 v, vec2 c, float r, vec2 outward) {
    vec2 radial = normalize(v - c);
    float beta = asin(min(r / max(length(v - c), 1e-3), 0.999));
    float cb = cos(beta), sb = sin(beta);
    vec2 d1 = vec2(radial.x * cb - radial.y * sb, radial.x * sb + radial.y * cb);
    vec2 d2 = vec2(radial.x * cb + radial.y * sb, -radial.x * sb + radial.y * cb);
    return dot(d1, outward) > dot(d2, outward) ? d1 : d2;
}
void main() {
    vec4 L = lights[uLightBase * 2];
    vec4 e = edges[gl_InstanceID];
    vEdge = e;
    vec2 a = e.xy, b = e.zw, ab = b - a;
    vec2 n = vec2(ab.y, -ab.x);
    vec2 pa = L.xy - a;
    float h = clamp(dot(pa, ab) / dot(ab, ab), 0.0, 1.0);
    if (dot(n, 0.5 * (a + b) - L.xy) <= 0.0 || length(pa - ab * h) > L.z) { gl_Position = kOffscreen; return; }
    vec2 along = normalize(ab);
    float far = L.z + max(length(a - L.xy), length(b - L.xy));
    vec2 corner;
    if (gl_VertexID == 0) corner = a;
    else if (gl_VertexID == 1) corner = b;
    else if (gl_VertexID == 2) corner = a + far * outerDirection(a, L.xy, L.w, -along);
    else corner = b + far * outerDirection(b, L.xy, L.w, along);
    gl_Position = toClip(corner);
}
)";

constexpr const char* kVolumeFragment = R"(
flat in vec4 vEdge;
out vec4 oOcclusion;
void main() {
    vec4 L = lights[uLightBase * 2];
    vec2 p = uCamera + gl_FragCoord.xy;
    vec2 a = vEdge.xy, b = vEdge.zw;
    vec2 toL = L.xy - p;
    float D = length(toL);
    if (D <= L.w || D >= L.z) discard;
    // Only what lies beyond the edge's line, seen from the light, is shadowed by it.
    if (dot(vec2(b.y - a.y, a.x - b.x), p - a) <= 0.0) discard;
    float alpha = asin(L.w / D);
    float base = atan(toL.y, toL.x);
    float ta = wrapPi(atan(a.y - p.y, a.x - p.x) - base);
    float tb = wrapPi(atan(b.y - p.y, b.x - p.x) - base);
    float lo = min(ta, tb), hi = max(ta, tb);
    if (hi - lo > PI) discard;
    oOcclusion = vec4(max(0.0, min(hi, alpha) - max(lo, -alpha)) / (2.0 * alpha));
}
)";

// Polar: each back-facing edge is drawn into its light's row as the angles it spans; the fragment
// keeps the nearest hit of three rays across the texel, so touching edges leave no gap.
constexpr const char* kPolarVertex = R"(
flat out vec4 vEdge;
flat out vec2 vOrigin;
void main() {
    int per = uEdgeCount * 2;
    int li = gl_InstanceID / per;
    int rem = gl_InstanceID - li * per;
    int copy = rem & 1;
    vec4 L = lights[li * 2];
    vec4 e = edges[rem >> 1];
    vEdge = e;
    vOrigin = L.xy;
    gl_Position = kOffscreen;
    vec2 a = e.xy, b = e.zw, ab = b - a;
    vec2 pa = L.xy - a;
    float h = clamp(dot(pa, ab) / dot(ab, ab), 0.0, 1.0);
    if (dot(vec2(ab.y, -ab.x), 0.5 * (a + b) - L.xy) <= 0.0 || length(pa - ab * h) > L.z) return;
    float ta = atan(a.y - L.y, a.x - L.x);
    float tb = atan(b.y - L.y, b.x - L.x);
    float d = wrapPi(tb - ta);
    float t0 = d >= 0.0 ? ta : tb;
    float t1 = t0 + abs(d);
    float pad = 2.0 * PI / float(uMapWidth);
    float shift = 0.0;
    if (copy == 1) {
        if (t1 + pad > PI) shift = -2.0 * PI;
        else if (t0 - pad < -PI) shift = 2.0 * PI;
        else return;
    }
    float x0 = (t0 - pad + shift) / PI, x1 = (t1 + pad + shift) / PI;
    float y0 = float(li) / float(uLightCount) * 2.0 - 1.0;
    float y1 = float(li + 1) / float(uLightCount) * 2.0 - 1.0;
    gl_Position = vec4((gl_VertexID & 1) == 0 ? x0 : x1, (gl_VertexID & 2) == 0 ? y0 : y1, 0.0, 1.0);
}
)";

constexpr const char* kPolarFragment = R"(
flat in vec4 vEdge;
flat in vec2 vOrigin;
out vec4 oDepth;
float raySegment(vec2 o, vec2 dir, vec2 a, vec2 b) {
    vec2 e = b - a;
    float den = dir.x * e.y - dir.y * e.x;
    if (abs(den) < 1e-12) return 1e20;
    vec2 ao = a - o;
    float t = (ao.x * e.y - ao.y * e.x) / den;
    float s = (ao.x * dir.y - ao.y * dir.x) / den;
    return t <= 0.0 || s < -1e-3 || s > 1.001 ? 1e20 : t;
}
void main() {
    float texel = 2.0 * PI / float(uMapWidth);
    float theta = gl_FragCoord.x * texel - PI;
    float best = 1e20;
    for (int k = -1; k <= 1; ++k) {
        float th = theta + float(k) * 0.5 * texel;
        best = min(best, raySegment(vOrigin, vec2(cos(th), sin(th)), vEdge.xy, vEdge.zw));
    }
    if (best >= 1e19) discard;
    oDepth = vec4(best);
}
)";

// The polar map's min pyramid lives beside level 0 in the same rows: level l starts at
// 2N - 2(N >> l).
constexpr const char* kPolarPyramid = R"(
layout(local_size_x = 64) in;
layout(binding = 0, r32f) uniform image2D uMap;
layout(location = 20) uniform int uSrc;
layout(location = 21) uniform int uDst;
layout(location = 22) uniform int uWidth;
layout(location = 23) uniform int uRows;
void main() {
    int x = int(gl_GlobalInvocationID.x), y = int(gl_GlobalInvocationID.y);
    if (x >= uWidth || y >= uRows) return;
    float a = imageLoad(uMap, ivec2(uSrc + 2 * x, y)).r;
    float b = imageLoad(uMap, ivec2(uSrc + 2 * x + 1, y)).r;
    imageStore(uMap, ivec2(uDst + x, y), vec4(min(a, b)));
}
)";

// Sdf: occluders filled into an occupancy texture over the view plus a margin.
constexpr const char* kOccupancyVertex = R"(
void main() {
    int o = gl_InstanceID;
    const int order[4] = int[](0, 1, 3, 2);
    int k = order[gl_VertexID];
    vec4 pair = polys[o * 2 + (k >> 1)];
    vec2 w = (k & 1) == 0 ? pair.xy : pair.zw;
    gl_Position = vec4((w - uSdfOrigin) / uSdfSize * 2.0 - 1.0, 0.0, 1.0);
}
)";

constexpr const char* kOccupancyFragment = R"(
out vec4 oOccupied;
void main() { oOccupied = vec4(1.0); }
)";

// Jump flooding for both sides at once: xy is the nearest occupied texel, zw the nearest empty one.
constexpr const char* kJfaInit = R"(
layout(local_size_x = 8, local_size_y = 8) in;
layout(binding = 0) uniform sampler2D uOccupancy;
layout(binding = 1, rgba32f) writeonly uniform image2D uDst;
layout(location = 23) uniform ivec2 uSize;
void main() {
    ivec2 c = ivec2(gl_GlobalInvocationID.xy);
    if (any(greaterThanEqual(c, uSize))) return;
    bool occupied = texelFetch(uOccupancy, c, 0).r > 0.5;
    imageStore(uDst, c, occupied ? vec4(vec2(c), -1e4, -1e4) : vec4(-1e4, -1e4, vec2(c)));
}
)";

constexpr const char* kJfaStep = R"(
layout(local_size_x = 8, local_size_y = 8) in;
layout(binding = 0, rgba32f) readonly uniform image2D uSrc;
layout(binding = 1, rgba32f) writeonly uniform image2D uDst;
layout(location = 22) uniform int uStep;
layout(location = 23) uniform ivec2 uSize;
void main() {
    ivec2 c = ivec2(gl_GlobalInvocationID.xy);
    if (any(greaterThanEqual(c, uSize))) return;
    vec4 best = imageLoad(uSrc, c);
    vec2 fc = vec2(c);
    float bo = best.x < -1.0 ? 1e20 : dot(best.xy - fc, best.xy - fc);
    float bi = best.z < -1.0 ? 1e20 : dot(best.zw - fc, best.zw - fc);
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            ivec2 q = c + ivec2(dx, dy) * uStep;
            if (any(lessThan(q, ivec2(0))) || any(greaterThanEqual(q, uSize))) continue;
            vec4 s = imageLoad(uSrc, q);
            if (s.x > -1.0) { float d = dot(s.xy - fc, s.xy - fc); if (d < bo) { bo = d; best.xy = s.xy; } }
            if (s.z > -1.0) { float d = dot(s.zw - fc, s.zw - fc); if (d < bi) { bi = d; best.zw = s.zw; } }
        }
    }
    imageStore(uDst, c, best);
}
)";

constexpr const char* kJfaFinal = R"(
layout(local_size_x = 8, local_size_y = 8) in;
layout(binding = 0, rgba32f) readonly uniform image2D uSrc;
layout(binding = 1, r32f) writeonly uniform image2D uDst;
layout(location = 23) uniform ivec2 uSize;
layout(location = 24) uniform float uTexel;
void main() {
    ivec2 c = ivec2(gl_GlobalInvocationID.xy);
    if (any(greaterThanEqual(c, uSize))) return;
    vec4 s = imageLoad(uSrc, c);
    vec2 fc = vec2(c);
    // The boundary lies half a texel from the nearest texel of the other kind.
    float d = s.x > -1.0 && distance(s.xy, fc) > 0.0 ? distance(s.xy, fc) - 0.5 : -(distance(s.zw, fc) - 0.5);
    imageStore(uDst, c, vec4(d * uTexel));
}
)";

constexpr const char* kFullscreenVertex = R"(
void main() {
    vec2 p = vec2((gl_VertexID & 1) == 0 ? -1.0 : 3.0, (gl_VertexID & 2) == 0 ? -1.0 : 3.0);
    gl_Position = vec4(p, 0.0, 1.0);
}
)";

// The combine step every technique shares: albedo decoded from sRGB, times ambient plus light,
// tone-mapped and encoded again.
constexpr const char* kCombineFragment = R"(
layout(binding = 0) uniform sampler2D uLight;
layout(binding = 1) uniform sampler2D uAlbedo;
out vec4 oColor;
vec3 decode(vec3 c) { return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(0.04045, c)); }
vec3 encode(vec3 c) { return mix(c * 12.92, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, step(0.0031308, c)); }
void main() {
    ivec2 c = ivec2(gl_FragCoord.xy);
    vec3 albedo = decode(texelFetch(uAlbedo, c, 0).rgb);
    vec3 color = albedo * (texelFetch(uLight, c, 0).rgb + vec3(0.05));
    color = color / (1.0 + color);
    oColor = vec4(encode(color), 1.0);
}
)";

std::string Source(const char* body, int tech = -1) {
    std::string text = "#version 450 core\n";
    if (tech >= 0) {
        text += "#define TECH " + std::to_string(tech) + "\n";
    }
    text += kCommon;
    text += body;
    return text;
}

GLuint CreateCompute(const char* name, const char* body) {
    const std::string text = Source(body);
    const char* source = text.c_str();
    const GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok == 0) {
        std::array<char, 4096> log{};
        glGetShaderInfoLog(shader, static_cast<GLsizei>(log.size()), nullptr, log.data());
        std::cerr << name << ": " << log.data() << '\n';
        glDeleteShader(shader);
        return 0;
    }
    const GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);
    glDeleteShader(shader);
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (ok == 0) {
        std::cerr << name << ": link failed\n";
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

// ---------------------------------------------------------------------------------------------
// GL resources.

struct Texture {
    GLuint id = 0;
    GLuint framebuffer = 0;
    int width = 0;
    int height = 0;

    void Create(GLenum format, int w, int h, GLint filter, bool attach) {
        Destroy();
        width = w;
        height = h;
        glCreateTextures(GL_TEXTURE_2D, 1, &id);
        glTextureStorage2D(id, 1, format, w, h);
        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, filter);
        glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, filter);
        glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        if (attach) {
            glCreateFramebuffers(1, &framebuffer);
            glNamedFramebufferTexture(framebuffer, GL_COLOR_ATTACHMENT0, id, 0);
        }
    }
    void Destroy() {
        if (framebuffer != 0) glDeleteFramebuffers(1, &framebuffer);
        if (id != 0) glDeleteTextures(1, &id);
        framebuffer = 0;
        id = 0;
    }
};

struct Timestamps {
    std::array<GLuint, 5> queries{};
    void Create() { glCreateQueries(GL_TIMESTAMP, static_cast<GLsizei>(queries.size()), queries.data()); }
    void Mark(int i) const { glQueryCounter(queries[static_cast<std::size_t>(i)], GL_TIMESTAMP); }
    double Milliseconds(int from, int to) const {
        GLuint64 a = 0;
        GLuint64 b = 0;
        glGetQueryObjectui64v(queries[static_cast<std::size_t>(from)], GL_QUERY_RESULT, &a);
        glGetQueryObjectui64v(queries[static_cast<std::size_t>(to)], GL_QUERY_RESULT, &b);
        return static_cast<double>(b - a) / 1.0e6;
    }
};

struct FrameTimes {
    double cpu = 0.0;      // building and uploading occluders and lights, and issuing every command
    double wall = 0.0;     // cpu plus waiting for the GPU to finish
    double prepare = 0.0;  // GPU: per-frame shadow data (polar maps, distance field)
    double lights = 0.0;   // GPU: light accumulation, with shadows
    double combine = 0.0;  // GPU: the combine and tone-map step
    double total = 0.0;    // GPU: all of it, clears included
};

class Lab {
public:
    bool Create() {
        const pixie::ShaderProgram* failed = nullptr;
        for (int tech = 0; tech <= 4; ++tech) {
            const std::string name = "light" + std::to_string(tech);
            if (!lightPrograms_[static_cast<std::size_t>(tech)].Create(name, Source(kLightVertex, tech), Source(kLightFragment, tech))) {
                failed = &lightPrograms_[static_cast<std::size_t>(tech)];
            }
        }
        if (failed != nullptr || !volume_.Create("volume", Source(kVolumeVertex), Source(kVolumeFragment)) ||
            !polar_.Create("polar", Source(kPolarVertex), Source(kPolarFragment)) ||
            !occupancy_.Create("occupancy", Source(kOccupancyVertex), Source(kOccupancyFragment)) ||
            !combine_.Create("combine", Source(kFullscreenVertex), Source(kCombineFragment))) {
            return false;
        }
        pyramid_ = CreateCompute("pyramid", kPolarPyramid);
        jfaInit_ = CreateCompute("jfa-init", kJfaInit);
        jfaStep_ = CreateCompute("jfa-step", kJfaStep);
        jfaFinal_ = CreateCompute("jfa-final", kJfaFinal);
        if (pyramid_ == 0 || jfaInit_ == 0 || jfaStep_ == 0 || jfaFinal_ == 0) return false;
        glCreateVertexArrays(1, &vertexArray_);
        glCreateBuffers(1, &lightBuffer_);
        glNamedBufferStorage(lightBuffer_, sizeof(float) * 8 * kMaxLights, nullptr, GL_DYNAMIC_STORAGE_BIT);
        glCreateBuffers(1, &edgeBuffer_);
        glNamedBufferStorage(edgeBuffer_, sizeof(float) * 4 * 4 * kMaxBoxes, nullptr, GL_DYNAMIC_STORAGE_BIT);
        glCreateBuffers(1, &polyBuffer_);
        glNamedBufferStorage(polyBuffer_, sizeof(float) * 8 * kMaxBoxes, nullptr, GL_DYNAMIC_STORAGE_BIT);
        timestamps_.Create();
        return true;
    }

    // While the context is current.
    void Destroy() {
        for (pixie::ShaderProgram& program : lightPrograms_) program.Destroy();
        for (pixie::ShaderProgram* program : {&volume_, &polar_, &occupancy_, &combine_}) program->Destroy();
        for (const GLuint program : {pyramid_, jfaInit_, jfaStep_, jfaFinal_}) glDeleteProgram(program);
        for (Texture* texture : {&light_, &mask_, &output_, &albedo_, &polarMap_, &occupancyMap_, &flood_[0], &flood_[1], &sdf_}) {
            texture->Destroy();
        }
        const std::array<GLuint, 3> buffers{lightBuffer_, edgeBuffer_, polyBuffer_};
        glDeleteBuffers(3, buffers.data());
        glDeleteVertexArrays(1, &vertexArray_);
        glDeleteQueries(static_cast<GLsizei>(timestamps_.queries.size()), timestamps_.queries.data());
    }

    void Resize(int width, int height) {
        if (width == width_ && height == height_) return;
        width_ = width;
        height_ = height;
        light_.Create(GL_RGBA16F, width, height, GL_NEAREST, true);
        mask_.Create(GL_R16F, width, height, GL_NEAREST, true);
        output_.Create(GL_RGBA8, width, height, GL_NEAREST, true);
        albedo_.Create(GL_RGBA8, width, height, GL_NEAREST, false);
        const std::vector<std::uint8_t> gray(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4, 180);
        glTextureSubImage2D(albedo_.id, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, gray.data());
        sdfScale_ = 0;
    }

    // Uploads this frame's occluders and lights.
    void Upload(const std::vector<Box>& boxes, const std::vector<Light>& lights) {
        edges_.clear();
        polys_.clear();
        for (const Box& box : boxes) {
            const float c = std::cos(box.angle);
            const float s = std::sin(box.angle);
            std::array<Vec2, 4> v{};
            const std::array<Vec2, 4> local{{{-box.half.x, -box.half.y}, {box.half.x, -box.half.y},
                                             {box.half.x, box.half.y}, {-box.half.x, box.half.y}}};
            for (std::size_t k = 0; k < 4; ++k) {
                v[k] = {box.center.x + c * local[k].x - s * local[k].y, box.center.y + s * local[k].x + c * local[k].y};
            }
            for (std::size_t k = 0; k < 4; ++k) {
                const Vec2 a = v[k];
                const Vec2 b = v[(k + 1) % 4];
                edges_.insert(edges_.end(), {a.x, a.y, b.x, b.y});
                polys_.insert(polys_.end(), {a.x, a.y});
            }
        }
        lightData_.clear();
        for (const Light& light : lights) {
            lightData_.insert(lightData_.end(), {light.position.x, light.position.y, light.range, light.source,
                                                 1.0f, 0.9f, 0.8f, 1.0f});
        }
        boxCount_ = static_cast<int>(boxes.size());
        lightCount_ = static_cast<int>(lights.size());
        if (!edges_.empty()) {
            glNamedBufferSubData(edgeBuffer_, 0, static_cast<GLsizeiptr>(edges_.size() * sizeof(float)), edges_.data());
            glNamedBufferSubData(polyBuffer_, 0, static_cast<GLsizeiptr>(polys_.size() * sizeof(float)), polys_.data());
        }
        glNamedBufferSubData(lightBuffer_, 0, static_cast<GLsizeiptr>(lightData_.size() * sizeof(float)), lightData_.data());
        lights_ = lights;
    }

    // Renders one frame of `variant` at `camera`; with `visibility`, the light buffer's red channel
    // holds the visibility of the (single) light instead of its color.
    void Render(const Variant& variant, Vec2 camera, bool visibility) {
        camera_ = camera;
        timestamps_.Mark(0);
        glBindVertexArray(vertexArray_);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightBuffer_);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, edgeBuffer_);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, polyBuffer_);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glBindFramebuffer(GL_FRAMEBUFFER, light_.framebuffer);
        glViewport(0, 0, width_, height_);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        timestamps_.Mark(1);
        switch (variant.technique) {
        case Technique::Polar: PreparePolar(variant.parameter); break;
        case Technique::Sdf: PrepareSdf(variant.parameter); break;
        default: break;
        }
        timestamps_.Mark(2);
        if (variant.technique == Technique::Volumes) {
            DrawVolumes(visibility);
        } else {
            DrawLights(variant, visibility);
        }
        timestamps_.Mark(3);
        glDisable(GL_BLEND);
        glDisable(GL_SCISSOR_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, output_.framebuffer);
        glViewport(0, 0, width_, height_);
        glUseProgram(combine_.Id());
        glBindTextureUnit(0, light_.id);
        glBindTextureUnit(1, albedo_.id);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
        timestamps_.Mark(4);
    }

    FrameTimes Times() const {
        FrameTimes t;
        t.prepare = timestamps_.Milliseconds(1, 2);
        t.lights = timestamps_.Milliseconds(2, 3);
        t.combine = timestamps_.Milliseconds(3, 4);
        t.total = timestamps_.Milliseconds(0, 4);
        return t;
    }

    // The light buffer's red channel, first row at the bottom.
    std::vector<float> ReadVisibility() const {
        std::vector<float> pixels(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_));
        glGetTextureImage(light_.id, 0, GL_RED, GL_FLOAT, static_cast<GLsizei>(pixels.size() * sizeof(float)), pixels.data());
        return pixels;
    }

    int Width() const { return width_; }
    int Height() const { return height_; }

private:
    void SetCommon(GLuint program) const {
        glUseProgram(program);
        glUniform2f(0, camera_.x, camera_.y);
        glUniform2f(1, static_cast<float>(width_), static_cast<float>(height_));
        glUniform1i(3, boxCount_ * 4);
        glUniform1i(4, lightCount_);
        glUniform1i(6, boxCount_);
    }

    void DrawLights(const Variant& variant, bool visibility) {
        const GLuint program = lightPrograms_[static_cast<std::size_t>(variant.technique)].Id();
        SetCommon(program);
        glUniform1i(2, 0);
        glUniform1i(5, variant.parameter);
        glUniform1i(7, visibility ? 1 : 0);
        if (variant.technique == Technique::Polar) {
            glBindTextureUnit(0, polarMap_.id);
        } else if (variant.technique == Technique::Sdf) {
            glBindTextureUnit(0, sdf_.id);
            glUniform2f(8, sdfOrigin_.x, sdfOrigin_.y);
            glUniform2f(9, sdfSize_.x, sdfSize_.y);
            glUniform1f(10, static_cast<float>(variant.parameter));
        }
        glBindFramebuffer(GL_FRAMEBUFFER, light_.framebuffer);
        glViewport(0, 0, width_, height_);
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_ONE, GL_ONE);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, lightCount_);
    }

    // Per light: its shadow quads into the mask, then the light reading the mask.
    void DrawVolumes(bool visibility) {
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_ONE, GL_ONE);
        glEnable(GL_SCISSOR_TEST);
        for (int i = 0; i < lightCount_; ++i) {
            const Light& light = lights_[static_cast<std::size_t>(i)];
            const int x0 = std::clamp(static_cast<int>(std::floor(light.position.x - light.range - camera_.x)), 0, width_);
            const int y0 = std::clamp(static_cast<int>(std::floor(light.position.y - light.range - camera_.y)), 0, height_);
            const int x1 = std::clamp(static_cast<int>(std::ceil(light.position.x + light.range - camera_.x)), 0, width_);
            const int y1 = std::clamp(static_cast<int>(std::ceil(light.position.y + light.range - camera_.y)), 0, height_);
            if (x1 <= x0 || y1 <= y0) continue;
            glScissor(x0, y0, x1 - x0, y1 - y0);
            glBindFramebuffer(GL_FRAMEBUFFER, mask_.framebuffer);
            glClear(GL_COLOR_BUFFER_BIT);
            if (boxCount_ > 0) {
                SetCommon(volume_.Id());
                glUniform1i(2, i);
                glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, boxCount_ * 4);
            }
            glBindFramebuffer(GL_FRAMEBUFFER, light_.framebuffer);
            SetCommon(lightPrograms_[1].Id());
            glUniform1i(2, i);
            glUniform1i(7, visibility ? 1 : 0);
            glBindTextureUnit(0, mask_.id);
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 1);
        }
        glDisable(GL_SCISSOR_TEST);
    }

    void PreparePolar(int width) {
        if (polarWidth_ != width) {
            polarWidth_ = width;
            polarMap_.Create(GL_R32F, 2 * width, kMaxLights, GL_NEAREST, true);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, polarMap_.framebuffer);
        glViewport(0, 0, width, lightCount_);
        glEnable(GL_SCISSOR_TEST);
        glScissor(0, 0, width, lightCount_);
        glClearColor(1e20f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        if (boxCount_ > 0) {
            SetCommon(polar_.Id());
            glUniform1i(5, width);
            glEnable(GL_BLEND);
            glBlendEquation(GL_MIN);
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, lightCount_ * boxCount_ * 4 * 2);
            glBlendEquation(GL_FUNC_ADD);
            glDisable(GL_BLEND);
        }
        glDisable(GL_SCISSOR_TEST);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT);
        glUseProgram(pyramid_);
        glBindImageTexture(0, polarMap_.id, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
        glUniform1i(23, lightCount_);
        int src = 0;
        for (int w = width / 2; w >= 1; w /= 2) {
            const int dst = 2 * width - 2 * w;
            glUniform1i(20, src);
            glUniform1i(21, dst);
            glUniform1i(22, w);
            glDispatchCompute(static_cast<GLuint>((w + 63) / 64), static_cast<GLuint>(lightCount_), 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
            src = dst;
        }
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
    }

    void PrepareSdf(int scale) {
        // The domain is snapped to the texel grid in world space, so a moving camera samples the
        // same field.
        const float texel = static_cast<float>(scale);
        sdfOrigin_ = {std::floor((camera_.x - kSdfMargin) / texel) * texel, std::floor((camera_.y - kSdfMargin) / texel) * texel};
        const int w = (width_ + 2 * kSdfMargin) / scale + 2;
        const int h = (height_ + 2 * kSdfMargin) / scale + 2;
        sdfSize_ = {static_cast<float>(w) * texel, static_cast<float>(h) * texel};
        if (sdfScale_ != scale) {
            sdfScale_ = scale;
            occupancyMap_.Create(GL_R8, w, h, GL_NEAREST, true);
            flood_[0].Create(GL_RGBA32F, w, h, GL_NEAREST, false);
            flood_[1].Create(GL_RGBA32F, w, h, GL_NEAREST, false);
            sdf_.Create(GL_R32F, w, h, GL_LINEAR, false);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, occupancyMap_.framebuffer);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);
        if (boxCount_ > 0) {
            SetCommon(occupancy_.Id());
            glUniform2f(8, sdfOrigin_.x, sdfOrigin_.y);
            glUniform2f(9, sdfSize_.x, sdfSize_.y);
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, boxCount_);
        }
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
        const GLuint gx = static_cast<GLuint>((w + 7) / 8);
        const GLuint gy = static_cast<GLuint>((h + 7) / 8);
        glUseProgram(jfaInit_);
        glUniform2i(23, w, h);
        glBindTextureUnit(0, occupancyMap_.id);
        glBindImageTexture(1, flood_[0].id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        glDispatchCompute(gx, gy, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        int step = 1;
        while (step * 2 < std::max(w, h)) step *= 2;
        int current = 0;
        glUseProgram(jfaStep_);
        glUniform2i(23, w, h);
        // The usual halving steps, then one more of 1 (JFA+1) to repair the remaining errors.
        for (bool last = false; !last;) {
            last = step == 0;
            const int s = last ? 1 : step;
            glUniform1i(22, s);
            glBindImageTexture(0, flood_[static_cast<std::size_t>(current)].id, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
            glBindImageTexture(1, flood_[static_cast<std::size_t>(1 - current)].id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
            glDispatchCompute(gx, gy, 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
            current = 1 - current;
            step /= 2;
        }
        glUseProgram(jfaFinal_);
        glUniform2i(23, w, h);
        glUniform1f(24, texel);
        glBindImageTexture(0, flood_[static_cast<std::size_t>(current)].id, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
        glBindImageTexture(1, sdf_.id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
        glDispatchCompute(gx, gy, 1);
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
    }

    std::array<pixie::ShaderProgram, 5> lightPrograms_;
    pixie::ShaderProgram volume_;
    pixie::ShaderProgram polar_;
    pixie::ShaderProgram occupancy_;
    pixie::ShaderProgram combine_;
    GLuint pyramid_ = 0;
    GLuint jfaInit_ = 0;
    GLuint jfaStep_ = 0;
    GLuint jfaFinal_ = 0;
    GLuint vertexArray_ = 0;
    GLuint lightBuffer_ = 0;
    GLuint edgeBuffer_ = 0;
    GLuint polyBuffer_ = 0;
    Timestamps timestamps_;
    Texture light_;
    Texture mask_;
    Texture output_;
    Texture albedo_;
    Texture polarMap_;
    Texture occupancyMap_;
    std::array<Texture, 2> flood_;
    Texture sdf_;
    int polarWidth_ = 0;
    int sdfScale_ = 0;
    Vec2 sdfOrigin_;
    Vec2 sdfSize_;
    int width_ = 0;
    int height_ = 0;
    int boxCount_ = 0;
    int lightCount_ = 0;
    Vec2 camera_;
    std::vector<Light> lights_;
    std::vector<float> edges_;
    std::vector<float> polys_;
    std::vector<float> lightData_;
};

// ---------------------------------------------------------------------------------------------
// Scenes.

struct Scene {
    const char* name;
    std::vector<Box> boxes;
    Light light;
};

Box Rect(float x0, float y0, float x1, float y1) {
    return {{0.5f * (x0 + x1), 0.5f * (y0 + y1)}, {0.5f * (x1 - x0), 0.5f * (y1 - y0)}, 0.0f};
}

// Scenes are authored for a 960x720 view with the camera at the origin; `k` scales them.
std::vector<Scene> QualityScenes(float k) {
    auto S = [k](Box b) {
        b.center = {b.center.x * k, b.center.y * k};
        b.half = {b.half.x * k, b.half.y * k};
        return b;
    };
    std::vector<Scene> scenes;
    std::vector<Box> mixed{
        S({{620, 360}, {30, 30}, 0.0f}),           // a box
        S({{360, 470}, {50, 14}, 0.52f}),          // a rotated plank
        S({{520, 400}, {10, 10}, 0.0f}),           // a small box close to the light
        S(Rect(300, 200, 380, 230)), S(Rect(350, 230, 380, 300)),  // an L of two touching boxes
        S(Rect(600, 160, 760, 163)),               // a thin wall, 3 units
        S({{420, 300}, {6, 40}, -0.3f}),           // a thin post
        S(Rect(690, 420, 730, 460)), S(Rect(730, 420, 770, 460)),  // two touching boxes in a row
    };
    scenes.push_back({"mixed", mixed, {{480 * k, 360 * k}, 340 * k, 12 * k}});
    scenes.push_back({"mixed-soft", mixed, {{480 * k, 360 * k}, 340 * k, 30 * k}});
    // A closed room of 4-unit walls that touch exactly at the corners, the light inside: nothing
    // outside may be lit.
    std::vector<Box> room{
        S(Rect(330, 230, 630, 234)), S(Rect(330, 486, 630, 490)),
        S(Rect(330, 234, 334, 486)), S(Rect(626, 234, 630, 486)),
        S({{540, 300}, {40, 2}, 0.785f}),
    };
    scenes.push_back({"closed-room", room, {{440 * k, 380 * k}, 340 * k, 12 * k}});
    // A light close to the outer corner of two touching boxes: the umbra behind the corner.
    std::vector<Box> corner{S(Rect(500, 300, 600, 340)), S(Rect(560, 340, 600, 440))};
    scenes.push_back({"corner", corner, {{470 * k, 470 * k}, 340 * k, 12 * k}});
    return scenes;
}

struct ErrorStats {
    double mae = 0.0;
    double bias = 0.0;
    double p99 = 0.0;
    double leakFraction = 0.0;  // eroded umbra pixels lit above 0.05
    double leakMax = 0.0;
    double acneFraction = 0.0;  // eroded fully lit pixels darkened below 0.95
    std::size_t pixels = 0;
};

double Percentile(std::vector<double> values, double q) {
    if (values.empty()) return 0.0;
    std::sort(values.begin(), values.end());
    const std::size_t i = std::min(values.size() - 1, static_cast<std::size_t>(q * static_cast<double>(values.size() - 1) + 0.5));
    return values[i];
}

// Pixels within the light's range and outside its source disk.
std::vector<std::uint8_t> RangeMask(int w, int h, Vec2 camera, const Light& light) {
    std::vector<std::uint8_t> mask(static_cast<std::size_t>(w) * static_cast<std::size_t>(h), 0);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const float px = camera.x + static_cast<float>(x) + 0.5f - light.position.x;
            const float py = camera.y + static_cast<float>(y) + 0.5f - light.position.y;
            const float d = std::sqrt(px * px + py * py);
            mask[static_cast<std::size_t>(y * w + x)] = d < light.range - 1.0f && d > light.source + 1.0f ? 1 : 0;
        }
    }
    return mask;
}

// Pixels whose whole (2r+1)^2 neighborhood satisfies `test`.
template <typename Test>
std::vector<std::uint8_t> Eroded(int w, int h, const std::vector<float>& image, int r, Test test) {
    std::vector<std::uint8_t> out(image.size(), 0);
    for (int y = r; y < h - r; ++y) {
        for (int x = r; x < w - r; ++x) {
            bool all = true;
            for (int dy = -r; dy <= r && all; ++dy) {
                for (int dx = -r; dx <= r && all; ++dx) {
                    all = test(image[static_cast<std::size_t>((y + dy) * w + x + dx)]);
                }
            }
            out[static_cast<std::size_t>(y * w + x)] = all ? 1 : 0;
        }
    }
    return out;
}

ErrorStats Compare(int w, int h, const std::vector<float>& image, const std::vector<float>& reference,
                   const std::vector<std::uint8_t>& mask) {
    ErrorStats stats;
    std::vector<double> errors;
    const auto umbra = Eroded(w, h, reference, 2, [](float v) { return v < 0.0005f; });
    const auto lit = Eroded(w, h, reference, 2, [](float v) { return v > 0.9995f; });
    std::size_t umbraCount = 0;
    std::size_t litCount = 0;
    std::size_t leaks = 0;
    std::size_t acne = 0;
    for (std::size_t i = 0; i < image.size(); ++i) {
        if (mask[i] == 0) continue;
        const double e = static_cast<double>(image[i]) - static_cast<double>(reference[i]);
        stats.mae += std::abs(e);
        stats.bias += e;
        errors.push_back(std::abs(e));
        if (umbra[i] != 0) {
            ++umbraCount;
            if (image[i] > 0.05f) ++leaks;
            stats.leakMax = std::max(stats.leakMax, static_cast<double>(image[i]));
        }
        if (lit[i] != 0) {
            ++litCount;
            if (image[i] < 0.95f) ++acne;
        }
    }
    stats.pixels = errors.size();
    if (!errors.empty()) {
        stats.mae /= static_cast<double>(errors.size());
        stats.bias /= static_cast<double>(errors.size());
    }
    stats.p99 = Percentile(errors, 0.99);
    stats.leakFraction = umbraCount == 0 ? 0.0 : static_cast<double>(leaks) / static_cast<double>(umbraCount);
    stats.acneFraction = litCount == 0 ? 0.0 : static_cast<double>(acne) / static_cast<double>(litCount);
    return stats;
}

void WriteGray(const std::filesystem::path& path, int w, int h, const std::vector<float>& image) {
    std::vector<std::uint8_t> rgba(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const float v = std::clamp(image[static_cast<std::size_t>((h - 1 - y) * w + x)], 0.0f, 1.0f);
            const auto g = static_cast<std::uint8_t>(std::lround(v * 255.0f));
            const std::size_t o = static_cast<std::size_t>(y * w + x) * 4;
            rgba[o] = g; rgba[o + 1] = g; rgba[o + 2] = g; rgba[o + 3] = 255;
        }
    }
    pixie::WriteFileBytes(path, pixie::EncodePng(w, h, rgba));
}

// Too bright in red, too dark in blue, |error| times four.
void WriteError(const std::filesystem::path& path, int w, int h, const std::vector<float>& image,
                const std::vector<float>& reference) {
    std::vector<std::uint8_t> rgba(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const std::size_t i = static_cast<std::size_t>((h - 1 - y) * w + x);
            const float e = std::clamp((image[i] - reference[i]) * 4.0f, -1.0f, 1.0f);
            const std::size_t o = static_cast<std::size_t>(y * w + x) * 4;
            const auto base = static_cast<std::uint8_t>(std::lround(reference[i] * 40.0f));
            rgba[o] = e > 0.0f ? static_cast<std::uint8_t>(std::lround(e * 255.0f)) : base;
            rgba[o + 1] = base;
            rgba[o + 2] = e < 0.0f ? static_cast<std::uint8_t>(std::lround(-e * 255.0f)) : base;
            rgba[o + 3] = 255;
        }
    }
    pixie::WriteFileBytes(path, pixie::EncodePng(w, h, rgba));
}

std::vector<float> RenderVisibility(Lab& lab, const Variant& variant, Vec2 camera) {
    lab.Render(variant, camera, true);
    glFinish();
    return lab.ReadVisibility();
}

constexpr Variant kReference{"reference", Technique::Reference, 0};

void RunQuality(Lab& lab, const std::filesystem::path& out, bool quick) {
    const float k = quick ? 0.5f : 1.0f;
    const int w = static_cast<int>(960 * k);
    const int h = static_cast<int>(720 * k);
    lab.Resize(w, h);
    std::ofstream csv(out / "quality.csv");
    csv << "scene,variant,pixels,mae,bias,p99,leak_fraction,leak_max,acne_fraction\n";
    for (const Scene& scene : QualityScenes(k)) {
        lab.Upload(scene.boxes, {scene.light});
        const Vec2 camera{0.0f, 0.0f};
        const auto reference = RenderVisibility(lab, kReference, camera);
        const auto mask = RangeMask(w, h, camera, scene.light);
        WriteGray(out / (std::string(scene.name) + "-reference.png"), w, h, reference);
        for (const Variant& variant : kVariants) {
            const auto image = RenderVisibility(lab, variant, camera);
            const ErrorStats s = Compare(w, h, image, reference, mask);
            csv << scene.name << ',' << variant.name << ',' << s.pixels << ',' << s.mae << ',' << s.bias << ','
                << s.p99 << ',' << s.leakFraction << ',' << s.leakMax << ',' << s.acneFraction << '\n';
            const std::string v = variant.name;
            if (v == "volumes" || v == "polar1024" || v == "sdf2" || (std::string(scene.name) == "mixed" && v != "none")) {
                WriteGray(out / (std::string(scene.name) + "-" + v + ".png"), w, h, image);
                WriteError(out / (std::string(scene.name) + "-" + v + "-error.png"), w, h, image, reference);
            }
        }
        std::cout << "quality " << scene.name << " done\n";
    }
}

// Two sequences at 60 steps per second, with the camera panning by a fraction of a pixel per frame:
// "moving" also rotates a platform, slides a door and moves the light on a small circle; "camera"
// keeps the scene still. The error against the reference is compared frame to frame on screen:
// what changes in it is flicker or swimming, not motion.
void RunFlicker(Lab& lab, const std::filesystem::path& out, bool quick) {
    const float k = quick ? 0.5f : 1.0f;
    const int w = static_cast<int>(960 * k);
    const int h = static_cast<int>(720 * k);
    const int frames = quick ? 24 : 90;
    lab.Resize(w, h);
    std::ofstream csv(out / "flicker.csv");
    csv << "sequence,variant,frames,mean_abs_error,temporal_mean,temporal_p99,temporal_max\n";
    std::vector<Variant> variants;
    for (const Variant& v : kVariants) {
        if (v.technique != Technique::None) variants.push_back(v);
    }
    for (const bool moving : {true, false}) {
        std::vector<std::vector<float>> previous(variants.size());
        std::vector<double> errorSum(variants.size(), 0.0);
        std::vector<std::vector<double>> deltas(variants.size());
        std::vector<std::uint8_t> previousMask;
        for (int f = 0; f < frames; ++f) {
            const float t = moving ? static_cast<float>(f) / 60.0f : 0.0f;
            std::vector<Box> boxes{
                {{660 * k, 430 * k}, {110 * k, 7 * k}, 0.35f * t},                       // rotating platform
                {{(330 + 40 * std::sin(1.3f * t)) * k, 470 * k}, {8 * k, 60 * k}, 0.0f},  // sliding door
                {{400 * k, 250 * k}, {30 * k, 30 * k}, 0.1f},
                {{560 * k, 230 * k}, {50 * k, 3 * k}, -0.2f},
            };
            const Light light{{(480 + 40 * std::cos(0.5f * t)) * k, (360 + 40 * std::sin(0.5f * t)) * k}, 340 * k, 12 * k};
            const Vec2 camera{0.37f * k * static_cast<float>(f), 0.21f * k * static_cast<float>(f)};
            lab.Upload(boxes, {light});
            const auto reference = RenderVisibility(lab, kReference, camera);
            const auto mask = RangeMask(w, h, camera, light);
            for (std::size_t v = 0; v < variants.size(); ++v) {
                auto image = RenderVisibility(lab, variants[v], camera);
                std::vector<float> error(image.size());
                double sum = 0.0;
                std::size_t n = 0;
                for (std::size_t i = 0; i < image.size(); ++i) {
                    error[i] = image[i] - reference[i];
                    if (mask[i] != 0) { sum += std::abs(error[i]); ++n; }
                }
                errorSum[v] += n == 0 ? 0.0 : sum / static_cast<double>(n);
                if (!previous[v].empty()) {
                    for (std::size_t i = 0; i < error.size(); ++i) {
                        if (mask[i] != 0 && previousMask[i] != 0) deltas[v].push_back(std::abs(error[i] - previous[v][i]));
                    }
                }
                previous[v] = std::move(error);
            }
            previousMask = mask;
        }
        for (std::size_t v = 0; v < variants.size(); ++v) {
            double mean = 0.0;
            double max = 0.0;
            for (const double d : deltas[v]) { mean += d; max = std::max(max, d); }
            mean /= std::max<double>(1.0, static_cast<double>(deltas[v].size()));
            csv << (moving ? "moving" : "camera") << ',' << variants[v].name << ',' << frames << ','
                << errorSum[v] / frames << ',' << mean << ',' << Percentile(deltas[v], 0.99) << ',' << max << '\n';
        }
    }
    std::cout << "flicker done\n";
}

// Deterministic boxes over twice the view, and lights within it.
struct StressScene {
    std::vector<Box> boxes;
    std::vector<float> spin;
    std::vector<Light> lights;
};

StressScene MakeStress(int width, int height, int boxCount, int lightCount) {
    std::uint32_t state = 12345u;
    auto next = [&state]() {
        state = state * 1664525u + 1013904223u;
        return static_cast<float>(state >> 8) / 16777216.0f;
    };
    StressScene scene;
    const float w = static_cast<float>(width);
    const float h = static_cast<float>(height);
    for (int i = 0; i < boxCount; ++i) {
        scene.boxes.push_back({{-0.5f * w + 2.0f * w * next(), -0.5f * h + 2.0f * h * next()},
                               {8.0f + 40.0f * next(), 8.0f + 40.0f * next()}, 6.28f * next()});
        scene.spin.push_back(next() - 0.5f);
    }
    for (int i = 0; i < lightCount; ++i) {
        scene.lights.push_back({{w * next(), h * next()}, lightCount > 16 ? 220.0f : 300.0f, 10.0f});
    }
    return scene;
}

struct Summary {
    double p50, p95, p99, max;
};

Summary Summarize(std::vector<double> values) {
    return {Percentile(values, 0.5), Percentile(values, 0.95), Percentile(values, 0.99),
            values.empty() ? 0.0 : *std::max_element(values.begin(), values.end())};
}

void RunTiming(Lab& lab, const std::filesystem::path& out, bool quick) {
    std::ofstream csv(out / "timing.csv");
    csv << "variant,width,height,lights,boxes,frames";
    for (const char* stage : {"cpu", "wall", "gpu_prepare", "gpu_lights", "gpu_combine", "gpu_total"}) {
        csv << ',' << stage << "_p50," << stage << "_p95," << stage << "_p99," << stage << "_max";
    }
    csv << '\n';
    struct Size { int w, h; };
    const std::vector<Size> sizes = quick ? std::vector<Size>{{640, 360}, {1280, 720}} : std::vector<Size>{{1280, 720}, {1920, 1080}};
    const std::vector<int> lightCounts = quick ? std::vector<int>{1, 16} : std::vector<int>{1, 16, 64};
    const std::vector<int> boxCounts = quick ? std::vector<int>{32, 128} : std::vector<int>{32, 128, 512};
    const int warm = quick ? 3 : 30;
    const int measured = quick ? 10 : 120;
    for (const Size size : sizes) {
        lab.Resize(size.w, size.h);
        for (const int lights : lightCounts) {
            for (const int boxes : boxCounts) {
                StressScene scene = MakeStress(size.w, size.h, boxes, lights);
                for (const Variant& variant : kVariants) {
                    const std::string name = variant.name;
                    const bool main = name == "none" || name == "volumes" || name == "polar1024" || name == "sdf2";
                    const bool probe = (size.w == (quick ? 1280 : 1920)) && lights == 16 && boxes == 128;
                    if (!main && !probe) continue;
                    std::array<std::vector<double>, 6> samples;
                    for (int f = 0; f < warm + measured; ++f) {
                        const auto start = Clock::now();
                        const float t = static_cast<float>(f) / 60.0f;
                        std::vector<Box> moved = scene.boxes;
                        for (std::size_t i = 0; i < moved.size(); ++i) moved[i].angle += scene.spin[i] * t;
                        lab.Upload(moved, scene.lights);
                        lab.Render(variant, {0.0f, 0.0f}, false);
                        const auto issued = Clock::now();
                        glFinish();
                        const auto finished = Clock::now();
                        if (f < warm) continue;
                        FrameTimes ft = lab.Times();
                        ft.cpu = std::chrono::duration<double, std::milli>(issued - start).count();
                        ft.wall = std::chrono::duration<double, std::milli>(finished - start).count();
                        const std::array<double, 6> values{ft.cpu, ft.wall, ft.prepare, ft.lights, ft.combine, ft.total};
                        for (std::size_t s = 0; s < values.size(); ++s) samples[s].push_back(values[s]);
                    }
                    csv << name << ',' << size.w << ',' << size.h << ',' << lights << ',' << boxes << ',' << measured;
                    for (auto& stage : samples) {
                        const Summary s = Summarize(stage);
                        csv << ',' << s.p50 << ',' << s.p95 << ',' << s.p99 << ',' << s.max;
                    }
                    csv << '\n';
                    csv.flush();
                    std::cout << "timing " << name << ' ' << size.w << 'x' << size.h << " lights " << lights
                              << " boxes " << boxes << " gpu p50 " << Summarize(samples[5]).p50 << " ms\n";
                }
            }
        }
    }
}

int Run(int argc, char** argv) {
    std::filesystem::path out = "lighting-prototype";
    bool quick = false;
    std::string commit = "unknown";
    std::string only;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--out" && i + 1 < argc) out = argv[++i];
        else if (arg == "--quick") quick = true;
        else if (arg == "--commit" && i + 1 < argc) commit = argv[++i];
        else if (arg == "--only" && i + 1 < argc) only = argv[++i];
        else {
            std::cerr << "usage: lighting-prototype --out <folder> [--quick] [--commit <hash>] [--only quality|flicker|timing]\n";
            return 2;
        }
    }
    std::filesystem::create_directories(out);
    if (glfwInit() == GLFW_FALSE) return 1;
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "lighting-prototype", nullptr, nullptr);
    if (window == nullptr) {
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    int status = 1;
    if (pixie::LoadGl()) {
        {
            std::ofstream env(out / "environment.txt");
            env << "renderer: " << reinterpret_cast<const char*>(glGetString(GL_RENDERER)) << '\n'
                << "version: " << reinterpret_cast<const char*>(glGetString(GL_VERSION)) << '\n'
                << "vendor: " << reinterpret_cast<const char*>(glGetString(GL_VENDOR)) << '\n'
#ifdef NDEBUG
                << "configuration: Release\n"
#else
                << "configuration: Debug\n"
#endif
                << "commit: " << commit << '\n'
                << "mode: " << (quick ? "quick" : "full") << '\n';
        }
        Lab lab;
        if (lab.Create()) {
            const auto start = Clock::now();
            if (only.empty() || only == "quality") RunQuality(lab, out, quick);
            if (only.empty() || only == "flicker") RunFlicker(lab, out, quick);
            if (only.empty() || only == "timing") RunTiming(lab, out, quick);
            std::cout << "done in " << std::chrono::duration<double>(Clock::now() - start).count() << " s\n";
            status = 0;
        }
        lab.Destroy();
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return status;
}

} // namespace

int main(int argc, char** argv) {
    return Run(argc, argv);
}
