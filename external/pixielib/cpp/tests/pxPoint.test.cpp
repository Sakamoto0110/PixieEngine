#include <cstdint>
#include <limits>
#include <type_traits>

#include <pixie/pxPoint.hpp>

#include "check.hpp"

namespace {

// Whether any of p * k, k * p and p / k compiles.
template<typename P, typename K>
concept Scalable = requires(P p, K k) { p * k; } || requires(P p, K k) { k * p; } ||
                   requires(P p, K k) { p / k; };

template<typename P>
concept Addable = requires(P a, P b) { a + b; };

template<typename P>
concept Negatable = requires(P p) { -p; };

template<typename P, typename K>
concept Lerpable = requires(P a, P b, K t) { P::Lerp(a, b, t); };

}  // namespace

void TestPoint() {
    pxPoint pt{ 1.5, -2 };
    PX_CHECK(pt.x == 1.5 && pt.y == -2);
    PX_CHECK(pxPoint{}.IsEmpty());
    PX_CHECK(!pt.IsEmpty());

    PX_CHECK(pt + pxSize(2, 3) == pxPoint(3.5, 1));
    PX_CHECK(pt - pxSize(2, 3) == pxPoint(-0.5, -5));
    PX_CHECK(+pt == pt);

    // What depends on the origin does not compile (8.1): point + point, point * k, point / k and
    // -point. point - point and a point with a vector are in pxVec.test.cpp, with pixie::math.
    static_assert(!Addable<pxPoint> && !Addable<pxPointf> && !Addable<pxPointi>);
    static_assert(!Scalable<pxPoint, double> && !Scalable<pxPoint, int> && !Scalable<pxPointi, int>);
    static_assert(!Negatable<pxPoint> && !Negatable<pxPointi>);

    // Lerp, glm::mix's formula: a * (1 - t) + b * t.
    PX_CHECK(pxPoint::Lerp(pxPoint(0, 0), pxPoint(10, 20), 0.25) == pxPoint(2.5, 5));
    PX_CHECK(pxPoint::Lerp(pt, pxPoint(9, 9), 0) == pt && pxPoint::Lerp(pt, pxPoint(9, 9), 1) == pxPoint(9, 9));
    PX_CHECK(pxPoint::Lerp(pxPoint(0, 0), pxPoint(10, 20), 2) == pxPoint(20, 40));
    PX_CHECK(pxPointf::Lerp(pxPointf(7.6f, 0), pxPointf(4.8f, 1), 0.22f).x == 6.9839993f);
    // In int32_t, t is a double and the result is truncated, as glm::mix does with an ivec2; an
    // ivec2 with a float t would lose 2^24 + 1, a double does not.
    PX_CHECK(pxPointi::Lerp(pxPointi(1, -2), pxPointi(4, -5), 0.5) == pxPointi(2, -3));
    PX_CHECK(pxPointi::Lerp(pxPointi(0, 0), pxPointi(3, -3), 0.5) == pxPointi(1, -1));
    PX_CHECK(pxPointi::Lerp(pxPointi(16777217, 0), pxPointi(16777217, 0), 0.5) == pxPointi(16777217, 0));
    PX_CHECK(pxPointi::Lerp(pxPointi(0, 0), pxPointi(INT32_MAX, INT32_MIN), 2) == pxPointi(INT32_MAX, INT32_MIN));
    // t has to be one that C# converts implicitly (5.1).
    static_assert(Lerpable<pxPoint, double> && Lerpable<pxPoint, float> && Lerpable<pxPoint, int>);
    static_assert(Lerpable<pxPointf, float> && !Lerpable<pxPointf, double>);
    static_assert(Lerpable<pxPointi, double> && Lerpable<pxPointi, float>);

    // A point and a size hold the same data; the conversion is explicit both ways.
    PX_CHECK(static_cast<pxSize>(pt) == pxSize(1.5, -2));
    PX_CHECK(pxPoint(pxSize{ 3, 4 }) == pxPoint(3, 4));
    static_assert(!std::is_convertible_v<pxPoint, pxSize> && !std::is_convertible_v<pxSize, pxPoint>);

    // Between precisions: implicit when nothing is lost, explicit (truncating) otherwise.
    pxPoint fromFloat = pxPointf{ 1.5f, 2.5f };
    pxPoint fromInt = pxPointi{ 3, 4 };
    PX_CHECK(fromFloat == pxPoint(1.5, 2.5));
    PX_CHECK(fromInt == pxPoint(3, 4));
    PX_CHECK(pxPointi(pxPoint{ 2.9, -2.9 }) == pxPointi(2, -2));
    static_assert(std::is_convertible_v<pxPointf, pxPoint> && std::is_convertible_v<pxPointi, pxPoint>);
    static_assert(!std::is_convertible_v<pxPoint, pxPointf> && !std::is_convertible_v<pxPoint, pxPointi>);
    static_assert(!std::is_convertible_v<pxPointi, pxPointf> && !std::is_convertible_v<pxPointf, pxPointi>);

    // Integer points wrap around, as in C# (5.2).
    pxPointi big{ INT32_MAX, INT32_MIN };
    PX_CHECK(big + pxSizei(1, 0) == pxPointi(INT32_MIN, INT32_MIN));
    PX_CHECK(big - pxSizei(-1, 1) == pxPointi(INT32_MIN, INT32_MAX));

    // To int32_t out of range: saturated, and NaN is 0 (5.3).
    double nan = std::numeric_limits<double>::quiet_NaN();
    PX_CHECK(pxPointi(pxPoint{ 3e9, -3e9 }) == pxPointi(INT32_MAX, INT32_MIN));
    PX_CHECK(pxPointi(pxPoint{ nan, -2147483648.9 }) == pxPointi(0, INT32_MIN));
    PX_CHECK(pxPointi(pxPointf{ 1e10f, -0.5f }) == pxPointi(INT32_MAX, 0));

}
