#include "pixie/runtime/ProjectFile.hpp"

#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <format>
#include <fstream>
#include <sstream>
#include <system_error>
#include <vector>

namespace pixie {
namespace {

using Json = nlohmann::ordered_json;

std::string PathText(const std::filesystem::path& path) {
    const std::u8string text = path.u8string();
    return std::string(reinterpret_cast<const char*>(text.data()), text.size());
}

std::filesystem::path FromText(std::string_view text) {
    return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(text.data()), text.size()));
}

OpenedProject Failed(std::string error) {
    OpenedProject opened;
    opened.ok = false;
    opened.error = std::move(error);
    return opened;
}

// The folder's own name, also for a path that ends with a separator.
std::string FolderName(const std::filesystem::path& folder) {
    std::error_code error;
    std::filesystem::path normal = std::filesystem::absolute(folder, error).lexically_normal();
    if (!normal.has_filename()) {
        normal = normal.parent_path();
    }
    return PathText(normal.filename());
}

} // namespace

std::filesystem::path OpenedProject::StartScenePath() const {
    return folder / FromText(file.startScene);
}

bool IsProjectRelativePath(std::string_view path) {
    if (path.empty() || path.front() == '/' || path.find('\\') != std::string_view::npos ||
        path.find(':') != std::string_view::npos) {
        return false;
    }
    std::size_t start = 0;
    while (start <= path.size()) {
        const std::size_t end = std::min(path.find('/', start), path.size());
        const std::string_view part = path.substr(start, end - start);
        if (part.empty() || part == "." || part == "..") {
            return false;
        }
        start = end + 1;
    }
    return true;
}

OpenedProject OpenProjectFolder(const std::filesystem::path& folder) {
    std::error_code error;
    if (!std::filesystem::is_directory(folder, error)) {
        return Failed(std::format("{} is not a folder", PathText(folder)));
    }
    std::vector<std::filesystem::path> found;
    for (const auto& entry : std::filesystem::directory_iterator(folder, error)) {
        if (entry.is_regular_file(error) && PathText(entry.path().extension()) == kProjectExtension) {
            found.push_back(entry.path());
        }
    }
    if (error) {
        return Failed(std::format("could not list {}: {}", PathText(folder), error.message()));
    }
    if (found.empty()) {
        return Failed(std::format("{} holds no {} file, so it is not a project", PathText(folder), kProjectExtension));
    }
    if (found.size() > 1) {
        return Failed(std::format("{} holds more than one {} file", PathText(folder), kProjectExtension));
    }

    std::ifstream in(found[0], std::ios::binary);
    if (!in) {
        return Failed(std::format("could not open {}", PathText(found[0])));
    }
    std::ostringstream text;
    text << in.rdbuf();
    Json document;
    try {
        document = Json::parse(text.str());
    } catch (const Json::parse_error& e) {
        return Failed(std::format("{} is not valid JSON: {}", PathText(found[0]), e.what()));
    }
    if (!document.is_object() || !document.contains("format") || document["format"] != std::string(kProjectFormat)) {
        return Failed(std::format("{} is not a project file: its format is not \"{}\"", PathText(found[0]),
                                  kProjectFormat));
    }
    if (!document.contains("version") || !document["version"].is_number_integer() ||
        document["version"].get<std::int64_t>() < 1) {
        return Failed(std::format("{} has no valid version", PathText(found[0])));
    }
    if (document["version"].get<std::int64_t>() > kProjectVersion) {
        return Failed(std::format("{} was written by a newer engine (version {}; this one reads up to {})",
                                  PathText(found[0]), document["version"].get<std::int64_t>(), kProjectVersion));
    }

    OpenedProject opened;
    opened.folder = folder;
    opened.path = found[0];
    opened.file.name = document.contains("name") && document["name"].is_string()
                           ? document["name"].get<std::string>()
                           : PathText(found[0].stem());
    if (document.contains("startScene")) {
        if (!document["startScene"].is_string() || !IsProjectRelativePath(document["startScene"].get<std::string>())) {
            return Failed(std::format("the start scene of {} must be a path inside the project's folder, with "
                                      "forward slashes",
                                      PathText(found[0])));
        }
        opened.file.startScene = document["startScene"].get<std::string>();
    }
    Json extras = Json::object();
    for (auto it = document.begin(); it != document.end(); ++it) {
        if (it.key() != "format" && it.key() != "version" && it.key() != "name" && it.key() != "startScene") {
            extras[it.key()] = it.value();
        }
    }
    if (!extras.empty()) {
        opened.file.extras = extras.dump();
    }
    return opened;
}

std::string WriteProjectFile(const std::filesystem::path& path, const ProjectFile& project) {
    if (!IsProjectRelativePath(project.startScene)) {
        return std::format("the start scene, \"{}\", is not a path inside the project's folder", project.startScene);
    }
    Json document = Json::object();
    document["format"] = std::string(kProjectFormat);
    document["version"] = kProjectVersion;
    document["name"] = project.name;
    document["startScene"] = project.startScene;
    if (!project.extras.empty()) {
        const Json extras = Json::parse(project.extras, nullptr, false);
        if (extras.is_object()) {
            for (auto it = extras.begin(); it != extras.end(); ++it) {
                if (!document.contains(it.key())) {
                    document[it.key()] = it.value();
                }
            }
        }
    }
    const std::string text = document.dump(2, ' ', false, Json::error_handler_t::replace) + "\n";
    if (const SceneReport saved = SaveTextFile(text, path); !saved.ok) {
        return saved.error;
    }
    return {};
}

OpenedProject CreateProjectFolder(const std::filesystem::path& folder) {
    const std::string name = FolderName(folder);
    if (name.empty()) {
        return Failed(std::format("{} has no name to give the project", PathText(folder)));
    }
    std::error_code error;
    if (std::filesystem::exists(folder, error)) {
        if (!std::filesystem::is_directory(folder, error)) {
            return Failed(std::format("{} is a file, not a folder", PathText(folder)));
        }
        if (!std::filesystem::is_empty(folder, error)) {
            return Failed(std::format("{} is not empty; a new project needs an empty or new folder", PathText(folder)));
        }
    }
    if (!std::filesystem::create_directories(folder / "scenes", error) && error) {
        return Failed(std::format("could not create {}: {}", PathText(folder), error.message()));
    }

    ProjectFile project;
    project.name = name;

    // The start scene: one shape at the origin, so the view shows something and there is something
    // to select.
    ComponentTypes types;
    DescribeRuntimeComponents(types);
    World world(1);
    Entity shape = world.Create("shape");
    shape.Add<Transform2D>();
    shape.Add<Shape>(Shape{ShapeKind::Circle, pxVec2(40.0, 40.0), pxColorRgba(110, 170, 255)});
    AddToScene(shape);
    if (const SceneReport saved = SaveSceneFile(world, types, folder / FromText(project.startScene)); !saved.ok) {
        return Failed(saved.error);
    }
    const std::filesystem::path path = folder / FromText(name + std::string(kProjectExtension));
    if (std::string written = WriteProjectFile(path, project); !written.empty()) {
        return Failed(std::move(written));
    }
    return OpenProjectFolder(folder);
}

} // namespace pixie
