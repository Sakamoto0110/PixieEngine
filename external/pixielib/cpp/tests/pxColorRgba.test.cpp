#include <cstdint>
#include <cstring>

#include <pixie/pxColorRgba.hpp>

#include "check.hpp"

void TestColorRgba() {
    pxColorRgba c{ 0x11, 0x22, 0x33, 0x44 };
    PX_CHECK(pxColorRgba(1, 2, 3) == pxColorRgba(1, 2, 3, 255));
    PX_CHECK(pxColorRgba{}.IsEmpty());
    PX_CHECK(!c.IsEmpty());

    // In memory: R, G, B, A, the order OpenGL reads with GL_RGBA and GL_UNSIGNED_BYTE.
    std::uint8_t bytes[4];
    std::memcpy(bytes, &c, sizeof bytes);
    PX_CHECK(bytes[0] == 0x11 && bytes[1] == 0x22 && bytes[2] == 0x33 && bytes[3] == 0x44);

    // The hex number as it is written, 0xRRGGBBAA, whatever the order in memory.
    PX_CHECK(c.ToHex() == 0x11223344u);
    PX_CHECK(pxColorRgba::FromHex(0x11223344u) == c);
    PX_CHECK(pxColorRgba::FromHex(0xFF000080u) == pxColorRgba(255, 0, 0, 128));
}
