#include <cstdint>
#include <limits>
#include <type_traits>

#include <pixie/pxSize.hpp>

#include "check.hpp"

namespace {

template<typename S, typename K>
concept Multipliable = requires(S s, K k) { s * k; } || requires(S s, K k) { k * s; } ||
                       requires(S s, K k) { s / k; };

}  // namespace

void TestSize() {
    pxSize sz{ 3, 4.5 };
    PX_CHECK(sz.width == 3 && sz.height == 4.5);
    PX_CHECK(pxSize{}.IsEmpty());
    PX_CHECK(!sz.IsEmpty());

    PX_CHECK(sz + pxSize(1, 1) == pxSize(4, 5.5));
    PX_CHECK(sz - pxSize(1, 1) == pxSize(2, 3.5));
    PX_CHECK(sz * 2 == pxSize(6, 9));
    PX_CHECK(2 * sz == pxSize(6, 9));
    PX_CHECK(sz / 2 == pxSize(1.5, 2.25));
    PX_CHECK(+sz == sz);
    PX_CHECK(-sz == pxSize(-3, -4.5));

    // Between precisions: implicit when nothing is lost, explicit (truncating) otherwise.
    pxSize fromFloat = pxSizef{ 1.5f, 2.5f };
    pxSize fromInt = pxSizei{ 3, 4 };
    PX_CHECK(fromFloat == pxSize(1.5, 2.5));
    PX_CHECK(fromInt == pxSize(3, 4));
    PX_CHECK(pxSizei(pxSize{ 2.9, -2.9 }) == pxSizei(2, -2));
    static_assert(std::is_convertible_v<pxSizef, pxSize> && std::is_convertible_v<pxSizei, pxSize>);
    static_assert(!std::is_convertible_v<pxSize, pxSizef> && !std::is_convertible_v<pxSize, pxSizei>);
    static_assert(!std::is_convertible_v<pxSizei, pxSizef> && !std::is_convertible_v<pxSizef, pxSizei>);
    static_assert(std::is_constructible_v<pxSizef, pxSize> && std::is_constructible_v<pxSizei, pxSize>);

    // The same rules as pxPoint_t (5.1 to 5.3).
    PX_CHECK(pxSizei(INT32_MAX, 1) + pxSizei(1, 1) == pxSizei(INT32_MIN, 2));
    PX_CHECK(-pxSizei(INT32_MIN, 0) == pxSizei(INT32_MIN, 0));
    PX_CHECK(pxSizei(pxSize{ -1e300, std::numeric_limits<double>::quiet_NaN() }) == pxSizei(INT32_MIN, 0));
    static_assert(!Multipliable<pxSizei, double> && Multipliable<pxSize, int>);
}
