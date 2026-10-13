#include <initializer_list>
#include <cstdint>
#include <type_traits>

#include <pixie/pxRegion.hpp>

#include "check.hpp"

void TestRegion() {
    pxRegion rg{ 1, 2, 4, 6 };
    PX_CHECK(rg.Width() == 3 && rg.Height() == 4);
    PX_CHECK(pxRegion{}.IsEmpty());
    PX_CHECK(!rg.IsEmpty());

    // The same thing as a rectangle, converted explicitly both ways.
    PX_CHECK(pxRegion(pxRect{ 1, 2, 3, 4 }) == rg);
    PX_CHECK(static_cast<pxRect>(rg) == pxRect(1, 2, 3, 4));
    static_assert(!std::is_convertible_v<pxRect, pxRegion> && !std::is_convertible_v<pxRegion, pxRect>);

    // The same edges as pxRect_t::Contains.
    pxRect r{ 1, 2, 3, 4 };
    for (pxPoint pt : { pxPoint{ 1, 2 }, pxPoint{ 3.999, 5.999 }, pxPoint{ 4, 2 }, pxPoint{ 1, 6 }, pxPoint{ 0.5, 3 } })
        PX_CHECK(rg.Contains(pt) == r.Contains(pt));
    PX_CHECK(rg.Contains({ 1, 2 }) && !rg.Contains({ 4, 2 }) && !rg.Contains({ 1, 6 }));

    pxRegion fromInt = pxRegioni{ 0, 0, 2, 2 };
    PX_CHECK(fromInt == pxRegion(0, 0, 2, 2));
    static_assert(std::is_convertible_v<pxRegioni, pxRegion> && !std::is_convertible_v<pxRegion, pxRegioni>);

    // The operations of pxRect_t, with the same rules.
    pxRegion a{ 0, 0, 4, 4 };
    PX_CHECK(a.Contains(a) && a.Contains(pxRegion(1, 1, 2, 2)) && a.Contains(pxRegion(4, 4, 4, 4)));
    PX_CHECK(!a.Contains(pxRegion(1, 1, 5, 2)));
    PX_CHECK(pxRegion::Intersect(a, pxRegion(2, 1, 6, 3)) == pxRegion(2, 1, 4, 3));
    PX_CHECK(!a.IntersectsWith(pxRegion(4, 0, 5, 4)) && pxRegion::Intersect(a, pxRegion(4, 0, 5, 4)).IsEmpty());
    PX_CHECK(!a.IntersectsWith(pxRegion(3, 3, 1, 1)));
    PX_CHECK(pxRegion::Union(a, pxRegion(6, -1, 7, 0)) == pxRegion(0, -1, 7, 4));
    PX_CHECK(pxRegion::Union(pxRegion{}, pxRegion(6, 6, 7, 7)) == pxRegion(6, 6, 7, 7));

    // The same answers as pxRect_t, converted.
    pxRect ra{ 0, 0, 4, 4 }, rb{ 2, 1, 4, 2 };
    PX_CHECK(pxRegion(pxRect::Intersect(ra, rb)) == pxRegion::Intersect(pxRegion(ra), pxRegion(rb)));
    PX_CHECK(pxRegion(pxRect::Union(ra, rb)) == pxRegion::Union(pxRegion(ra), pxRegion(rb)));

    // In int32_t the size wraps around (5.2).
    PX_CHECK(pxRegioni(INT32_MIN, 0, INT32_MAX, 0).Width() == -1);
}
