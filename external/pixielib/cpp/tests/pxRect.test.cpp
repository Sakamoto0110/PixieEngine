#include <cstdint>
#include <limits>
#include <type_traits>

#include <pixie/pxRect.hpp>

#include "check.hpp"

namespace {

template<typename R, typename P>
concept Movable = requires(R r, P p) { r + p; } || requires(R r, P p) { r - p; };

}  // namespace

void TestRect() {
    pxRect r{ 1, 2, 3, 4 };
    PX_CHECK(r.Right() == 4 && r.Bottom() == 6);
    PX_CHECK(pxRect(pxPoint{ 1, 2 }, pxSize{ 3, 4 }) == r);
    PX_CHECK(pxRect{}.IsEmpty());
    PX_CHECK(!r.IsEmpty());

    // The left and top edges are inside, the right and bottom ones are not.
    PX_CHECK(r.Contains({ 1, 2 }));
    PX_CHECK(r.Contains({ 3.999, 5.999 }));
    PX_CHECK(!r.Contains({ 4, 2 }));
    PX_CHECK(!r.Contains({ 1, 6 }));
    PX_CHECK(!r.Contains({ 0.999, 2 }));
    PX_CHECK(!r.Contains({ 1, 1.999 }));

    pxRecti ri{ 0, 0, 2, 2 };
    PX_CHECK(ri.Contains({ 1, 1 }) && !ri.Contains({ 2, 1 }));

    pxRect fromInt = ri;
    PX_CHECK(fromInt == pxRect(0, 0, 2, 2));
    PX_CHECK(pxRecti(pxRect{ 1.9, 2.9, 3.9, 4.9 }) == pxRecti(1, 2, 3, 4));
    static_assert(std::is_convertible_v<pxRectf, pxRect> && !std::is_convertible_v<pxRect, pxRectf>);

    PX_CHECK(r.Location() == pxPoint(1, 2) && r.Size() == pxSize(3, 4));
    PX_CHECK(r.Center() == pxPoint(2.5, 4));
    PX_CHECK(pxRecti(0, 0, 3, -3).Center() == pxPointi(1, -1));
    PX_CHECK(pxRecti(INT32_MAX - 1, 0, 4, 2).Center() == pxPointi(INT32_MIN, 1));

    // A rectangle moves by a vector, not by a point (8.1); rect + vector is in pxVec.test.cpp.
    static_assert(!Movable<pxRect, pxPoint> && !Movable<pxRecti, pxPointi>);

    // Contains in int32_t computes x + width in 64 bits, so a rectangle near the end works (5.2),
    // while Right wraps around, as in C#.
    pxRecti far{ 2000000000, 0, 200000000, 10 };
    PX_CHECK(far.Contains({ 2000000001, 0 }) && far.Contains({ INT32_MAX, 9 }));
    PX_CHECK(!far.Contains({ 1999999999, 0 }) && !far.Contains({ 2000000001, 10 }));
    PX_CHECK(far.Right() == -2094967296);

    // Contains a rectangle: its edges within these edges, whatever its size.
    PX_CHECK(r.Contains(r) && r.Contains(pxRect(2, 3, 1, 1)) && r.Contains(pxRect(4, 6, 0, 0)));
    PX_CHECK(!r.Contains(pxRect(2, 3, 3, 1)) && !r.Contains(pxRect(0, 3, 1, 1)));
    PX_CHECK(far.Contains(pxRecti(2100000000, 0, 100000000, 10)));

    // Intersect: touching is not sharing, and no area shares nothing.
    pxRect a{ 0, 0, 4, 4 };
    PX_CHECK(pxRect::Intersect(a, pxRect(2, 1, 4, 2)) == pxRect(2, 1, 2, 2));
    PX_CHECK(pxRect::Intersect(a, pxRect(1, 1, 1, 1)) == pxRect(1, 1, 1, 1));
    PX_CHECK(a.IntersectsWith(pxRect(3.5, 3.5, 1, 1)));
    PX_CHECK(!a.IntersectsWith(pxRect(4, 0, 1, 4)) && pxRect::Intersect(a, pxRect(4, 0, 1, 4)).IsEmpty());
    PX_CHECK(!a.IntersectsWith(pxRect(1, 1, 0, 2)) && !a.IntersectsWith(pxRect(1, 1, -1, 2)));
    PX_CHECK(pxRecti::Intersect(far, pxRecti(INT32_MAX - 5, 5, 10, 10)) == pxRecti(INT32_MAX - 5, 5, 10, 5));

    // Union: one with no area adds nothing, so it can start from the empty rectangle.
    PX_CHECK(pxRect::Union(a, pxRect(6, -1, 1, 1)) == pxRect(0, -1, 7, 5));
    PX_CHECK(pxRect::Union(pxRect{}, pxRect(6, 6, 1, 1)) == pxRect(6, 6, 1, 1));
    PX_CHECK(pxRect::Union(a, pxRect(9, 9, 0, 5)) == a);
    PX_CHECK(pxRect::Union(pxRect(9, 9, 0, 5), pxRect(1, 1, -1, 2)) == pxRect(9, 9, 0, 5));

    // With a padding: the size can go negative.
    pxPadding p{ 1, 2, 3, 4 };
    PX_CHECK(a.Deflate(p) == pxRect(1, 2, 0, -2));
    PX_CHECK(a.Inflate(p) == pxRect(-1, -2, 8, 10));
    PX_CHECK(a.Inflate(p).Deflate(p) == a);

    // To int32_t out of range: saturated, and NaN is 0 (5.3).
    PX_CHECK(pxRecti(pxRect{ 3e9, -3e9, std::numeric_limits<double>::quiet_NaN(), 0.5 }) == pxRecti(INT32_MAX, INT32_MIN, 0, 0));
}
