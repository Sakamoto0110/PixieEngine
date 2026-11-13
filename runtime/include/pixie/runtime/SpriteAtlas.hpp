#pragma once

#include "pixie/runtime/Export.hpp"

#include <pixie/math/pxVec.hpp>
#include <pixie/pxRect.hpp>

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace pixie {

struct SpriteAtlasFrameRef {
    std::string atlas; // Project-relative .atlas.json path.
    std::string frameId; // Opaque 128-bit ID scoped to this atlas.

    bool IsSet() const { return !atlas.empty() || !frameId.empty(); }
    PIXIE_RUNTIME_API bool IsValid() const;
};

struct SpriteAtlasFrame {
    std::string name;
    pxRect region; // Pixels from the top-left of the atlas image.
    pxVec2 pivot; // Normalized within the region, from its top-left.
};

class SpriteAtlas {
public:
    // Validates a version 1 descriptor. Reports why it failed and clears problem on success.
    // Callers supply UTF-8 JSON; this performs no filesystem or graphics work.
    PIXIE_RUNTIME_API static std::optional<SpriteAtlas> Parse(std::string_view json, std::string& problem);
    // Reads and parses the supplied file. The caller resolves and validates the path before opening it.
    PIXIE_RUNTIME_API static std::optional<SpriteAtlas> Load(const std::filesystem::path& file, std::string& problem);
    PIXIE_RUNTIME_API static bool IsDescriptorPath(std::string_view path);

    const std::string& Image() const { return image_; }
    pxSizei ImageSize() const { return imageSize_; }

    // A missing ID has no substitute. The caller keeps its original frame reference for repair.
    PIXIE_RUNTIME_API const SpriteAtlasFrame* FindFrame(std::string_view id) const;

    const auto& Frames() const { return frames_; }

private:
    SpriteAtlas() = default;

    std::string image_;
    pxSizei imageSize_;
    std::map<std::string, SpriteAtlasFrame, std::less<>> frames_;
};

} // namespace pixie
