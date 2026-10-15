#include "pixie/Image.hpp"

#include <algorithm>
#include <array>
#include <climits>
#include <fstream>
#include <iterator>
#include <string_view>

// PNG only: the one format sprites read. Decoding from memory keeps stb_image away from file names,
// which it would open with fopen.
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_FAILURE_USERMSG
#include <stb_image.h>

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

void PutChunk(std::vector<std::uint8_t>& out, std::string_view type, const std::vector<std::uint8_t>& data) {
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

std::optional<Image> DecodePng(std::span<const std::uint8_t> bytes, std::string& error) {
    if (bytes.empty()) {
        error = "the file is empty";
        return std::nullopt;
    }
    if (bytes.size() > static_cast<std::size_t>(INT_MAX)) {
        error = "the file is too large";
        return std::nullopt;
    }
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* decoded = stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &width, &height,
                                             &channels, 4);
    if (decoded == nullptr) {
        const char* reason = stbi_failure_reason();
        error = std::string("not a PNG that can be read (") + (reason != nullptr ? reason : "unknown") + ")";
        return std::nullopt;
    }
    Image image;
    image.width = width;
    image.height = height;
    const std::size_t size = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4;
    image.pixels.assign(decoded, decoded + size);
    stbi_image_free(decoded);

    for (std::size_t i = 0; i < size; i += 4) {
        const unsigned alpha = image.pixels[i + 3];
        for (std::size_t c = 0; c < 3; ++c) {
            image.pixels[i + c] = static_cast<std::uint8_t>((image.pixels[i + c] * alpha + 127) / 255);
        }
    }
    return image;
}

std::optional<Image> LoadPngFile(const std::filesystem::path& path, std::string& error) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::error_code ignored;
        error = std::filesystem::exists(path, ignored) ? "the file cannot be opened" : "the file does not exist";
        return std::nullopt;
    }
    const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    return DecodePng(bytes, error);
}

std::vector<std::uint8_t> EncodePng(int width, int height, std::span<const std::uint8_t> rgba) {
    const auto rowBytes = static_cast<std::size_t>(width) * 4;
    std::vector<std::uint8_t> raw;
    raw.reserve(static_cast<std::size_t>(height) * (rowBytes + 1));
    for (std::size_t row = 0; row < static_cast<std::size_t>(height); ++row) {
        // Each PNG row starts with its filter type (0, none).
        raw.push_back(0);
        const auto first = rgba.begin() + static_cast<std::ptrdiff_t>(row * rowBytes);
        raw.insert(raw.end(), first, first + static_cast<std::ptrdiff_t>(rowBytes));
    }

    std::vector<std::uint8_t> png{0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    std::vector<std::uint8_t> header;
    PutU32(header, static_cast<std::uint32_t>(width));
    PutU32(header, static_cast<std::uint32_t>(height));
    header.insert(header.end(), {8, 6, 0, 0, 0}); // 8 bits, RGBA, deflate, no filter, no interlace
    PutChunk(png, "IHDR", header);
    PutChunk(png, "IDAT", StoredZlib(raw));
    PutChunk(png, "IEND", {});
    return png;
}

bool WriteFileBytes(const std::filesystem::path& path, std::span<const std::uint8_t> bytes) {
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return static_cast<bool>(file);
}

Image CheckerboardImage(int width, int height, int cell) {
    Image image;
    image.width = width;
    image.height = height;
    image.pixels.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const bool magenta = ((x / cell) + (y / cell)) % 2 == 0;
            auto* p = image.pixels.data() + (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                                             static_cast<std::size_t>(x)) * 4;
            p[0] = magenta ? 255 : 40;
            p[1] = magenta ? 0 : 40;
            p[2] = magenta ? 255 : 40;
            p[3] = 255;
        }
    }
    return image;
}

} // namespace pixie
