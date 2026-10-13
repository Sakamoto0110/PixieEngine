#include <cstdint>
#include <type_traits>

#include <pixie/pxPadding.hpp>

#include "check.hpp"

void TestPadding() {
    pxPadding p{ 1, 2, 3, 4 };
    PX_CHECK(p.left == 1 && p.top == 2 && p.right == 3 && p.bottom == 4);
    PX_CHECK(p.Horizontal() == 4 && p.Vertical() == 6);
    PX_CHECK(pxPadding(5) == pxPadding(5, 5, 5, 5));
    PX_CHECK(pxPadding{}.IsEmpty());
    PX_CHECK(!p.IsEmpty());

    // A single value does not become a padding by accident.
    static_assert(!std::is_convertible_v<double, pxPadding>);

    pxPadding fromInt = pxPaddingi{ 1, 2, 3, 4 };
    PX_CHECK(fromInt == p);
    static_assert(std::is_convertible_v<pxPaddingf, pxPadding> && !std::is_convertible_v<pxPadding, pxPaddingf>);

    // The sums wrap around in int32_t (5.2).
    PX_CHECK(pxPaddingi(INT32_MAX, 0, 1, 0).Horizontal() == INT32_MIN);
}
