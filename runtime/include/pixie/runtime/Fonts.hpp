#pragma once

#include "pixie/render/2d/Font.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace pixie {

// The fonts a session's text names, by path, as SpriteImages holds its images: each read on first
// use, with paths resolved against the folder, the project's (the working directory with none set).
// A font that cannot be read (no path, a missing file, not a TrueType font) draws nothing, and its
// problem is logged once. Needs no GL context; the session's TextRenderer holds the textures.
class Fonts {
public:
    // Resolves paths against `folder` from now on, and forgets every font.
    void SetFolder(std::filesystem::path folder);
    const std::filesystem::path& Folder() const { return folder_; }
    // The file a path names.
    std::filesystem::path Resolve(std::string_view path) const;

    // The font at `path`; nothing when it cannot be read.
    Font* Find(std::string_view path);
    // Why the font at `path` cannot be read; empty when it can.
    std::string Problem(std::string_view path);

private:
    struct Entry {
        std::optional<Font> font;
        std::string problem;
    };

    Entry& Read(std::string_view path);

    std::filesystem::path folder_;
    std::unordered_map<std::string, Entry> entries_;
};

} // namespace pixie
