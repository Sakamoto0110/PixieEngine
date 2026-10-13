#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <pixie/math/pxSegment.hpp>

#include "check.hpp"

namespace {

template<typename S, typename K>
concept PointAtTakes = requires(S s, K t) { s.PointAt(t); };

}  // namespace

void TestSegment() {
    pxSegment s{ pxPoint(1, 2), pxPoint(4, 6) };
    PX_CHECK(s.start == pxPoint(1, 2) && s.end == pxPoint(4, 6));
    PX_CHECK(pxSegment() == pxSegment(pxPoint(), pxPoint()));

    // The direction is end - start, not normalized: its length is the segment's.
    PX_CHECK(s.Direction() == pxVec2(3, 4));
    PX_CHECK(s.Length() == 5 && glm::length(s.Direction()) == s.Length());
    PX_CHECK(pxSegmentf(pxPointf(0, 0), pxPointf(3, 4)).Length() == 5);
    static_assert(std::is_same_v<decltype(s.Direction()), pxVec2> && std::is_same_v<decltype(s.Length()), double>);
    static_assert(std::is_same_v<decltype(pxSegmentf().Length()), float>);

    // PointAt is the Lerp of the two ends, glm::mix's formula.
    PX_CHECK(s.PointAt(0) == s.start && s.PointAt(1) == s.end);
    PX_CHECK(s.PointAt(0.5) == pxPoint(2.5, 4) && s.PointAt(2) == pxPoint(7, 10));
    PX_CHECK(pxSegmentf(pxPointf(7.6f, 0), pxPointf(4.8f, 1)).PointAt(0.22f).x == 6.9839993f);

    // In int32_t, what GLM does with an ivec2 (docs/notas.md, 4.17): PointAt takes a double and
    // truncates, as glm::mix, and the length is a double (8.3).
    pxSegmenti si{ pxPointi(1, -2), pxPointi(4, -5) };
    PX_CHECK(si.Direction() == pxVec2i(3, -3));
    PX_CHECK(si.PointAt(0.5) == pxPointi(2, -3));
    PX_CHECK(si.Length() == 4.242640687119285);
    static_assert(std::is_same_v<decltype(si.Length()), double>);
    static_assert(PointAtTakes<pxSegmenti, double> && PointAtTakes<pxSegment, int>);
    static_assert(PointAtTakes<pxSegmentf, float> && !PointAtTakes<pxSegmentf, double>);
    // The length takes the difference in 64 bits; the direction wraps around, as a point does (5.2).
    pxSegmenti far{ pxPointi(INT32_MIN, 0), pxPointi(INT32_MAX, 0) };
    PX_CHECK(far.Length() == 4294967295.0 && far.Direction() == pxVec2i(-1, 0));

    // Between precisions: implicit when nothing is lost, explicit (truncating) otherwise.
    pxSegment fromInt = si;
    PX_CHECK(fromInt == pxSegment(pxPoint(1, -2), pxPoint(4, -5)));
    PX_CHECK(pxSegmenti(pxSegment(pxPoint(2.9, -2.9), pxPoint(3e9, 0))) == pxSegmenti(pxPointi(2, -2), pxPointi(INT32_MAX, 0)));
    static_assert(std::is_convertible_v<pxSegmentf, pxSegment> && std::is_convertible_v<pxSegmenti, pxSegment>);
    static_assert(!std::is_convertible_v<pxSegment, pxSegmentf> && !std::is_convertible_v<pxSegmentf, pxSegmenti>);

    // The text: the two points, in the order of the fields.
    PX_CHECK(s.ToString() == "(1, 2, 4, 6)");
    PX_CHECK(pxSegmentf(pxPointf(0.1f, -1.5f), pxPointf(1e20f, 0)).ToString() == "(0.1, -1.5, 100000000000000000000, 0)");

    // The layout, the contract with C# (docs/layout.md).
    PX_CHECK(sizeof(pxSegment) == 32 && offsetof(pxSegment, end) == 16 && offsetof(pxSegmenti, end) == 8);
}
