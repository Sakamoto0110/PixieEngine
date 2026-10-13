#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <type_traits>

#include "pxPoint.hpp"
#include "pxPrecision.hpp"
#include "pxRect.hpp"
#include "pxToString.hpp"

// A rectangle as two corners: (x1, y1) is inside, (x2, y2) is just outside, as in pxRect_t, where
// x2 is x + width. It converts to and from pxRect_t explicitly, because the sums and differences can
// round in floating point.
template<typename T>
struct pxRegion_t {
    static_assert(pxIsPrecision<T>, "pxRegion_t comes in double, float or int32_t");

    T x1{};
    T y1{};
    T x2{};
    T y2{};

    constexpr pxRegion_t() noexcept = default;
    constexpr pxRegion_t(T px1, T py1, T px2, T py2) noexcept : x1(px1), y1(py1), x2(px2), y2(py2) {}
    constexpr explicit pxRegion_t(const pxRect_t<T>& r) noexcept : x1(r.x), y1(r.y), x2(r.Right()), y2(r.Bottom()) {}

    template<typename U>
    constexpr explicit(!pxIsLossless<U, T>) pxRegion_t(const pxRegion_t<U>& o) noexcept
        : x1(pxConvert<T>(o.x1)), y1(pxConvert<T>(o.y1)),
          x2(pxConvert<T>(o.x2)), y2(pxConvert<T>(o.y2)) {}

    constexpr explicit operator pxRect_t<T>() const noexcept { return { x1, y1, Width(), Height() }; }

    // In int32_t the difference wraps around, as in C# (5.2).
    constexpr T Width() const noexcept { return pxSub(x2, x1); }
    constexpr T Height() const noexcept { return pxSub(y2, y1); }

    constexpr bool IsEmpty() const noexcept { return x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0; }

    std::string ToString() const { return pxTupleString(x1, y1, x2, y2); }

    // The same edges as pxRect_t::Contains: x1 and y1 are inside, x2 and y2 are not.
    constexpr bool Contains(pxPoint_t<T> pt) const noexcept {
        return pt.x >= x1 && pt.x < x2 && pt.y >= y1 && pt.y < y2;
    }

    // The same rules as in pxRect_t: r is inside when its edges are, whatever its size.
    constexpr bool Contains(const pxRegion_t& r) const noexcept {
        return r.x1 >= x1 && r.x2 <= x2 && r.y1 >= y1 && r.y2 <= y2;
    }

    // Regions that only touch share nothing, and neither does one with no area.
    constexpr bool IntersectsWith(const pxRegion_t& r) const noexcept {
        return r.x1 < x2 && x1 < r.x2 && r.y1 < y2 && y1 < r.y2 && HasArea() && r.HasArea();
    }

    // The part the two share, or the empty region (all zeros) when they share nothing.
    static constexpr pxRegion_t Intersect(const pxRegion_t& a, const pxRegion_t& b) noexcept {
        if (!a.IntersectsWith(b))
            return {};
        return { std::max(a.x1, b.x1), std::max(a.y1, b.y1), std::min(a.x2, b.x2), std::min(a.y2, b.y2) };
    }

    // The smallest region around both. One with no area adds nothing; when neither has area, the
    // result is a.
    static constexpr pxRegion_t Union(const pxRegion_t& a, const pxRegion_t& b) noexcept {
        if (!b.HasArea())
            return a;
        if (!a.HasArea())
            return b;
        return { std::min(a.x1, b.x1), std::min(a.y1, b.y1), std::max(a.x2, b.x2), std::max(a.y2, b.y2) };
    }

    friend constexpr bool operator==(const pxRegion_t&, const pxRegion_t&) noexcept = default;

private:
    constexpr bool HasArea() const noexcept { return x2 > x1 && y2 > y1; }
};

using pxRegion  = pxRegion_t<double>;
using pxRegionf = pxRegion_t<float>;
using pxRegioni = pxRegion_t<std::int32_t>;

// The layout is the contract with C# (docs/layout.md).
static_assert(std::is_standard_layout_v<pxRegion> && std::is_trivially_copyable_v<pxRegion>);
static_assert(sizeof(pxRegion) == 32 && offsetof(pxRegion, y1) == 8 && offsetof(pxRegion, x2) == 16 && offsetof(pxRegion, y2) == 24);
static_assert(sizeof(pxRegionf) == 16 && offsetof(pxRegionf, y2) == 12);
static_assert(sizeof(pxRegioni) == 16 && offsetof(pxRegioni, y2) == 12);
