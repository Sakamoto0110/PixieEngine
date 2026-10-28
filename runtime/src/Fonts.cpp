#include "pixie/runtime/Fonts.hpp"

#include "pixie/core/Log.hpp"

#include <format>
#include <utility>

namespace pixie {

void Fonts::SetFolder(std::filesystem::path folder) {
    entries_.clear();
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

Fonts::Entry& Fonts::Read(std::string_view path) {
    if (const auto found = entries_.find(std::string(path)); found != entries_.end()) {
        return found->second;
    }
    Entry& entry = entries_[std::string(path)];
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

} // namespace pixie
