#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

#include "pxColorRgba.hpp"
#include "pxToString.hpp"

// A color in hue (0 to 360), saturation and lightness (0 to 1), in double, with the alpha in a byte,
// last, as in pxColorRgba. There is no implicit conversion to or from pxColorRgba, only the static
// functions (P8.4); the math of both ways lives here, the same as in the C# PxColorHsl.
struct pxColorHsl {
    double h{};
    double s{};
    double l{};
    std::uint8_t a{};

    constexpr pxColorHsl() noexcept = default;
    constexpr pxColorHsl(double hue, double saturation, double lightness, std::uint8_t alpha = 255) noexcept
        : h(hue), s(saturation), l(lightness), a(alpha) {}

    static pxColorHsl FromRgba(pxColorRgba c) noexcept {
        double r = c.r / 255.0;
        double g = c.g / 255.0;
        double b = c.b / 255.0;
        double max = std::max(r, std::max(g, b));
        double min = std::min(r, std::min(g, b));
        double delta = max - min;
        double lightness = (max + min) / 2;
        if (delta == 0)
            return { 0, 0, lightness, c.a };

        double saturation = delta / (1 - std::fabs(2 * lightness - 1));
        double hue;
        if (max == r)
            hue = 60 * std::fmod((g - b) / delta, 6.0);
        else if (max == g)
            hue = 60 * (((b - r) / delta) + 2);
        else
            hue = 60 * (((r - g) / delta) + 4);
        if (hue < 0)
            hue += 360;
        return { hue, saturation, lightness, c.a };
    }

    static pxColorRgba ToRgba(const pxColorHsl& c) noexcept {
        double hue = std::fmod(c.h, 360.0);
        if (hue < 0)
            hue += 360;
        double chroma = (1 - std::fabs(2 * c.l - 1)) * c.s;
        double x = chroma * (1 - std::fabs(std::fmod(hue / 60, 2.0) - 1));
        double m = c.l - chroma / 2;

        double r, g, b;
        if (hue < 60)       { r = chroma; g = x;      b = 0; }
        else if (hue < 120) { r = x;      g = chroma; b = 0; }
        else if (hue < 180) { r = 0;      g = chroma; b = x; }
        else if (hue < 240) { r = 0;      g = x;      b = chroma; }
        else if (hue < 300) { r = x;      g = 0;      b = chroma; }
        else                { r = chroma; g = 0;      b = x; }
        return { ToByte(r + m), ToByte(g + m), ToByte(b + m), c.a };
    }

    constexpr bool IsEmpty() const noexcept { return h == 0 && s == 0 && l == 0 && a == 0; }

    // No hex: the text of an HSL is its fields (docs/notas.md, 4.2).
    std::string ToString() const { return pxTupleString(h, s, l, a); }

    friend constexpr bool operator==(const pxColorHsl&, const pxColorHsl&) noexcept = default;

private:
    // Rounds a half to the even neighbor, like Math.Round in C# (std::nearbyint in the default
    // rounding mode; std::round would take 126.5 to 127, C# to 126). Out of range values, which only
    // come from a saturation or lightness outside 0 to 1, are clamped, and NaN is 0 (5.3).
    static std::uint8_t ToByte(double unit) noexcept {
        if (std::isnan(unit))
            return 0;
        return static_cast<std::uint8_t>(std::nearbyint(std::clamp(unit * 255, 0.0, 255.0)));
    }
};

// The layout is the contract with C# (docs/layout.md): three doubles, the alpha, and seven bytes of
// padding to keep the size a multiple of eight.
static_assert(std::is_standard_layout_v<pxColorHsl> && std::is_trivially_copyable_v<pxColorHsl>);
static_assert(sizeof(pxColorHsl) == 32 && offsetof(pxColorHsl, s) == 8 && offsetof(pxColorHsl, l) == 16 && offsetof(pxColorHsl, a) == 24);
