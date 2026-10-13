#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <limits>

#include <pixie/pxColorHsl.hpp>

#include "check.hpp"

namespace {

bool Near(const pxColorHsl& a, const pxColorHsl& b) {
    return std::fabs(a.h - b.h) < 1e-9 && std::fabs(a.s - b.s) < 1e-9 && std::fabs(a.l - b.l) < 1e-9 && a.a == b.a;
}

}  // namespace

void TestColorHsl() {
    PX_CHECK(pxColorHsl(10, 0.5, 0.5) == pxColorHsl(10, 0.5, 0.5, 255));
    PX_CHECK(pxColorHsl{}.IsEmpty());

    // The primaries, white, black and a gray, both ways.
    PX_CHECK(Near(pxColorHsl::FromRgba({ 255, 0, 0 }), { 0, 1, 0.5 }));
    PX_CHECK(Near(pxColorHsl::FromRgba({ 0, 255, 0 }), { 120, 1, 0.5 }));
    PX_CHECK(Near(pxColorHsl::FromRgba({ 0, 0, 255 }), { 240, 1, 0.5 }));
    PX_CHECK(Near(pxColorHsl::FromRgba({ 255, 0, 255 }), { 300, 1, 0.5 }));
    PX_CHECK(Near(pxColorHsl::FromRgba({ 255, 255, 255 }), { 0, 0, 1 }));
    PX_CHECK(Near(pxColorHsl::FromRgba({ 0, 0, 0, 7 }), { 0, 0, 0, 7 }));
    PX_CHECK(Near(pxColorHsl::FromRgba({ 128, 128, 128 }), { 0, 0, 128 / 255.0 }));

    PX_CHECK(pxColorHsl::ToRgba({ 0, 1, 0.5 }) == pxColorRgba(255, 0, 0));
    PX_CHECK(pxColorHsl::ToRgba({ 120, 1, 0.5 }) == pxColorRgba(0, 255, 0));
    PX_CHECK(pxColorHsl::ToRgba({ 240, 1, 0.5, 9 }) == pxColorRgba(0, 0, 255, 9));
    PX_CHECK(pxColorHsl::ToRgba({ 0, 0, 1 }) == pxColorRgba(255, 255, 255));

    // The hue wraps around 360, in both directions.
    PX_CHECK(pxColorHsl::ToRgba({ 420, 1, 0.5 }) == pxColorRgba(255, 255, 0));
    PX_CHECK(pxColorHsl::ToRgba({ -60, 1, 0.5 }) == pxColorRgba(255, 0, 255));

    // Every color survives the round trip.
    bool roundTrip = true;
    for (int v = 0; v < 256; v += 5)
        for (pxColorRgba c : { pxColorRgba(255, static_cast<std::uint8_t>(v), 0), pxColorRgba(static_cast<std::uint8_t>(v), 40, 200),
                               pxColorRgba(static_cast<std::uint8_t>(v), static_cast<std::uint8_t>(v), static_cast<std::uint8_t>(v), 3) })
            roundTrip = roundTrip && pxColorHsl::ToRgba(pxColorHsl::FromRgba(c)) == c;
    PX_CHECK(roundTrip);

    // A half rounds to the even neighbor, like Math.Round in C#: 126.5 is 126, 127.5 is 128.
    PX_CHECK(pxColorHsl::ToRgba({ 0, 0, 126.5 / 255 }).r == 126);

    // NaN becomes 0 (5.3).
    PX_CHECK(pxColorHsl::ToRgba({ 0, 0, std::numeric_limits<double>::quiet_NaN(), 7 }) == pxColorRgba(0, 0, 0, 7));
    PX_CHECK(pxColorHsl::ToRgba({ 0, 0, 127.5 / 255 }).r == 128);

    // Out of range lightness is clamped instead of overflowing the byte.
    PX_CHECK(pxColorHsl::ToRgba({ 0, 0, 1.5 }) == pxColorRgba(255, 255, 255));
    PX_CHECK(pxColorHsl::ToRgba({ 0, 0, -0.5 }) == pxColorRgba(0, 0, 0));
}
