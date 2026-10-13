#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

#include <pixie/math/pxVec.hpp>
#include <pixie/pxPoint.hpp>
#include <pixie/pxPrecision.hpp>
#include <pixie/pxToString.hpp>

// A segment of a line, from start to end: for debug drawing, gizmos, edges, collision, and the lines
// cast with a maximum distance, as Box2D's RayCast with its two points (docs/notas.md, 4.17). There
// is no ray: a ray with a length is a segment. Its direction is a vector, so it lives in pixie::math,
// and in int32_t it does what GLM does with an ivec2: PointAt truncates, as glm::mix, and the length
// comes in double (8.3), the one thing GLM refuses that PixieLib adds.
template<typename T>
struct pxSegment_t {
    static_assert(pxIsPrecision<T>, "pxSegment_t comes in double, float or int32_t");

    // The length is a double in int32_t, where it is seldom an integer.
    using LengthType = std::conditional_t<std::is_floating_point_v<T>, T, double>;

    pxPoint_t<T> start{};
    pxPoint_t<T> end{};

    constexpr pxSegment_t() noexcept = default;
    constexpr pxSegment_t(pxPoint_t<T> s, pxPoint_t<T> e) noexcept : start(s), end(e) {}

    template<typename U>
    constexpr explicit(!pxIsLossless<U, T>) pxSegment_t(const pxSegment_t<U>& o) noexcept
        : start(o.start), end(o.end) {}

    // end - start, not normalized: its length is the segment's. In int32_t it wraps around, as the
    // rest of the point (5.2); Length does not.
    constexpr glm::vec<2, T> Direction() const noexcept { return end - start; }

    // The point a fraction t of the way, the Lerp of the two ends: start at 0, end at 1.
    constexpr pxPoint_t<T> PointAt(typename pxPoint_t<T>::LerpScalar t) const noexcept {
        return pxPoint_t<T>::Lerp(start, end, t);
    }

    template<pxRejectedScalar<typename pxPoint_t<T>::LerpScalar> K> pxPoint_t<T> PointAt(K) const = delete;

    // glm::distance in float and double; in int32_t, pxDistance, with the difference in 64 bits.
    LengthType Length() const noexcept {
        if constexpr (std::is_floating_point_v<T>)
            return glm::distance(pxToVec2(start), pxToVec2(end));
        else
            return pxDistance(pxToVec2(start), pxToVec2(end));
    }

    std::string ToString() const { return pxTupleString(start.x, start.y, end.x, end.y); }

    friend constexpr bool operator==(const pxSegment_t&, const pxSegment_t&) noexcept = default;
};

using pxSegment  = pxSegment_t<double>;
using pxSegmentf = pxSegment_t<float>;
using pxSegmenti = pxSegment_t<std::int32_t>;

// The layout is the contract with C# (docs/layout.md): the two points, one after the other.
static_assert(std::is_standard_layout_v<pxSegment> && std::is_trivially_copyable_v<pxSegment>);
static_assert(sizeof(pxSegment) == 32 && offsetof(pxSegment, end) == 16);
static_assert(sizeof(pxSegmentf) == 16 && offsetof(pxSegmentf, end) == 8);
static_assert(sizeof(pxSegmenti) == 16 && offsetof(pxSegmenti, end) == 8);
