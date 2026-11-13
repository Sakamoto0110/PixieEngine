#include "pixie/runtime/SpriteImages.hpp"

#include "pixie/core/Log.hpp"
#include "pixie/runtime/LightingDrawing.hpp"
#include "pixie/runtime/Sprite.hpp"

#include <format>
#include <system_error>
#include <utility>

namespace pixie {
namespace {

std::optional<std::filesystem::file_time_type> LastWrite(const std::filesystem::path& file) {
    std::error_code error;
    const auto time = std::filesystem::last_write_time(file, error);
    return error ? std::nullopt : std::optional{time};
}

} // namespace

SpriteImages::~SpriteImages() = default;

void SpriteImages::SetFolder(std::filesystem::path folder) {
    ReleaseAll();
    folder_ = std::move(folder);
}

std::filesystem::path SpriteImages::Resolve(std::string_view path) const {
    const auto relative = std::filesystem::path(std::u8string(path.begin(), path.end()));
    return folder_.empty() ? relative : folder_ / relative;
}

std::optional<pxVec2> SpriteImages::Size(std::string_view path) {
    return Find(path).size;
}

std::string SpriteImages::Problem(std::string_view path) {
    return Find(path).problem;
}

const Texture& SpriteImages::TextureOf(std::string_view path) {
    Entry& entry = Find(path);
    if (entry.image) {
        entry.texture.Create(*entry.image);
        entry.image.reset();
    }
    if (entry.texture.IsValid()) {
        return entry.texture;
    }
    if (!checkerboard_.IsValid()) {
        checkerboard_.Create(CheckerboardImage(16, 16, 4));
    }
    return checkerboard_;
}

SpriteSource SpriteImages::Source(const Sprite& sprite) {
    const auto& ref = sprite.atlasFrame;
    if (!ref.IsSet()) {
        return {sprite.image, sprite.region, sprite.pivot};
    }
    const SpriteSource missing{{}, {0.0, 0.0, 0.0, 0.0}, {0.5, 0.5}};
    if (!ref.IsValid()) {
        if (invalidReferences_.emplace(ref.atlas, ref.frameId).second) {
            LogWarning(std::format("Sprite atlas reference {} ({}): expected a project-relative .atlas.json path and a valid frame ID",
                                   ref.atlas, ref.frameId));
        }
        return missing;
    }

    AtlasEntry& entry = FindAtlas(ref.atlas);
    if (!entry.atlas) {
        return missing;
    }
    const auto* frame = entry.atlas->FindFrame(ref.frameId);
    if (frame == nullptr) {
        if (entry.missingFrames.insert(ref.frameId).second) {
            LogWarning(std::format("Sprite atlas {}: frame {} is missing", ref.atlas, ref.frameId));
        }
        return missing;
    }

    if (!CheckAtlasImage(ref.atlas, entry)) {
        return missing;
    }
    return {entry.atlas->Image(), frame->region, frame->pivot};
}

bool SpriteImages::CheckAtlasImage(std::string_view path, AtlasEntry& entry) {
    const auto& image = Find(entry.atlas->Image());
    const auto size = entry.atlas->ImageSize();
    if (!image.size || *image.size != pxVec2(size.width, size.height)) {
        const auto problem = image.size
            ? std::format("decoded PNG is {}x{}, expected {}x{}", image.size->x, image.size->y, size.width, size.height)
            : image.problem;
        if (entry.imageProblem != problem) {
            LogWarning(std::format("Sprite atlas {} ({}): {}", path, entry.atlas->Image(), problem));
            entry.imageProblem = problem;
        }
        return false;
    }
    entry.imageProblem.clear();
    return true;
}

bool SpriteImages::Reload(std::string_view path) {
    auto it = entries_.try_emplace(std::string(path)).first;
    InvalidateOutlines();
    Read(it->first, it->second);
    return it->second.size.has_value();
}

bool SpriteImages::ReloadAtlas(std::string_view path) {
    if (!SpriteAtlas::IsDescriptorPath(path)) {
        LogWarning(std::format("Sprite atlas {}: expected a project-relative .atlas.json path", path));
        return false;
    }
    auto [it, inserted] = atlases_.try_emplace(std::string(path));
    auto& entry = it->second;
    if (inserted) entry.file = Resolve(it->first);
    ReadAtlas(it->first, entry);
    if (!entry.atlas) {
        InvalidateOutlines();
        return false;
    }
    Reload(entry.atlas->Image());
    return CheckAtlasImage(it->first, entry);
}

bool SpriteImages::Release(std::string_view path) {
    const auto it = entries_.find(std::string(path));
    if (it == entries_.end()) return false;
    InvalidateOutlines();
    it->second.texture.Destroy();
    entries_.erase(it);
    return true;
}

bool SpriteImages::ReleaseAtlas(std::string_view path) {
    const auto released = atlases_.erase(std::string(path));
    if (released) InvalidateOutlines();
    return released != 0;
}

void SpriteImages::ReleaseAll() {
    Destroy();
    entries_.clear();
}

void SpriteImages::InvalidateOutlines() {
    if (outlines_) outlines_->Clear();
}

int SpriteImages::CheckForChanges() {
    int changed = 0;
    for (auto& [path, entry] : entries_) {
        if (path.empty()) {
            continue;
        }
        std::error_code error;
        const bool exists = std::filesystem::exists(entry.file, error);
        const auto time = exists ? std::filesystem::last_write_time(entry.file, error) : std::filesystem::file_time_type{};
        if (exists != entry.existed || time != entry.time) {
            Read(path, entry);
            ++changed;
        }
    }
    for (auto& [path, entry] : atlases_) {
        if (LastWrite(entry.file) != entry.time) {
            ReadAtlas(path, entry);
            ++changed;
        }
    }
    if (changed != 0) InvalidateOutlines();
    return changed;
}

void SpriteImages::Destroy() {
    InvalidateOutlines();
    for (auto& [path, entry] : entries_) {
        entry.texture.Destroy();
    }
    // Images whose texture was released are read again on their next use.
    std::erase_if(entries_, [](const auto& item) { return !item.second.image && item.second.size; });
    atlases_.clear();
    invalidReferences_.clear();
    checkerboard_.Destroy();
}

SpriteImages::AtlasEntry& SpriteImages::FindAtlas(std::string_view path) {
    auto [it, inserted] = atlases_.try_emplace(std::string(path));
    if (inserted) {
        it->second.file = Resolve(it->first);
        ReadAtlas(it->first, it->second);
    }
    return it->second;
}

SpriteImages::Entry& SpriteImages::Find(std::string_view path) {
    const auto found = entries_.find(std::string(path));
    if (found != entries_.end()) {
        return found->second;
    }
    Entry& entry = entries_[std::string(path)];
    Read(path, entry);
    return entry;
}

void SpriteImages::Read(std::string_view path, Entry& entry) {
    Entry next;
    if (path.empty()) {
        next.problem = "no image is set";
    } else {
        next.file = Resolve(path);
        std::error_code error;
        next.existed = std::filesystem::exists(next.file, error);
        if (next.existed) next.time = std::filesystem::last_write_time(next.file, error);
        next.image = LoadPngFile(next.file, next.problem);
        if (next.image) {
            next.size = pxVec2(next.image->width, next.image->height);
        } else {
            LogWarning(std::format("Sprite image {}: {}", path, next.problem));
        }
    }
    entry.texture.Destroy();
    entry = std::move(next);
}

void SpriteImages::ReadAtlas(std::string_view path, AtlasEntry& entry) {
    entry.time = LastWrite(entry.file);
    entry.missingFrames.clear();
    entry.imageProblem.clear();
    std::string problem;
    entry.atlas = SpriteAtlas::Load(entry.file, problem);
    if (!entry.atlas) {
        LogWarning(std::format("Sprite atlas {}: {}", path, problem));
    }
}

} // namespace pixie
