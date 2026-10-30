#include "pixie/render/Screenshot.hpp"

#include "pixie/render/gl/Gl.hpp"
#include "pixie/core/Image.hpp"
#include "pixie/core/Log.hpp"

#include <cstdint>
#include <format>
#include <vector>

namespace pixie {

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

    // Opaque output: the framebuffer's alpha is not meant to be seen. GL's rows run bottom to top.
    const std::size_t rowBytes = width * 4;
    std::vector<std::uint8_t> topFirst(pixels.size());
    for (std::size_t row = 0; row < height; ++row) {
        const auto* source = pixels.data() + (height - 1 - row) * rowBytes;
        auto* target = topFirst.data() + row * rowBytes;
        for (std::size_t i = 0; i < rowBytes; ++i) {
            target[i] = i % 4 == 3 ? std::uint8_t{255} : source[i];
        }
    }

    if (!WriteFileBytes(path, EncodePng(size.width, size.height, topFirst))) {
        LogError(std::format("Screenshot: cannot write {}", path.string()));
        return false;
    }
    LogInfo(std::format("Screenshot: {} ({}x{})", path.string(), width, height));
    return true;
}

} // namespace pixie
