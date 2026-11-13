#include "pixie/runtime/SpriteAtlas.hpp"

#include "pixie/runtime/ProjectFile.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <fstream>
#include <limits>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

namespace pixie {
namespace {

using Json = nlohmann::json;

char LowerAscii(char ch) {
    return ch >= 'A' && ch <= 'Z' ? static_cast<char>(ch - 'A' + 'a') : ch;
}

bool IsDeviceName(std::string_view part) {
    auto stem = part.substr(0, part.find('.'));
    const auto last = stem.find_last_not_of(' ');
    stem = last == std::string_view::npos ? std::string_view{} : stem.substr(0, last + 1);
    for (const std::string_view name : {"con", "prn", "aux", "nul", "conin$", "conout$"}) {
        if (std::ranges::equal(stem, name, {}, LowerAscii)) return true;
    }
    if (stem.size() <= 3) return false;
    const auto prefix = stem.substr(0, 3);
    const auto suffix = stem.substr(3);
    return (std::ranges::equal(prefix, std::string_view{"com"}, {}, LowerAscii) ||
            std::ranges::equal(prefix, std::string_view{"lpt"}, {}, LowerAscii)) &&
           ((suffix.size() == 1 && suffix.front() >= '1' && suffix.front() <= '9') ||
            suffix == "\xC2\xB9" || suffix == "\xC2\xB2" || suffix == "\xC2\xB3");
}

bool ValidAssetPath(std::string_view path, std::string_view suffix) {
    if (path.size() <= suffix.size() || !IsProjectRelativePath(path) ||
        !std::ranges::equal(path.substr(path.size() - suffix.size()), suffix, {}, LowerAscii) ||
        path.find_first_of("<>\"|?*") != std::string_view::npos ||
        std::ranges::any_of(path, [](unsigned char ch) { return ch < 32 || ch == 127; })) {
        return false;
    }

    for (std::size_t start = 0; start < path.size();) {
        const auto end = std::min(path.find('/', start), path.size());
        const auto part = path.substr(start, end - start);
        if (part.back() == '.' || part.back() == ' ' || IsDeviceName(part) ||
            (end == path.size() && part.size() <= suffix.size())) {
            return false;
        }
        start = end + 1;
    }
    return true;
}

bool ValidFrameId(std::string_view id) {
    return id.size() == 32 && id.find_first_not_of('0') != std::string_view::npos &&
           id.find_first_not_of("0123456789abcdef") == std::string_view::npos;
}

bool PixelInteger(const Json& value, std::int32_t& result) {
    if (!value.is_number_integer()) {
        return false;
    }
    // Check the range before converting, including JSON integers stored as unsigned.
    const auto number = value.get<double>();
    if (number < 0 || number > (std::numeric_limits<std::int32_t>::max)()) {
        return false;
    }
    result = static_cast<std::int32_t>(number);
    return true;
}

bool NormalizedNumber(const Json& value, double& result) {
    if (!value.is_number()) {
        return false;
    }
    result = value.get<double>();
    return std::isfinite(result) && result >= 0.0 && result <= 1.0;
}

bool ReadRegion(const Json& rect, pxSizei imageSize, pxRect& region) {
    std::int32_t x, y, width, height;
    if (!rect.is_array() || rect.size() != 4 || !PixelInteger(rect[0], x) || !PixelInteger(rect[1], y) ||
        !PixelInteger(rect[2], width) || !PixelInteger(rect[3], height) || width == 0 || height == 0 ||
        width > imageSize.width - x || height > imageSize.height - y) {
        return false;
    }
    region = {static_cast<double>(x), static_cast<double>(y), static_cast<double>(width), static_cast<double>(height)};
    return true;
}

} // namespace

bool SpriteAtlasFrameRef::IsValid() const {
    return SpriteAtlas::IsDescriptorPath(atlas) && ValidFrameId(frameId);
}

bool SpriteAtlas::IsDescriptorPath(std::string_view path) {
    return ValidAssetPath(path, ".atlas.json");
}

std::optional<SpriteAtlas> SpriteAtlas::Load(const std::filesystem::path& file, std::string& problem) {
    std::ifstream input(file, std::ios::binary);
    std::ostringstream contents;
    if (!input || !(contents << input.rdbuf()) || input.bad()) {
        problem = "the descriptor cannot be read";
        return std::nullopt;
    }
    return Parse(contents.str(), problem);
}

std::optional<SpriteAtlas> SpriteAtlas::Parse(std::string_view json, std::string& problem) {
    auto fail = [&](std::string message) -> std::optional<SpriteAtlas> {
        problem = std::move(message);
        return std::nullopt;
    };

    // Duplicate members otherwise use the last value silently, so different readers could disagree.
    bool malformed = false;
    std::vector<std::set<std::string, std::less<>>> keys;
    const auto trackKeys = [&](int depth, Json::parse_event_t event, Json& value) {
        if (depth > 8) {
            malformed = true;
        }
        switch (event) {
        case Json::parse_event_t::object_start: keys.emplace_back(); break;
        case Json::parse_event_t::key:
            if (!keys.back().insert(value.get_ref<const std::string&>()).second) {
                malformed = true;
            }
            break;
        case Json::parse_event_t::object_end: keys.pop_back(); break;
        default: break;
        }
        return true;
    };
    const auto document = Json::parse(json, trackKeys, false);
    if (malformed || !document.is_object()) {
        return fail("expected a sprite atlas JSON object with unique members and nesting at most 8 levels deep");
    }
    const auto format = document.find("format");
    const auto version = document.find("version");
    if (format == document.end() || *format != "pixie-sprite-atlas") {
        return fail("expected format pixie-sprite-atlas");
    }
    if (version == document.end() || !version->is_number_integer() || *version != 1) {
        return fail("unsupported sprite atlas version; expected 1");
    }

    SpriteAtlas atlas;
    const auto image = document.find("image");
    if (image == document.end() || !image->is_string() ||
        !ValidAssetPath(image->get_ref<const std::string&>(), ".png")) {
        return fail("image must be a project-relative PNG path");
    }
    atlas.image_ = image->get<std::string>();
    const auto size = document.find("imageSize");
    if (size == document.end() || !size->is_array() || size->size() != 2 ||
        !PixelInteger((*size)[0], atlas.imageSize_.width) || !PixelInteger((*size)[1], atlas.imageSize_.height) ||
        atlas.imageSize_.width == 0 || atlas.imageSize_.height == 0) {
        return fail("imageSize must contain positive integer width and height");
    }
    const auto frames = document.find("frames");
    if (frames == document.end() || !frames->is_array()) {
        return fail("frames must be an array");
    }
    std::size_t index = 0;
    for (const auto& frame : *frames) {
        const auto frameProblem = [&](std::string_view detail) {
            return fail(std::format("frames[{}]: {}", index, detail));
        };
        const auto id = frame.find("id");
        if (id == frame.end() || !id->is_string() || !ValidFrameId(id->get_ref<const std::string&>())) {
            return frameProblem("id must be 32 lowercase hexadecimal digits and nonzero");
        }
        const auto name = frame.find("name");
        if (name == frame.end() || !name->is_string() || name->get_ref<const std::string&>().empty()) {
            return frameProblem("name must be a nonempty string");
        }
        SpriteAtlasFrame entry;
        const auto rect = frame.find("rect");
        if (rect == frame.end() || !ReadRegion(*rect, atlas.imageSize_, entry.region)) {
            return frameProblem("rect must be a nonempty integer pixel rectangle inside the image");
        }
        const auto pivot = frame.find("pivot");
        if (pivot == frame.end() || !pivot->is_array() || pivot->size() != 2 ||
            !NormalizedNumber((*pivot)[0], entry.pivot.x) || !NormalizedNumber((*pivot)[1], entry.pivot.y)) {
            return frameProblem("pivot must contain two numbers between 0 and 1");
        }
        entry.name = name->get<std::string>();
        if (!atlas.frames_.try_emplace(id->get_ref<const std::string&>(), std::move(entry)).second) {
            return frameProblem("duplicate frame id");
        }
        ++index;
    }
    problem.clear();
    return atlas;
}

const SpriteAtlasFrame* SpriteAtlas::FindFrame(std::string_view id) const {
    const auto found = frames_.find(id);
    return found == frames_.end() ? nullptr : &found->second;
}

} // namespace pixie
