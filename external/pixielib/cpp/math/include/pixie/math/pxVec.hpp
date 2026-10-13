#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

#include <glm/glm.hpp>

#include <pixie/pxPoint.hpp>
#include <pixie/pxRect.hpp>
#include <pixie/pxToString.hpp>

// The vectors are GLM's (6.1), under the names of PixieLib: the unsuffixed one is double, then f for
// float and i for int32_t (docs/notas.md, 4.1). The math is GLM's too: glm::dot, glm::cross,
// glm::length, glm::normalize, glm::mix... The C# PxVec types have the same layout and, for each of
// these, the same operation under the name System.Numerics gives it (docs/notas.md, 4.13).
//
// The layout is the contract with C# (docs/layout.md), so GLM has to keep its packed types: with
// GLM_FORCE_DEFAULT_ALIGNED_GENTYPES a vec3 takes 16 bytes instead of 12, and the static_asserts below
// stop the build.
using pxVec2  = glm::dvec2;
using pxVec2f = glm::vec2;
using pxVec2i = glm::ivec2;
using pxVec3  = glm::dvec3;
using pxVec3f = glm::vec3;
using pxVec3i = glm::ivec3;
using pxVec4  = glm::dvec4;
using pxVec4f = glm::vec4;
using pxVec4i = glm::ivec4;

// The text of a vector, the same as that of a primitive (4.10): "(1.5, -2, 0)". A function, since an
// alias cannot have a ToString method.
template<glm::length_t L, typename T, glm::qualifier Q>
std::string pxToString(const glm::vec<L, T, Q>& v) {
    if constexpr (L == 2)
        return pxTupleString(v.x, v.y);
    else if constexpr (L == 3)
        return pxTupleString(v.x, v.y, v.z);
    else
        return pxTupleString(v.x, v.y, v.z, v.w);
}

// A point and a vec2 of the same precision hold the same data, so each converts to the other, as a
// point and a size do: explicitly, through a function, since an alias cannot have a constructor
// (6.6). In C#, (PxVec2)point and (PxPoint)vector.
template<typename T>
constexpr glm::vec<2, T> pxToVec2(const pxPoint_t<T>& p) noexcept {
    return glm::vec<2, T>(p.x, p.y);
}

template<typename T, glm::qualifier Q>
constexpr pxPoint_t<T> pxToPoint(const glm::vec<2, T, Q>& v) noexcept {
    return pxPoint_t<T>(v.x, v.y);
}

// A point is where something is, a vector how far and which way it moves (8.1). These are the
// operations between the two: point - point is the vector from one to the other, and a point plus or
// minus a vector is a point. They are templates on purpose: a template does not convert its
// arguments, so a point never passes for a vector here, and point + point keeps not compiling. In
// int32_t the arithmetic of the point wraps around, as in C# (5.2).
template<> struct pxPointDifference<double> { using type = pxVec2; };
template<> struct pxPointDifference<float> { using type = pxVec2f; };
template<> struct pxPointDifference<std::int32_t> { using type = pxVec2i; };

template<typename T>
constexpr typename pxPointDifference<T>::type operator-(pxPoint_t<T> a, pxPoint_t<T> b) noexcept {
    return typename pxPointDifference<T>::type(pxSub(a.x, b.x), pxSub(a.y, b.y));
}

template<typename T, glm::qualifier Q>
constexpr pxPoint_t<T> operator+(pxPoint_t<T> p, const glm::vec<2, T, Q>& v) noexcept {
    return { pxAdd(p.x, v.x), pxAdd(p.y, v.y) };
}

template<typename T, glm::qualifier Q>
constexpr pxPoint_t<T> operator-(pxPoint_t<T> p, const glm::vec<2, T, Q>& v) noexcept {
    return { pxSub(p.x, v.x), pxSub(p.y, v.y) };
}

// A rectangle moves by a vector, and the size stays (8.1).
template<typename T, glm::qualifier Q>
constexpr pxRect_t<T> operator+(const pxRect_t<T>& r, const glm::vec<2, T, Q>& v) noexcept {
    return { pxAdd(r.x, v.x), pxAdd(r.y, v.y), r.width, r.height };
}

template<typename T, glm::qualifier Q>
constexpr pxRect_t<T> operator-(const pxRect_t<T>& r, const glm::vec<2, T, Q>& v) noexcept {
    return { pxSub(r.x, v.x), pxSub(r.y, v.y), r.width, r.height };
}

// The length of an integer vec2, which GLM refuses (glm::length asks for floating point): it is
// seldom an integer, (3, 1) measures the root of 10, so it comes in double, and the squared one,
// exact, in int64_t, which compares distances without a root (8.3). The C# PxVec2i has the same,
// as Length(), LengthSquared(), Distance and DistanceSquared.
//
// The squares are computed in 64 bits: a difference of two int32_t fits, and its square fits in
// uint64_t. The squared length wraps around past INT64_MAX, as the rest of the integers (5.2): the
// length only at (INT32_MIN, INT32_MIN), the distance between points about 3 billion apart. The
// lengths in double square each side in double, exact while a square fits in 53 bits and rounded
// beyond, the same in C#; they do not wrap.
namespace pxMathDetail {
constexpr std::uint64_t Square(std::int64_t d) noexcept {
    return static_cast<std::uint64_t>(d) * static_cast<std::uint64_t>(d);
}

inline double Hypot(std::int64_t dx, std::int64_t dy) noexcept {
    double x = static_cast<double>(dx);
    double y = static_cast<double>(dy);
    return std::sqrt(x * x + y * y);
}
}  // namespace pxMathDetail

template<glm::qualifier Q>
constexpr std::int64_t pxLengthSquared(const glm::vec<2, std::int32_t, Q>& v) noexcept {
    return static_cast<std::int64_t>(pxMathDetail::Square(v.x) + pxMathDetail::Square(v.y));
}

template<glm::qualifier Q>
double pxLength(const glm::vec<2, std::int32_t, Q>& v) noexcept {
    return pxMathDetail::Hypot(v.x, v.y);
}

// From a to b, as glm::distance: the length of b - a, with the difference in 64 bits, where the
// int32_t one would wrap around.
template<glm::qualifier Q>
constexpr std::int64_t pxDistanceSquared(const glm::vec<2, std::int32_t, Q>& a,
                                         const glm::vec<2, std::int32_t, Q>& b) noexcept {
    std::int64_t dx = std::int64_t(b.x) - a.x;
    std::int64_t dy = std::int64_t(b.y) - a.y;
    return static_cast<std::int64_t>(pxMathDetail::Square(dx) + pxMathDetail::Square(dy));
}

template<glm::qualifier Q>
double pxDistance(const glm::vec<2, std::int32_t, Q>& a, const glm::vec<2, std::int32_t, Q>& b) noexcept {
    return pxMathDetail::Hypot(std::int64_t(b.x) - a.x, std::int64_t(b.y) - a.y);
}

// The layout is the contract with C# (docs/layout.md).
static_assert(std::is_standard_layout_v<pxVec3> && std::is_trivially_copyable_v<pxVec3>);
static_assert(std::is_standard_layout_v<pxVec4f> && std::is_trivially_copyable_v<pxVec4f>);
static_assert(sizeof(pxVec2) == 16 && sizeof(pxVec2f) == 8 && sizeof(pxVec2i) == 8);
static_assert(sizeof(pxVec3) == 24 && sizeof(pxVec3f) == 12 && sizeof(pxVec3i) == 12);
static_assert(sizeof(pxVec4) == 32 && sizeof(pxVec4f) == 16 && sizeof(pxVec4i) == 16);
static_assert(offsetof(pxVec3, z) == 16 && offsetof(pxVec3f, z) == 8 && offsetof(pxVec4i, w) == 12);
static_assert(alignof(pxVec3f) == 4 && alignof(pxVec4f) == 4,
              "GLM's aligned types change the layout (docs/layout.md)");
