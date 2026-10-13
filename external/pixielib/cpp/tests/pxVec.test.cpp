#include <cstdint>
#include <cstring>
#include <new>
#include <type_traits>

#include <pixie/math/pxVec.hpp>

#include "check.hpp"

namespace {

template<typename V, typename K>
concept Multipliable = requires(V v, K k) { v * k; };

template<typename A, typename B>
concept Addable = requires(A a, B b) { a + b; };

}  // namespace

void TestVec() {
    // A vector starts at zero, as in C# (GLM_FORCE_CTOR_INIT, from pixie::math), even on memory that
    // held something else.
    alignas(pxVec3) unsigned char memory[sizeof(pxVec3)];
    std::memset(memory, 0xFF, sizeof memory);
    PX_CHECK(*new (memory) pxVec3 == pxVec3(0, 0, 0));
    PX_CHECK(pxVec4i() == pxVec4i(0));

    pxVec3 a{ 1, 2, 3 };
    pxVec3 b{ 4, 5, 6 };
    PX_CHECK(a + b == pxVec3(5, 7, 9));
    PX_CHECK(b - a == pxVec3(3, 3, 3));
    PX_CHECK(a * b == pxVec3(4, 10, 18));
    PX_CHECK(a * 2.0 == pxVec3(2, 4, 6) && 2.0 * a == pxVec3(2, 4, 6));
    PX_CHECK(b / 2.0 == pxVec3(2, 2.5, 3));
    PX_CHECK(-a == pxVec3(-1, -2, -3));

    // The math is GLM's; these are the values the C# tests use too.
    PX_CHECK(glm::dot(a, b) == 32);
    PX_CHECK(glm::cross(pxVec3(1, 0, 0), pxVec3(0, 1, 0)) == pxVec3(0, 0, 1));
    PX_CHECK(glm::cross(a, b) == pxVec3(-3, 6, -3));
    PX_CHECK(glm::length(pxVec3(3, 4, 0)) == 5 && glm::distance(a, b) == glm::length(b - a));
    // GLM multiplies by 1 / length, so 3 / 5 comes out one unit in the last place off 0.6; the
    // results of a library are the library's (docs/layout.md, 3).
    PX_CHECK(glm::length(glm::normalize(pxVec3(3, 4, 0)) - pxVec3(0.6, 0.8, 0)) < 1e-15);
    PX_CHECK(glm::normalize(pxVec3(0, 0, 2)) == pxVec3(0, 0, 1));
    PX_CHECK(glm::normalize(pxVec3(3, 4, 0)).x == 3 * (1 / 5.0));

    // Two float results that System.Numerics would give one bit off (Vector2.Normalize divides,
    // Vector2.Lerp rounds in another order); the C# tests expect these same bits.
    PX_CHECK(glm::normalize(pxVec2f(11, 8)).x == 0.808736f);
    PX_CHECK(glm::mix(pxVec2f(7.6f), pxVec2f(4.8f), 0.22f).x == 6.9839993f);
    PX_CHECK(glm::mix(pxVec3(0, 0, 0), pxVec3(10, 20, 30), 0.25) == pxVec3(2.5, 5, 7.5));
    PX_CHECK(glm::min(a, pxVec3(2, 1, 3)) == pxVec3(1, 1, 3) && glm::max(a, pxVec3(2, 1, 3)) == pxVec3(2, 2, 3));
    PX_CHECK(glm::min(pxVec3(3, 2, 1), pxVec3(1, 2, 3)) == pxVec3(1, 2, 1));
    PX_CHECK(glm::max(pxVec3(3, 2, 1), pxVec3(1, 2, 3)) == pxVec3(3, 2, 3));
    PX_CHECK(glm::min(pxVec3i(3, 2, 1), pxVec3i(1, 2, 3)) == pxVec3i(1, 2, 1));
    PX_CHECK(glm::clamp(pxVec3(-1, 5, 2), pxVec3(0), pxVec3(3)) == pxVec3(0, 3, 2));
    PX_CHECK(glm::abs(pxVec3i(-1, 0, 2)) == pxVec3i(1, 0, 2));
    PX_CHECK(pxVec2f(1, 2) + pxVec2f(3, 4) == pxVec2f(4, 6));
    PX_CHECK(pxVec4(1, 2, 3, 4).w == 4 && glm::dot(pxVec4(1, 2, 3, 4), pxVec4(1)) == 10);

    // GLM's vec4 dot adds in pairs, (x + y) + (z + w), and so does the C# one; with 1e16, adding 1 is
    // lost, and adding in sequence would give 1. On MSVC, GLM adds in sequence (docs/layout.md, 3).
#if defined(_MSC_VER)
    PX_CHECK(glm::dot(pxVec4(1e16, 1, -1e16, 1), pxVec4(1)) == 1);
#else
    PX_CHECK(glm::dot(pxVec4(1e16, 1, -1e16, 1), pxVec4(1)) == 0);
    PX_CHECK(glm::dot(pxVec4f(1e8f, 1, -1e8f, 1), pxVec4f(1)) == 0);
#endif

    // A conversion between precisions is explicit, even the one that loses nothing
    // (GLM_FORCE_EXPLICIT_CTOR, from pixie::math), and truncates.
    static_assert(!std::is_convertible_v<pxVec3f, pxVec3> && !std::is_convertible_v<pxVec3, pxVec3i>);
    static_assert(std::is_constructible_v<pxVec3, pxVec3f> && std::is_constructible_v<pxVec3i, pxVec3>);
    PX_CHECK(pxVec3i(pxVec3(2.9, -2.9, 0.5)) == pxVec3i(2, -2, 0));

    // The scalar has to be of the vector's own type: GLM is stricter than C#, where PxVec3 * 2 works.
    static_assert(Multipliable<pxVec3, double> && !Multipliable<pxVec3, int>);
    static_assert(Multipliable<pxVec3i, int> && !Multipliable<pxVec3i, double> && !Multipliable<pxVec3f, double>);

    // A point and a vec2 convert to each other, explicitly, in the same precision (6.6).
    PX_CHECK(pxToVec2(pxPoint(1.5, -2)) == pxVec2(1.5, -2) && pxToPoint(pxVec2i(3, 4)) == pxPointi(3, 4));
    PX_CHECK(pxToPoint(pxToVec2(pxPointf(0.1f, 7))) == pxPointf(0.1f, 7));
    static_assert(!std::is_convertible_v<pxPoint, pxVec2> && !std::is_convertible_v<pxVec2, pxPoint>);
    static_assert(std::is_same_v<decltype(pxToVec2(pxPointi())), pxVec2i>);

    // A point and a vector (8.1): point - point is the vector between them, and a point plus or minus
    // a vector is a point. Nothing converts on the way, so point + point still does not compile, nor
    // does vector + point.
    pxPoint p{ 1.5, -2 };
    PX_CHECK(pxPoint(4, 6) - pxPoint(1, 2) == pxVec2(3, 4));
    static_assert(std::is_same_v<decltype(pxPointi() - pxPointi()), pxVec2i>);
    static_assert(std::is_same_v<decltype(pxPointf() - pxPointf()), pxVec2f>);
    PX_CHECK(p + pxVec2(1, 1) == pxPoint(2.5, -1) && p - pxVec2(1, 1) == pxPoint(0.5, -3));
    PX_CHECK(pxPoint(1, 2) + (pxPoint(4, 6) - pxPoint(1, 2)) == pxPoint(4, 6));
    PX_CHECK(pxPointf(1, 2) + pxVec2f(0.5f, 0.5f) == pxPointf(1.5f, 2.5f));
    static_assert(!Addable<pxPoint, pxPoint> && !Addable<pxVec2, pxPoint> && !Addable<pxPoint, pxVec2f>);
    static_assert(!Addable<pxPoint, pxVec2i> && Addable<pxPoint, pxVec2> && Addable<pxPointi, pxVec2i>);
    // In int32_t the point wraps around, as in C# (5.2), the difference too.
    pxPointi big{ INT32_MAX, INT32_MIN };
    PX_CHECK(big + pxVec2i(1, -1) == pxPointi(INT32_MIN, INT32_MAX));
    PX_CHECK(big - pxVec2i(-1, 1) == pxPointi(INT32_MIN, INT32_MAX));
    PX_CHECK(big - pxPointi(-1, 1) == pxVec2i(INT32_MIN, INT32_MAX));

    // A rectangle moves by a vector, and the size stays (8.1).
    pxRect r{ 1, 2, 3, 4 };
    PX_CHECK(r + pxVec2(1, -1) == pxRect(2, 1, 3, 4) && r - pxVec2(1, -1) == pxRect(0, 3, 3, 4));
    PX_CHECK(pxRecti(INT32_MAX, 0, 1, 1) + pxVec2i(1, 0) == pxRecti(INT32_MIN, 0, 1, 1));

    // The length of an integer vec2, which GLM refuses (8.3): in double, and squared in int64_t. The
    // C# tests expect the same values.
    PX_CHECK(pxLength(pxVec2i(3, 4)) == 5 && pxLengthSquared(pxVec2i(3, 4)) == 25);
    PX_CHECK(pxLength(pxVec2i(3, 1)) == 3.1622776601683795 && pxLengthSquared(pxVec2i(3, -1)) == 10);
    static_assert(std::is_same_v<decltype(pxLength(pxVec2i())), double>);
    static_assert(std::is_same_v<decltype(pxLengthSquared(pxVec2i())), std::int64_t>);
    // 64 bits, where int32_t would wrap: the square of INT32_MIN is 2^62.
    PX_CHECK(pxLengthSquared(pxVec2i(INT32_MIN, 0)) == 4611686018427387904);
    // The one length whose square passes INT64_MAX: 2^63 wraps around (5.2), the length does not.
    PX_CHECK(pxLengthSquared(pxVec2i(INT32_MIN, INT32_MIN)) == INT64_MIN);
    PX_CHECK(pxLength(pxVec2i(INT32_MIN, INT32_MIN)) == 3037000499.97605);
    // The distance takes the difference in 64 bits: from INT32_MIN to INT32_MAX is 2^32 - 1, which
    // int32_t would wrap to -1. Its square passes INT64_MAX and wraps.
    PX_CHECK(pxDistance(pxVec2i(INT32_MIN, 0), pxVec2i(INT32_MAX, 0)) == 4294967295.0);
    PX_CHECK(pxDistanceSquared(pxVec2i(INT32_MIN, 0), pxVec2i(INT32_MAX, 0)) == -8589934591);
    PX_CHECK(pxDistance(pxVec2i(1, 1), pxVec2i(4, 5)) == 5 && pxDistanceSquared(pxVec2i(4, 5), pxVec2i(1, 1)) == 25);
    // Beyond 2^53 each square rounds in double: 94906267^2 is odd and above it.
    PX_CHECK(pxLength(pxVec2i(94906267, 0)) == 94906267);
    PX_CHECK(pxDistance(pxVec2i(0, 0), pxVec2i(-123456789, 987654321)) == 995340462.62581);
    // And the root of the double sum, not of the exact one, which would be 2392612263.308584: the
    // same as C#, which has no wider type to compute in.
    PX_CHECK(pxLength(pxVec2i(1721854523, -1661267722)) == 2392612263.3085847);

    // The text, the same as the primitives'.
    PX_CHECK(pxToString(pxVec3(1.5, -2, 0)) == "(1.5, -2, 0)");
    PX_CHECK(pxToString(pxVec2f(0.1f, 1e20f)) == "(0.1, 100000000000000000000)");
    PX_CHECK(pxToString(pxVec4i(1, -2, 3, 2147483647)) == "(1, -2, 3, 2147483647)");
}
