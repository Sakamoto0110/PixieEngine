#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <type_traits>

#include "pxPadding.hpp"
#include "pxPoint.hpp"
#include "pxPrecision.hpp"
#include "pxSize.hpp"
#include "pxToString.hpp"

// A rectangle as a corner and a size. pxRegion_t holds the same thing as two corners.
template<typename T>
struct pxRect_t {
    static_assert(pxIsPrecision<T>, "pxRect_t comes in double, float or int32_t");

    T x{};
    T y{};
    T width{};
    T height{};

    constexpr pxRect_t() noexcept = default;
    constexpr pxRect_t(T px, T py, T w, T h) noexcept : x(px), y(py), width(w), height(h) {}
    constexpr pxRect_t(pxPoint_t<T> location, pxSize_t<T> size) noexcept
        : x(location.x), y(location.y), width(size.width), height(size.height) {}

    template<typename U>
    constexpr explicit(!pxIsLossless<U, T>) pxRect_t(const pxRect_t<U>& o) noexcept
        : x(pxConvert<T>(o.x)), y(pxConvert<T>(o.y)),
          width(pxConvert<T>(o.width)), height(pxConvert<T>(o.height)) {}

    // In int32_t, Right and Bottom wrap around when x + width does not fit, as in C# (5.2). The
    // operations below compute the edges in 64 bits instead, so they hold for any rectangle.
    constexpr T Right() const noexcept { return pxAdd(x, width); }
    constexpr T Bottom() const noexcept { return pxAdd(y, height); }

    constexpr pxPoint_t<T> Location() const noexcept { return { x, y }; }
    constexpr pxSize_t<T> Size() const noexcept { return { width, height }; }

    // In int32_t, half the size is rounded toward zero, as an integer division.
    constexpr pxPoint_t<T> Center() const noexcept {
        return { static_cast<T>(W(x) + W(width) / 2), static_cast<T>(W(y) + W(height) / 2) };
    }

    constexpr bool IsEmpty() const noexcept { return x == 0 && y == 0 && width == 0 && height == 0; }

    std::string ToString() const { return pxTupleString(x, y, width, height); }

    // The left and top edges are inside, the right and bottom ones are not.
    constexpr bool Contains(pxPoint_t<T> pt) const noexcept {
        return pt.x >= x && W(pt.x) < Right64() && pt.y >= y && W(pt.y) < Bottom64();
    }

    // Whether r lies inside, its edges within these edges. Its size is not looked at: a rectangle
    // with no area on an edge, the right one included, is inside.
    constexpr bool Contains(const pxRect_t& r) const noexcept {
        return r.x >= x && r.Right64() <= Right64() && r.y >= y && r.Bottom64() <= Bottom64();
    }

    // Whether the two share any point. Rectangles that only touch do not, since the right and bottom
    // edges are outside; one with no area shares nothing.
    constexpr bool IntersectsWith(const pxRect_t& r) const noexcept {
        return r.x < Right64() && x < r.Right64() && r.y < Bottom64() && y < r.Bottom64() &&
               HasArea() && r.HasArea();
    }

    // The part the two share, or the empty rectangle (all zeros) when they share nothing.
    static constexpr pxRect_t Intersect(const pxRect_t& a, const pxRect_t& b) noexcept {
        if (!a.IntersectsWith(b))
            return {};
        T left = std::max(a.x, b.x);
        T top = std::max(a.y, b.y);
        return FromEdges(left, top, std::min(a.Right64(), b.Right64()), std::min(a.Bottom64(), b.Bottom64()));
    }

    // The smallest rectangle around both. One with no area adds nothing, so a Union that starts from
    // the empty rectangle does not grow toward (0, 0); when neither has area, the result is a.
    static constexpr pxRect_t Union(const pxRect_t& a, const pxRect_t& b) noexcept {
        if (!b.HasArea())
            return a;
        if (!a.HasArea())
            return b;
        T left = std::min(a.x, b.x);
        T top = std::min(a.y, b.y);
        return FromEdges(left, top, std::max(a.Right64(), b.Right64()), std::max(a.Bottom64(), b.Bottom64()));
    }

    // Moved by the padding, inward or outward. The size can go negative, when the padding is larger.
    constexpr pxRect_t Deflate(const pxPadding_t<T>& p) const noexcept {
        return { pxAdd(x, p.left), pxAdd(y, p.top), pxSub(width, p.Horizontal()), pxSub(height, p.Vertical()) };
    }
    constexpr pxRect_t Inflate(const pxPadding_t<T>& p) const noexcept {
        return { pxSub(x, p.left), pxSub(y, p.top), pxAdd(width, p.Horizontal()), pxAdd(height, p.Vertical()) };
    }

    friend constexpr bool operator==(const pxRect_t&, const pxRect_t&) noexcept = default;

    // Moving is a displacement, so a rectangle moves by a vector, rect + vector, which is in
    // <pixie/math/pxVec.hpp> with the vectors (8.1). Putting it somewhere is setting x and y.

private:
    using W = pxWide_t<T>;

    constexpr W Right64() const noexcept { return W(x) + W(width); }
    constexpr W Bottom64() const noexcept { return W(y) + W(height); }
    constexpr bool HasArea() const noexcept { return width > 0 && height > 0; }

    static constexpr pxRect_t FromEdges(T left, T top, W right, W bottom) noexcept {
        return { left, top, static_cast<T>(right - W(left)), static_cast<T>(bottom - W(top)) };
    }
};

using pxRect  = pxRect_t<double>;
using pxRectf = pxRect_t<float>;
using pxRecti = pxRect_t<std::int32_t>;

// The layout is the contract with C# (docs/layout.md).
static_assert(std::is_standard_layout_v<pxRect> && std::is_trivially_copyable_v<pxRect>);
static_assert(sizeof(pxRect) == 32 && offsetof(pxRect, y) == 8 && offsetof(pxRect, width) == 16 && offsetof(pxRect, height) == 24);
static_assert(sizeof(pxRectf) == 16 && offsetof(pxRectf, height) == 12);
static_assert(sizeof(pxRecti) == 16 && offsetof(pxRecti, height) == 12);
