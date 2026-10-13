#pragma once

#include <cstdint>
#include <type_traits>

// The precisions a primitive comes in. The unsuffixed alias is double, then f for float and i for
// int32_t (docs/notas.md, 4.1). The layout of each one is part of the contract with C#
// (docs/layout.md), so no other type is accepted.
template<typename T>
inline constexpr bool pxIsPrecision =
    std::is_same_v<T, double> || std::is_same_v<T, float> || std::is_same_v<T, std::int32_t>;

// A conversion between precisions is implicit only when nothing is lost (P8.5): a float and an
// int32_t fit in a double. Every other one is explicit and goes through pxConvert.
template<typename From, typename To>
inline constexpr bool pxIsLossless =
    std::is_same_v<From, To> ||
    (std::is_same_v<To, double> && (std::is_same_v<From, float> || std::is_same_v<From, std::int32_t>));

// The scalars a primitive in T multiplies and divides by: the ones C# converts to T implicitly (5.1).
// Any integer goes to float and double, a float goes to double, and only the integers that fit go to
// int32_t. pxPointi * 0.5 does not compile, as in C#, instead of becoming pxPointi * 0.
template<typename K, typename T>
inline constexpr bool pxIsImplicitScalar =
    std::is_same_v<K, T> ||
    (std::is_integral_v<K> && !std::is_same_v<K, bool> && std::is_floating_point_v<T>) ||
    (std::is_same_v<K, float> && std::is_same_v<T, double>) ||
    (std::is_integral_v<K> && !std::is_same_v<K, bool> && std::is_same_v<T, std::int32_t> &&
     (sizeof(K) < 4 || (sizeof(K) == 4 && std::is_signed_v<K>)));

// A scalar that would convert silently in C++ but not in C#; the deleted operators take it.
template<typename K, typename T>
concept pxRejectedScalar = std::is_arithmetic_v<K> && !pxIsImplicitScalar<K, T>;

// The arithmetic of the primitives (5.2). In int32_t it wraps around, as in C# (unchecked, the
// default): the sum goes through uint32_t, whose overflow is defined, and comes back modulo 2^32,
// which C++20 defines too. In float and double it is the plain operation.
template<typename T>
constexpr T pxAdd(T a, T b) noexcept {
    if constexpr (std::is_same_v<T, std::int32_t>)
        return static_cast<std::int32_t>(static_cast<std::uint32_t>(a) + static_cast<std::uint32_t>(b));
    else
        return a + b;
}

template<typename T>
constexpr T pxSub(T a, T b) noexcept {
    if constexpr (std::is_same_v<T, std::int32_t>)
        return static_cast<std::int32_t>(static_cast<std::uint32_t>(a) - static_cast<std::uint32_t>(b));
    else
        return a - b;
}

template<typename T>
constexpr T pxMul(T a, T b) noexcept {
    if constexpr (std::is_same_v<T, std::int32_t>)
        return static_cast<std::int32_t>(static_cast<std::uint32_t>(a) * static_cast<std::uint32_t>(b));
    else
        return a * b;
}

template<typename T>
constexpr T pxNeg(T a) noexcept {
    if constexpr (std::is_same_v<T, std::int32_t>)
        return static_cast<std::int32_t>(0u - static_cast<std::uint32_t>(a));
    else
        return -a;
}

// Integer division by zero, and INT32_MIN / -1, are a precondition (5.4): C++ leaves them undefined
// and C# throws, as for a plain int.

// The type the edges of a rectangle are computed in: int64_t for int32_t, so that x + width never
// overflows (5.2), and T itself otherwise. A static_cast back to int32_t keeps the low 32 bits, the
// same wrap as pxAdd.
template<typename T>
using pxWide_t = std::conditional_t<std::is_same_v<T, std::int32_t>, std::int64_t, T>;

// The conversion between precisions (5.3). From float or double to int32_t it truncates toward zero,
// saturates out of range and takes NaN to 0, as C# does from .NET 9 on; written here, and in C#, so
// that every compiler and runtime gives the same number. A plain static_cast would be undefined
// behavior out of range. The others are a static_cast: double to float rounds, to the nearest.
template<typename To, typename From>
constexpr To pxConvert(From value) noexcept {
    if constexpr (std::is_same_v<To, std::int32_t> && std::is_floating_point_v<From>) {
        double v = static_cast<double>(value);
        if (v != v)
            return 0;
        if (v >= 2147483648.0)
            return INT32_MAX;
        if (v <= -2147483649.0)
            return INT32_MIN;
        return static_cast<std::int32_t>(v);
    } else {
        return static_cast<To>(value);
    }
}
