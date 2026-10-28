#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace pixie {

// An RGBA8 image in memory, four bytes per pixel, first row at the top.
struct Image {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> pixels;
};

// Decodes a PNG. The pixels come out with premultiplied alpha (red, green, and blue already
// multiplied by alpha), as the SpriteRenderer draws them, so filtering leaves no dark fringes.
// Returns nothing, and sets `error`, when the bytes are not a PNG it can read.
std::optional<Image> DecodePng(std::span<const std::uint8_t> bytes, std::string& error);

// Reads and decodes the PNG file at `path`, as DecodePng does.
std::optional<Image> LoadPngFile(const std::filesystem::path& path, std::string& error);

// Encodes `width` x `height` RGBA pixels with straight alpha, first row at the top, as a PNG. The
// deflate blocks are stored, not compressed: the file is as large as the pixels.
std::vector<std::uint8_t> EncodePng(int width, int height, std::span<const std::uint8_t> rgba);

// Writes `bytes` to `path`; returns false when the file cannot be written.
bool WriteFileBytes(const std::filesystem::path& path, std::span<const std::uint8_t> bytes);

// What is drawn in place of an image that is missing or cannot be read: magenta and dark squares of
// `cell` pixels, opaque.
Image CheckerboardImage(int width, int height, int cell);

} // namespace pixie
