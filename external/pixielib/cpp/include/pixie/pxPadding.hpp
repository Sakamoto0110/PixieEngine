#pragma once

#include <cstddef>
#include <string>
#include <type_traits>

#include "pxPrecision.hpp"
#include "pxToString.hpp"

// Space around something, one value per side, in the same order as pxRegion_t: left, top, right,
// bottom.
template<typename T>
struct pxPadding_t {
    static_assert(pxIsPrecision<T>, "pxPadding_t comes in double, float or int32_t");

    T left{};
    T top{};
    T right{};
    T bottom{};

    constexpr pxPadding_t() noexcept = default;
    constexpr explicit pxPadding_t(T all) noexcept : left(all), top(all), right(all), bottom(all) {}
    constexpr pxPadding_t(T l, T t, T r, T b) noexcept : left(l), top(t), right(r), bottom(b) {}

    template<typename U>
    constexpr explicit(!pxIsLossless<U, T>) pxPadding_t(const pxPadding_t<U>& o) noexcept
        : left(pxConvert<T>(o.left)), top(pxConvert<T>(o.top)),
          right(pxConvert<T>(o.right)), bottom(pxConvert<T>(o.bottom)) {}

    // The space taken across and along. In int32_t the sum wraps around, as in C# (5.2).
    constexpr T Horizontal() const noexcept { return pxAdd(left, right); }
    constexpr T Vertical() const noexcept { return pxAdd(top, bottom); }

    constexpr bool IsEmpty() const noexcept { return left == 0 && top == 0 && right == 0 && bottom == 0; }

    std::string ToString() const { return pxTupleString(left, top, right, bottom); }

    friend constexpr bool operator==(const pxPadding_t&, const pxPadding_t&) noexcept = default;
};

using pxPadding  = pxPadding_t<double>;
using pxPaddingf = pxPadding_t<float>;
using pxPaddingi = pxPadding_t<std::int32_t>;

// The layout is the contract with C# (docs/layout.md).
static_assert(std::is_standard_layout_v<pxPadding> && std::is_trivially_copyable_v<pxPadding>);
static_assert(sizeof(pxPadding) == 32 && offsetof(pxPadding, top) == 8 && offsetof(pxPadding, right) == 16 && offsetof(pxPadding, bottom) == 24);
static_assert(sizeof(pxPaddingf) == 16 && offsetof(pxPaddingf, bottom) == 12);
static_assert(sizeof(pxPaddingi) == 16 && offsetof(pxPaddingi, bottom) == 12);
