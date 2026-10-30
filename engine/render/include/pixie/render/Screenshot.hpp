#pragma once

#include "pixie/render/Export.hpp"

#include "pixie/core/FramebufferSize.hpp"

#include <filesystem>

namespace pixie {

// Reads the color buffer of the framebuffer bound for reading and writes it as an RGBA PNG, top row
// first. Call it after rendering and before the buffers swap. Returns false, after logging why, if
// the file cannot be written.
PIXIE_RENDER_API bool SaveFramebufferPng(const std::filesystem::path& path, FramebufferSize size);

} // namespace pixie
