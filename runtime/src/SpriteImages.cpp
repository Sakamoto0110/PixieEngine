#include "pixie/runtime/SpriteImages.hpp"

#include "pixie/Log.hpp"

#include <format>
#include <system_error>
#include <utility>

namespace pixie {

SpriteImages::~SpriteImages() = default;

void SpriteImages::SetFolder(std::filesystem::path folder) {
    Destroy();
    entries_.clear();
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
    return changed;
}

void SpriteImages::Destroy() {
    for (auto& [path, entry] : entries_) {
        entry.texture.Destroy();
    }
    // Images whose texture was released are read again on their next use.
    std::erase_if(entries_, [](const auto& item) { return !item.second.image && item.second.size; });
    checkerboard_.Destroy();
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
    entry.texture.Destroy();
    entry.image.reset();
    entry.size.reset();
    entry.problem.clear();
    if (path.empty()) {
        entry.problem = "no image is set";
        return;
    }
    entry.file = Resolve(path);
    std::error_code error;
    entry.existed = std::filesystem::exists(entry.file, error);
    entry.time = entry.existed ? std::filesystem::last_write_time(entry.file, error) : std::filesystem::file_time_type{};

    std::string problem;
    if (std::optional<Image> image = LoadPngFile(entry.file, problem)) {
        entry.size = pxVec2(image->width, image->height);
        entry.image = std::move(image);
        return;
    }
    entry.problem = std::move(problem);
    LogWarning(std::format("Sprite image {}: {}", path, entry.problem));
}

} // namespace pixie
