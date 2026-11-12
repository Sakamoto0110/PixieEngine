#pragma once

#include "pixie/core/Export.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace pixie {

/// Owns tightly packed, top-row-first RGBA8 pixels. Alpha convention depends on the producer.
struct Image {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> pixels;
};

// Decodes a PNG. The pixels come out with premultiplied alpha (red, green, and blue already
// multiplied by alpha), as the SpriteRenderer draws them, so filtering leaves no dark fringes.
// Returns nothing, and sets `error`, when the bytes are not a PNG it can read.
PIXIE_CORE_API std::optional<Image> DecodePng(std::span<const std::uint8_t> bytes, std::string& error);

// Reads and decodes the PNG file at `path`, as DecodePng does.
PIXIE_CORE_API std::optional<Image> LoadPngFile(const std::filesystem::path& path, std::string& error);

/// Encodes a complete, tightly packed RGBA8 span with straight alpha and the top row first.
/// Supply positive dimensions and at least width * height * 4 bytes. Uses stored deflate blocks.
PIXIE_CORE_API std::vector<std::uint8_t> EncodePng(int width, int height, std::span<const std::uint8_t> rgba);

// Writes `bytes` to `path`; returns false when the file cannot be written.
PIXIE_CORE_API bool WriteFileBytes(const std::filesystem::path& path, std::span<const std::uint8_t> bytes);

// What is drawn in place of an image that is missing or cannot be read: magenta and dark squares of
// `cell` pixels, opaque.
PIXIE_CORE_API Image CheckerboardImage(int width, int height, int cell);

} // namespace pixie
