#pragma once

#include "pixie/render/Export.hpp"

#include "pixie/core/FramebufferSize.hpp"

#include <filesystem>

namespace pixie {

/// Reads origin-zero RGBA pixels from the current read framebuffer after drawing, before swapping.
/// Uses client-memory pack state, sets pack alignment to 1 and writes opaque alpha, top row first.
/// Logs and returns false for an empty framebuffer or a file-write failure.
PIXIE_RENDER_API bool SaveFramebufferPng(const std::filesystem::path& path, FramebufferSize size);

} // namespace pixie
