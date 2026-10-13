#include "pixie/Screenshot.hpp"

#include "pixie/Gl.hpp"
#include "pixie/Log.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <fstream>
#include <string_view>
#include <vector>

namespace pixie {

namespace {

// A PNG needs a zlib stream; stored (uncompressed) deflate blocks keep the writer short, at the
// cost of a file as large as the pixels.

std::uint32_t Crc32(std::uint32_t crc, const std::uint8_t* data, std::size_t size) {
    static const auto table = [] {
        std::array<std::uint32_t, 256> t{};
        for (std::uint32_t n = 0; n < 256; ++n) {
            std::uint32_t c = n;
            for (int k = 0; k < 8; ++k) {
                c = (c & 1) != 0 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            }
            t[n] = c;
        }
        return t;
    }();
    crc = ~crc;
    for (std::size_t i = 0; i < size; ++i) {
        crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return ~crc;
}

void PutU32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    out.push_back(static_cast<std::uint8_t>(value >> 24));
    out.push_back(static_cast<std::uint8_t>(value >> 16));
    out.push_back(static_cast<std::uint8_t>(value >> 8));
    out.push_back(static_cast<std::uint8_t>(value));
}

void PutChunk(std::vector<std::uint8_t>& out, std::string_view type,
              const std::vector<std::uint8_t>& data) {
    PutU32(out, static_cast<std::uint32_t>(data.size()));
    const std::size_t start = out.size();
    out.insert(out.end(), type.begin(), type.end());
    out.insert(out.end(), data.begin(), data.end());
    PutU32(out, Crc32(0, out.data() + start, out.size() - start));
}

std::vector<std::uint8_t> StoredZlib(const std::vector<std::uint8_t>& raw) {
    std::vector<std::uint8_t> out{0x78, 0x01};
    constexpr std::size_t kBlock = 65535;
    std::size_t offset = 0;
    do {
        const std::size_t size = std::min(kBlock, raw.size() - offset);
        const bool last = offset + size == raw.size();
        out.push_back(last ? 1 : 0);
        out.push_back(static_cast<std::uint8_t>(size));
        out.push_back(static_cast<std::uint8_t>(size >> 8));
        out.push_back(static_cast<std::uint8_t>(~size));
        out.push_back(static_cast<std::uint8_t>(~size >> 8));
        out.insert(out.end(), raw.begin() + static_cast<std::ptrdiff_t>(offset),
                   raw.begin() + static_cast<std::ptrdiff_t>(offset + size));
        offset += size;
    } while (offset < raw.size());

    std::uint32_t a = 1;
    std::uint32_t b = 0;
    for (const std::uint8_t byte : raw) {
        a = (a + byte) % 65521;
        b = (b + a) % 65521;
    }
    PutU32(out, (b << 16) | a);
    return out;
}

} // namespace

bool SaveFramebufferPng(const std::filesystem::path& path, FramebufferSize size) {
    if (size.width <= 0 || size.height <= 0) {
        LogError("Screenshot: the framebuffer is empty");
        return false;
    }
    const auto width = static_cast<std::size_t>(size.width);
    const auto height = static_cast<std::size_t>(size.height);
    std::vector<std::uint8_t> pixels(width * height * 4);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, size.width, size.height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    // Opaque output: the framebuffer's alpha is not meant to be seen.
    for (std::size_t alpha = 3; alpha < pixels.size(); alpha += 4) {
        pixels[alpha] = 255;
    }
    // Each PNG row starts with its filter type (0, none); GL's rows run bottom to top.
    const std::size_t rowBytes = width * 4;
    std::vector<std::uint8_t> raw;
    raw.reserve(height * (rowBytes + 1));
    for (std::size_t row = 0; row < height; ++row) {
        const auto* source = pixels.data() + (height - 1 - row) * rowBytes;
        raw.push_back(0);
        raw.insert(raw.end(), source, source + rowBytes);
    }

    std::vector<std::uint8_t> png{0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    std::vector<std::uint8_t> header;
    PutU32(header, static_cast<std::uint32_t>(width));
    PutU32(header, static_cast<std::uint32_t>(height));
    header.insert(header.end(), {8, 6, 0, 0, 0}); // 8 bits, RGBA, deflate, no filter, no interlace
    PutChunk(png, "IHDR", header);
    PutChunk(png, "IDAT", StoredZlib(raw));
    PutChunk(png, "IEND", {});

    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size()));
    if (!file) {
        LogError(std::format("Screenshot: cannot write {}", path.string()));
        return false;
    }
    LogInfo(std::format("Screenshot: {} ({}x{})", path.string(), width, height));
    return true;
}

} // namespace pixie
