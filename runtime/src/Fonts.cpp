#include "pixie/runtime/Fonts.hpp"

#include "pixie/core/Log.hpp"
#include "pixie/render/2d/TextRenderer.hpp"

#include <format>
#include <utility>

namespace pixie {

Fonts::Fonts(Fonts&& other) noexcept
    : renderer_(std::exchange(other.renderer_, nullptr)),
      folder_(std::move(other.folder_)), entries_(std::move(other.entries_)) {}

Fonts& Fonts::operator=(Fonts&& other) noexcept {
    if (this != &other) {
        ReleaseAll();
        renderer_ = std::exchange(other.renderer_, nullptr);
        folder_ = std::move(other.folder_);
        entries_ = std::move(other.entries_);
    }
    return *this;
}

void Fonts::SetFolder(std::filesystem::path folder) {
    ReleaseAll();
    folder_ = std::move(folder);
}

std::filesystem::path Fonts::Resolve(std::string_view path) const {
    const auto relative = std::filesystem::path(std::u8string(path.begin(), path.end()));
    return folder_.empty() ? relative : folder_ / relative;
}

Font* Fonts::Find(std::string_view path) {
    Entry& entry = Read(path);
    return entry.font ? &*entry.font : nullptr;
}

std::string Fonts::Problem(std::string_view path) {
    return Read(path).problem;
}

bool Fonts::Reload(std::string_view path) {
    std::string key(path);
    Entry next = Load(key);
    const bool loaded = next.font.has_value();
    auto& entry = entries_[std::move(key)];
    ForgetTextures(entry);
    entry = std::move(next);
    return loaded;
}

bool Fonts::Release(std::string_view path) {
    const auto it = entries_.find(std::string(path));
    if (it == entries_.end()) return false;
    ForgetTextures(it->second);
    entries_.erase(it);
    return true;
}

void Fonts::ReleaseAll() {
    for (const auto& [path, entry] : entries_) ForgetTextures(entry);
    entries_.clear();
}

Fonts::Entry& Fonts::Read(std::string_view path) {
    if (const auto found = entries_.find(std::string(path)); found != entries_.end()) {
        return found->second;
    }
    return entries_.emplace(std::string(path), Load(path)).first->second;
}

Fonts::Entry Fonts::Load(std::string_view path) const {
    Entry entry;
    if (path.empty()) {
        entry.problem = "no font is named";
    } else {
        entry.font = Font::Load(Resolve(path), entry.problem);
    }
    if (!entry.font) {
        LogWarning(std::format("font \"{}\": {}", path, entry.problem));
    }
    return entry;
}

void Fonts::ForgetTextures(const Entry& entry) {
    if (renderer_ && entry.font) renderer_->Forget(*entry.font);
}

} // namespace pixie
