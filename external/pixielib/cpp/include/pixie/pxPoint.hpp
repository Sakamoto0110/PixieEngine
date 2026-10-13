#pragma once

#include <cstddef>
#include <string>
#include <type_traits>

#include "pxPrecision.hpp"
#include "pxSize.hpp"
#include "pxToString.hpp"

// A position: where something is (8.1). A point and a size hold the same data, so each converts to
// the other, explicitly; a point plus or minus a size is a point, as the corner across a rectangle.
// What has no meaning for a position is not here: point + point, point * k, point / k and -point all
// give another answer when the origin of the world moves. The arithmetic with a vector, the
// displacement (a point plus or minus a vector, and point - point, which gives one), is in
// <pixie/math/pxVec.hpp>, with the vectors.
template<typename T>
struct pxPoint_t {
    static_assert(pxIsPrecision<T>, "pxPoint_t comes in double, float or int32_t");

    // The type of t in Lerp: T itself in float and double, and double for int32_t, as in C#.
    using LerpScalar = std::conditional_t<std::is_floating_point_v<T>, T, double>;

    T x{};
    T y{};

    constexpr pxPoint_t() noexcept = default;
    constexpr pxPoint_t(T px, T py) noexcept : x(px), y(py) {}
    constexpr explicit pxPoint_t(const pxSize_t<T>& sz) noexcept : x(sz.width), y(sz.height) {}

    template<typename U>
    constexpr explicit(!pxIsLossless<U, T>) pxPoint_t(const pxPoint_t<U>& o) noexcept
        : x(pxConvert<T>(o.x)), y(pxConvert<T>(o.y)) {}

    constexpr explicit operator pxSize_t<T>() const noexcept { return { x, y }; }

    constexpr bool IsEmpty() const noexcept { return x == 0 && y == 0; }

    std::string ToString() const { return pxTupleString(x, y); }

    // The point a fraction t of the way from a to b. It has a meaning where point + point does not:
    // the weights add up to 1, so the origin cancels out. The formula is glm::mix's, a * (1 - t) +
    // b * t, computed in the type of t; in int32_t that is double, and the result is truncated, as
    // glm::mix does with an ivec2 (docs/notas.md, 4.17), saturating out of range (5.3).
    static constexpr pxPoint_t Lerp(pxPoint_t a, pxPoint_t b, LerpScalar t) noexcept {
        return { Mix(a.x, b.x, t), Mix(a.y, b.y, t) };
    }

    // Only the t that C# converts implicitly (5.1): a pxPointf does not take a double.
    template<pxRejectedScalar<LerpScalar> K> static pxPoint_t Lerp(pxPoint_t, pxPoint_t, K) = delete;

    friend constexpr bool operator==(const pxPoint_t&, const pxPoint_t&) noexcept = default;

    // In int32_t the arithmetic wraps around, as in C# (5.2).
    friend constexpr pxPoint_t operator+(pxPoint_t pt, pxSize_t<T> sz) noexcept { return { pxAdd(pt.x, sz.width), pxAdd(pt.y, sz.height) }; }
    friend constexpr pxPoint_t operator-(pxPoint_t pt, pxSize_t<T> sz) noexcept { return { pxSub(pt.x, sz.width), pxSub(pt.y, sz.height) }; }
    friend constexpr pxPoint_t operator+(pxPoint_t pt) noexcept { return pt; }

private:
    static constexpr T Mix(T a, T b, LerpScalar t) noexcept {
        return pxConvert<T>(static_cast<LerpScalar>(a) * (LerpScalar(1) - t) + static_cast<LerpScalar>(b) * t);
    }
};

// point - point is a vector, and the vectors live in pixie::math (8.1), so the type comes from
// <pixie/math/pxVec.hpp>, which also defines the operator. Without it, the subtraction stops at this
// static_assert, which says what to do, instead of an error from deep inside a template.
template<typename T>
struct pxPointDifference {
    static_assert(sizeof(T) == 0,
                  "point - point gives a vector, and the vectors live in pixie::math: include "
                  "<pixie/math/pxVec.hpp> and link pixie::math (the PIXIE_MATH option)");
};

template<typename T>
constexpr typename pxPointDifference<T>::type operator-(pxPoint_t<T> a, pxPoint_t<T> b) noexcept;

using pxPoint  = pxPoint_t<double>;
using pxPointf = pxPoint_t<float>;
using pxPointi = pxPoint_t<std::int32_t>;

// The layout is the contract with C# (docs/layout.md).
static_assert(std::is_standard_layout_v<pxPoint> && std::is_trivially_copyable_v<pxPoint>);
static_assert(sizeof(pxPoint) == 16 && offsetof(pxPoint, y) == 8);
static_assert(sizeof(pxPointf) == 8 && offsetof(pxPointf, y) == 4);
static_assert(sizeof(pxPointi) == 8 && offsetof(pxPointi, y) == 4);
