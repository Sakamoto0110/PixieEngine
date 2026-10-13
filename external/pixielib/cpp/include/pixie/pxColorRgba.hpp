#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

#include "pxToString.hpp"

// A color in bytes, in memory as R, G, B, A: the order of GL_RGBA with GL_UNSIGNED_BYTE, so a color,
// an array of vertex colors or the pixels of a texture go to OpenGL as they are (docs/notas.md, 4.2).
// The order in memory and the order of a hex number are not the same thing: read as a uint32_t on a
// little-endian machine these bytes are 0xAABBGGRR, so the hex conversions shift bits.
struct pxColorRgba {
    std::uint8_t r{};
    std::uint8_t g{};
    std::uint8_t b{};
    std::uint8_t a{};

    constexpr pxColorRgba() noexcept = default;
    constexpr pxColorRgba(std::uint8_t red, std::uint8_t green, std::uint8_t blue, std::uint8_t alpha = 255) noexcept
        : r(red), g(green), b(blue), a(alpha) {}

    // The hex number as it is written, 0xRRGGBBAA, like #RRGGBBAA in CSS and GetColor in raylib.
    static constexpr pxColorRgba FromHex(std::uint32_t rrggbbaa) noexcept {
        return { static_cast<std::uint8_t>(rrggbbaa >> 24), static_cast<std::uint8_t>(rrggbbaa >> 16),
                 static_cast<std::uint8_t>(rrggbbaa >> 8), static_cast<std::uint8_t>(rrggbbaa) };
    }

    constexpr std::uint32_t ToHex() const noexcept {
        return (std::uint32_t{ r } << 24) | (std::uint32_t{ g } << 16) | (std::uint32_t{ b } << 8) | std::uint32_t{ a };
    }

    constexpr bool IsEmpty() const noexcept { return r == 0 && g == 0 && b == 0 && a == 0; }

    std::string ToString() const { return pxTupleString(r, g, b, a); }

    friend constexpr bool operator==(const pxColorRgba&, const pxColorRgba&) noexcept = default;
};

// The layout is the contract with C# (docs/layout.md).
static_assert(std::is_standard_layout_v<pxColorRgba> && std::is_trivially_copyable_v<pxColorRgba>);
static_assert(sizeof(pxColorRgba) == 4 && offsetof(pxColorRgba, g) == 1 && offsetof(pxColorRgba, b) == 2 && offsetof(pxColorRgba, a) == 3);
