#pragma once

#include "pixie/runtime/Export.hpp"

#include "pixie/core/Image.hpp"
#include "pixie/render/gl/Texture.hpp"
#include "pixie/runtime/SpriteAtlas.hpp"

#include <pixie/math/pxVec.hpp>

#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace pixie {

struct Sprite;
class SpriteAlphaOutlines;

struct SpriteSource {
    std::string_view image;
    pxRect region;
    pxVec2 pivot;
};

// The images a session's sprites draw, by the path a Sprite names. Each is read on first use and
// read again when its file changes, so an image saved by a painting program shows without reopening
// anything. Paths are resolved against the folder, the project's; with none set, against the
// working directory.
//
// An image that cannot be read (no path, a missing file, not a PNG) is drawn as a checkerboard, and
// its problem is logged once each time it is read. Lookups that only read files need no GL context.
// Uploads and mutations that can delete textures require the owning context current, after all
// draws borrowing the texture names are flushed. Call Destroy or ReleaseAll before context teardown.
class SpriteImages {
public:
    SpriteImages() = default;
    // The session's derived outlines are invalidated when images or descriptors change.
    // The outline cache must outlive this cache.
    explicit SpriteImages(SpriteAlphaOutlines& outlines) : outlines_(&outlines) {}
    SpriteImages(const SpriteImages&) = delete;
    SpriteImages& operator=(const SpriteImages&) = delete;
    PIXIE_RUNTIME_API ~SpriteImages();

    // Resolves paths against `folder` from now on, and forgets every image.
    PIXIE_RUNTIME_API void SetFolder(std::filesystem::path folder);
    const std::filesystem::path& Folder() const { return folder_; }
    // The file a Sprite's path names.
    PIXIE_RUNTIME_API std::filesystem::path Resolve(std::string_view path) const;

    // The size of the image at `path`, in pixels; nothing when it cannot be read.
    PIXIE_RUNTIME_API std::optional<pxVec2> Size(std::string_view path);
    // Why the image at `path` cannot be read; empty when it can.
    PIXIE_RUNTIME_API std::string Problem(std::string_view path);
    // Borrows the texture for `path`, or the shared checkerboard when it cannot be read.
    PIXIE_RUNTIME_API const Texture& TextureOf(std::string_view path);
    // Resolves the atlas source, or returns the direct source. An unavailable atlas frame
    // gives the centered checkerboard. Consume the view before the sprite or cache changes.
    PIXIE_RUNTIME_API SpriteSource Source(const Sprite& sprite);

    // Reads the PNG again regardless of its timestamp. Returns whether it decoded. A failed
    // read replaces the entry with its diagnostic. The next draw uploads the new texture.
    PIXIE_RUNTIME_API bool Reload(std::string_view path);
    // Reads the descriptor and its current PNG again. Returns whether both files and the
    // declared dimensions are valid. Frame IDs are checked when a sprite requests one.
    PIXIE_RUNTIME_API bool ReloadAtlas(std::string_view path);
    // Evicts one cached entry (including a cached failure). Next use reads it from disk.
    PIXIE_RUNTIME_API bool Release(std::string_view path);
    // Evicts a descriptor while retaining the PNG, which other sprites can share.
    PIXIE_RUNTIME_API bool ReleaseAtlas(std::string_view path);
    // Drops all cached data and textures, including the checkerboard, while retaining the folder.
    PIXIE_RUNTIME_API void ReleaseAll();

    // Reads again each image and descriptor whose file changed, appeared or disappeared;
    // returns the number of changed files. A session calls it at most every kCheckInterval seconds.
    PIXIE_RUNTIME_API int CheckForChanges();
    static constexpr double kCheckInterval = 0.5;

    // Releases the textures; the images are read again when next used.
    PIXIE_RUNTIME_API void Destroy();

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

    struct AtlasEntry {
        std::filesystem::path file;
        std::optional<std::filesystem::file_time_type> time;
        std::optional<SpriteAtlas> atlas;
        std::set<std::string, std::less<>> missingFrames;
        std::string imageProblem;
    };

    PIXIE_RUNTIME_API Entry& Find(std::string_view path);
    PIXIE_RUNTIME_API void Read(std::string_view path, Entry& entry);
    AtlasEntry& FindAtlas(std::string_view path);
    void ReadAtlas(std::string_view path, AtlasEntry& entry);
    bool CheckAtlasImage(std::string_view path, AtlasEntry& entry);
    void InvalidateOutlines();

    SpriteAlphaOutlines* outlines_ = nullptr;
    std::filesystem::path folder_;
    std::unordered_map<std::string, Entry> entries_;
    std::unordered_map<std::string, AtlasEntry> atlases_;
    std::set<std::pair<std::string, std::string>> invalidReferences_;
    Texture checkerboard_;
};

} // namespace pixie
