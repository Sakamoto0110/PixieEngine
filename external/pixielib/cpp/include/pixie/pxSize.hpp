#pragma once

#include <cstddef>
#include <string>
#include <type_traits>

#include "pxPrecision.hpp"
#include "pxToString.hpp"

// A width and a height.
template<typename T>
struct pxSize_t {
    static_assert(pxIsPrecision<T>, "pxSize_t comes in double, float or int32_t");

    T width{};
    T height{};

    constexpr pxSize_t() noexcept = default;
    constexpr pxSize_t(T w, T h) noexcept : width(w), height(h) {}

    template<typename U>
    constexpr explicit(!pxIsLossless<U, T>) pxSize_t(const pxSize_t<U>& o) noexcept
        : width(pxConvert<T>(o.width)), height(pxConvert<T>(o.height)) {}

    constexpr bool IsEmpty() const noexcept { return width == 0 && height == 0; }

    std::string ToString() const { return pxTupleString(width, height); }

    friend constexpr bool operator==(const pxSize_t&, const pxSize_t&) noexcept = default;

    // In int32_t the arithmetic wraps around, as in C# (5.2), and dividing by zero is a precondition
    // (5.4). The scalar is a T, or a type that C# converts to T implicitly (5.1).
    friend constexpr pxSize_t operator+(pxSize_t a, pxSize_t b) noexcept { return { pxAdd(a.width, b.width), pxAdd(a.height, b.height) }; }
    friend constexpr pxSize_t operator-(pxSize_t a, pxSize_t b) noexcept { return { pxSub(a.width, b.width), pxSub(a.height, b.height) }; }
    friend constexpr pxSize_t operator*(pxSize_t sz, T k) noexcept { return { pxMul(sz.width, k), pxMul(sz.height, k) }; }
    friend constexpr pxSize_t operator*(T k, pxSize_t sz) noexcept { return sz * k; }
    friend constexpr pxSize_t operator/(pxSize_t sz, T k) noexcept { return { sz.width / k, sz.height / k }; }
    friend constexpr pxSize_t operator+(pxSize_t sz) noexcept { return sz; }
    friend constexpr pxSize_t operator-(pxSize_t sz) noexcept { return { pxNeg(sz.width), pxNeg(sz.height) }; }

    template<pxRejectedScalar<T> K> friend pxSize_t operator*(pxSize_t, K) = delete;
    template<pxRejectedScalar<T> K> friend pxSize_t operator*(K, pxSize_t) = delete;
    template<pxRejectedScalar<T> K> friend pxSize_t operator/(pxSize_t, K) = delete;
};

using pxSize  = pxSize_t<double>;
using pxSizef = pxSize_t<float>;
using pxSizei = pxSize_t<std::int32_t>;

// The layout is the contract with C# (docs/layout.md).
static_assert(std::is_standard_layout_v<pxSize> && std::is_trivially_copyable_v<pxSize>);
static_assert(sizeof(pxSize) == 16 && offsetof(pxSize, height) == 8);
static_assert(sizeof(pxSizef) == 8 && offsetof(pxSizef, height) == 4);
static_assert(sizeof(pxSizei) == 8 && offsetof(pxSizei, height) == 4);
