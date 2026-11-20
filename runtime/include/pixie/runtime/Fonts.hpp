#pragma once

#include "pixie/runtime/Export.hpp"

#include "pixie/render/2d/Font.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace pixie {

class TextRenderer;

// The fonts a session's text names, by path, as SpriteImages holds its images: each read on first
// use, with paths resolved against the folder, the project's (the working directory with none set).
// A font that cannot be read (no path, a missing file, not a TrueType font) draws nothing, and its
// problem is logged once per read. Owns the font data and rasterized glyphs in CPU memory.
// When attached to a TextRenderer, mutations that discard fonts can also delete GPU textures.
// Run those mutations with the owning context current after all borrowing draws are flushed.
class Fonts {
public:
    Fonts() = default;
    // The renderer must outlive this cache. ReleaseAll before tearing down its context.
    explicit Fonts(TextRenderer& renderer) : renderer_(&renderer) {}
    Fonts(const Fonts&) = delete;
    Fonts& operator=(const Fonts&) = delete;
    PIXIE_RUNTIME_API Fonts(Fonts&& other) noexcept;
    PIXIE_RUNTIME_API Fonts& operator=(Fonts&& other) noexcept;

    // Resolves paths against `folder` from now on, and forgets every font.
    PIXIE_RUNTIME_API void SetFolder(std::filesystem::path folder);
    const std::filesystem::path& Folder() const { return folder_; }
    // The file a path names.
    PIXIE_RUNTIME_API std::filesystem::path Resolve(std::string_view path) const;

    // Borrows the font at `path` until it is reloaded or released. Null when it cannot be read.
    PIXIE_RUNTIME_API Font* Find(std::string_view path);
    // Why the font at `path` cannot be read; empty when it can.
    PIXIE_RUNTIME_API std::string Problem(std::string_view path);

    // Reads the TTF again regardless of its timestamp. A failed read installs its diagnostic
    // and returns false. The next draw builds and uploads the new glyph atlas on demand.
    PIXIE_RUNTIME_API bool Reload(std::string_view path);
    // Evicts one cached font or failure. The next use reads it from disk.
    PIXIE_RUNTIME_API bool Release(std::string_view path);
    // Evicts every entry while retaining the folder.
    PIXIE_RUNTIME_API void ReleaseAll();

private:
    struct Entry {
        std::optional<Font> font;
        std::string problem;
    };

    PIXIE_RUNTIME_API Entry& Read(std::string_view path);
    Entry Load(std::string_view path) const;
    void ForgetTextures(const Entry& entry);

    TextRenderer* renderer_ = nullptr;
    std::filesystem::path folder_;
    std::unordered_map<std::string, Entry> entries_;
};

} // namespace pixie
