#pragma once

#include "pixie/core/Image.hpp"
#include "pixie/render/gl/Texture.hpp"

#include <pixie/math/pxVec.hpp>

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace pixie {

// The images a session's sprites draw, by the path a Sprite names. Each is read on first use and
// read again when its file changes, so an image saved by a painting program shows without reopening
// anything. Paths are resolved against the folder, the project's; with none set, against the
// working directory.
//
// An image that cannot be read (no path, a missing file, not a PNG) is drawn as a checkerboard, and
// its problem is logged once each time it is read. Reading needs no GL context; TextureOf does, and
// so do CheckForChanges, SetFolder, and Destroy once a texture exists.
class SpriteImages {
public:
    SpriteImages() = default;
    SpriteImages(const SpriteImages&) = delete;
    SpriteImages& operator=(const SpriteImages&) = delete;
    ~SpriteImages();

    // Resolves paths against `folder` from now on, and forgets every image.
    void SetFolder(std::filesystem::path folder);
    const std::filesystem::path& Folder() const { return folder_; }
    // The file a Sprite's path names.
    std::filesystem::path Resolve(std::string_view path) const;

    // The size of the image at `path`, in pixels; nothing when it cannot be read.
    std::optional<pxVec2> Size(std::string_view path);
    // Why the image at `path` cannot be read; empty when it can.
    std::string Problem(std::string_view path);
    // The texture drawn for `path`: the image's, or the checkerboard when it cannot be read.
    const Texture& TextureOf(std::string_view path);

    // Reads again every image whose file changed, appeared, or disappeared since it was read;
    // returns how many did. A session calls it at most every kCheckInterval seconds.
    int CheckForChanges();
    static constexpr double kCheckInterval = 0.5;

    // Releases the textures; the images are read again when next used.
    void Destroy();

    // The side, in pixels, of what is drawn for an image that cannot be read, when the sprite's
    // region does not give a size.
    static constexpr double kMissingSize = 64.0;

private:
    struct Entry {
        std::filesystem::path file;
        // Read and not yet uploaded.
        std::optional<Image> image;
        std::optional<pxVec2> size;
        std::string problem;
        bool existed = false;
        std::filesystem::file_time_type time{};
        Texture texture;
    };

    Entry& Find(std::string_view path);
    void Read(std::string_view path, Entry& entry);

    std::filesystem::path folder_;
    std::unordered_map<std::string, Entry> entries_;
    Texture checkerboard_;
};

} // namespace pixie
