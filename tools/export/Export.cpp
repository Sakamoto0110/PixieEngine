#include "Export.hpp"
#include "PeImage.hpp"
#include "Process.hpp"
#include "pixie/runtime/ProjectFile.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace pixie::packaging {
namespace {
namespace fs = std::filesystem;
using Json = nlohmann::json;
std::string Lower(std::string value) {
    for (auto& c : value) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return value;
}
Json Read(const fs::path& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot read " + Utf8(path));
    return Json::parse(input);
}
void Write(const fs::path& path, const Json& value) {
    std::ofstream output(path);
    output << value.dump(2) << '\n';
    if (!output) throw std::runtime_error("cannot write export manifest");
}
bool Reparse(const fs::path& path) {
#ifdef _WIN32
    const auto attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
#else
    return fs::is_symlink(fs::symlink_status(path));
#endif
}
bool Within(const fs::path& child, const fs::path& parent) {
    const auto relative = child.lexically_relative(parent);
    return !relative.empty() && !relative.is_absolute() && *relative.begin() != "..";
}
fs::path Relative(const std::string& text) {
    if (!pixie::IsProjectRelativePath(text) || text.find(':') != std::string::npos)
        throw std::runtime_error("export destination is not a portable relative path: " + text);
    const auto path = FromUtf8(text);
    for (const auto& part : path) {
        const auto name = Utf8(part);
        if (name.empty() || name.back() == '.' || name.back() == ' ' ||
            std::any_of(name.begin(), name.end(), [](unsigned char c) { return c < 32; }))
            throw std::runtime_error("invalid export path component");
    }
    return path;
}
void NoReparse(const fs::path& path) {
    for (auto part = fs::absolute(path); !part.empty(); part = part.parent_path()) {
        if (Reparse(part)) throw std::runtime_error("export paths cannot traverse reparse points: " + Utf8(part));
        if (part == part.parent_path()) break;
    }
}
void Binary(const fs::path& path) {
    const auto image = InspectPe(path);
    const auto forbidden = [](const std::string& symbol) {
        return symbol == "pixie_module_load" || symbol == "pixie_dev_dispatch" ||
            symbol == "pixie_game_create" || symbol == "pixie_game_api_version" ||
            symbol.find("LocalCommandChannel@pixie@@") != std::string::npos ||
            symbol.find("WorldDispatcher@dev@pixie@@") != std::string::npos ||
            symbol.find("PlayerChannel@dev@pixie@@") != std::string::npos;
    };
    if (image.developmentMarker || std::any_of(image.exports.begin(), image.exports.end(), forbidden) ||
        std::any_of(image.imports.begin(), image.imports.end(), forbidden))
        throw std::runtime_error("development code or game-module loader in " + Utf8(path));
}
bool IsBinary(const std::string& role) { return role == "player" || role == "library" || role == "runtime"; }
void CheckManifest(const Json& manifest) {
    if (manifest.at("format") != "pixie-export" || manifest.at("version") != 1 ||
        manifest.at("development") != false || manifest.at("dev_channel") != false)
        throw std::runtime_error("export requires a development-free shipping manifest");
    std::set<std::string> destinations;
    int players = 0;
    int projects = 0;
    int scenes = 0;
    std::string playerName;
    bool engine = false;
    for (const auto& file : manifest.at("files")) {
        const auto destination = file.at("destination").get<std::string>();
        const auto path = Relative(destination);
        const auto lower = Lower(destination);
        if (!destinations.insert(lower).second) throw std::runtime_error("duplicate export destination");
        const auto role = file.at("role").get<std::string>();
        const auto extension = Lower(Utf8(path.extension()));
        if (IsBinary(role)) {
            if (path.has_parent_path() || (role == "player" ? extension != ".exe" : extension != ".dll"))
                throw std::runtime_error("invalid binary destination");
            if (role == "player") { ++players; playerName = lower; }
            if (lower == "pixie.dll" && role == "library") engine = true;
            if (lower.ends_with("-module.dll") || lower.ends_with("-capi.dll") ||
                lower == "pixieeditor.exe" || lower == "pixie-export.exe" || lower == "pixie-mcp.exe")
                throw std::runtime_error("development artifact in export manifest");
        } else {
            if (role != "project" && role != "scene" && role != "asset") throw std::runtime_error("unknown manifest role");
            if (role == "project") {
                if (path.has_parent_path() || extension != ".pixie") throw std::runtime_error("invalid project destination");
                ++projects;
            }
            if (role == "scene") {
                if (extension != ".scene") throw std::runtime_error("invalid scene destination");
                ++scenes;
            }
            if (extension == ".dll" || extension == ".exe" || extension == ".pdb" || extension == ".lib" ||
                extension == ".obj" || extension == ".cs" || extension == ".cpp" || extension == ".hpp" ||
                extension == ".ps1" || extension == ".cmd" || extension == ".bat" || extension == ".py")
                throw std::runtime_error("development artifact declared as a game asset");
        }
        if (file.at("sha256").get<std::string>().size() != 64) throw std::runtime_error("invalid file digest");
    }
    if (players != 1 || projects != 1 || scenes == 0 || !engine || playerName != Lower(manifest.at("player").get<std::string>()))
        throw std::runtime_error("export requires one player, pixie.dll, one project and its scenes");
}

void Closure(const Json& manifest, const fs::path& output, bool sourcePaths) {
    std::set<std::string> files;
    for (const auto& item : manifest.at("files")) files.insert(Lower(item.at("destination").get<std::string>()));
    for (const auto& item : manifest.at("files")) {
        if (!IsBinary(item.at("role").get<std::string>())) continue;
        const auto binary = sourcePaths ? FromUtf8(item.at("source").get<std::string>())
            : output / Relative(item.at("destination").get<std::string>());
        for (const auto& name : InspectPe(binary).libraries)
            if (!files.contains(Lower(name)) && !IsWindowsLibrary(name))
                throw std::runtime_error("export has an unresolved native import: " + name);
    }
}

class Plan {
public:
    explicit Plan(fs::path project) : project_(fs::canonical(project)) {
        manifest = {{"format", "pixie-export"}, {"version", 1}, {"development", false},
            {"dev_channel", false}, {"project", Utf8(project_)}, {"files", Json::array()}};
    }
    void Add(const fs::path& source, const std::string& destination, const std::string& role) {
        Relative(destination);
        NoReparse(source);
        if (!fs::is_regular_file(source)) throw std::runtime_error("missing export input: " + Utf8(source));
        const auto canonical = fs::canonical(source);
        const auto key = Lower(destination);
        if (const auto found = sources_.find(key); found != sources_.end()) {
            if (found->second != canonical) throw std::runtime_error("two sources claim " + destination);
            return;
        }
        if (!IsBinary(role) && !Within(canonical, project_)) throw std::runtime_error("asset escapes the project");
        if (IsBinary(role)) Binary(canonical);
        sources_.emplace(key, canonical);
        manifest["files"].push_back({{"source", Utf8(canonical)}, {"destination", destination},
            {"role", role}, {"sha256", Digest(canonical)}});
    }
    void Asset(const std::string& path, const std::string& role = "asset") {
        Add(project_ / Relative(path), path, role);
    }
    void Binaries(const Json& product) {
        if (product.at("development") != false || product.at("dev_channel") != false || product.at("configuration") != "Release")
            throw std::runtime_error("product was not built as a shipping Release");
        const auto player = FromUtf8(product.at("player").get<std::string>());
        const auto destination = Utf8(player.filename());
        manifest["player"] = destination;
        Add(player, destination, "player");
        std::vector<fs::path> queue{player};
        for (const auto& file : product.at("libraries")) {
            const auto path = FromUtf8(file.get<std::string>());
            Add(path, Utf8(path.filename()), "library"); queue.push_back(path);
        }
        // Include the compiler's release CRT bundle, including dynamically used satellite DLLs.
        for (const auto& file : product.at("runtime")) {
            const auto path = FromUtf8(file.get<std::string>());
            Add(path, Utf8(path.filename()), "runtime"); queue.push_back(path);
        }
        for (std::size_t i = 0; i < queue.size(); ++i) {
            for (const auto& name : InspectPe(queue[i]).libraries) {
                if (sources_.contains(Lower(name)) || IsWindowsLibrary(name)) continue;
                const auto dependency = player.parent_path() / FromUtf8(name);
                Add(dependency, name, "library"); queue.push_back(dependency);
            }
        }
    }
    void Scenes(const pixie::OpenedProject& project) {
        pixie::ComponentTypes types;
        pixie::DescribeRuntimeComponents(types);
        std::set<fs::path> scenes{project.StartScenePath()};
        const auto directory = project_ / "scenes";
        if (fs::exists(directory)) {
            NoReparse(directory);
            for (const auto& entry : fs::recursive_directory_iterator(directory)) {
                if (Reparse(entry.path())) throw std::runtime_error("scene tree contains a reparse point");
                if (entry.is_regular_file() && entry.path().extension() == ".scene") scenes.insert(entry.path());
            }
        }
        for (const auto& scene : scenes) {
            NoReparse(scene);
            Asset(Utf8(scene.lexically_relative(project_)), "scene");
            const auto data = Read(scene);
            for (const auto& entity : data.at("entities")) {
                for (const auto& [name, component] : entity.at("components").items()) {
                    const auto type = types.IndexOf(name);
                    if (type < 0) continue; // Game-owned assets are declared explicitly by pixie_game(ASSETS ...).
                    for (const auto& field : types.All()[static_cast<std::size_t>(type)].fields) {
                        if (!field.metadata.asset.empty() && component.contains(field.name)) {
                            const auto path = component.at(field.name).get<std::string>();
                            if (!path.empty()) Asset(path);
                        }
                    }
                }
            }
        }
    }
    Json manifest;
private:
    fs::path project_;
    std::map<std::string, fs::path> sources_;
};
}

void VerifyExport(const fs::path& manifestPath, const fs::path& output) {
    const auto manifest = Read(manifestPath);
    CheckManifest(manifest);
    NoReparse(output);
    std::map<std::string, std::string> files;
    std::set<std::string> directories;
    for (const auto& item : manifest.at("files")) {
        const auto name = item.at("destination").get<std::string>();
        const auto path = output / Relative(name);
        NoReparse(path);
        if (!fs::is_regular_file(path)) throw std::runtime_error("missing exported file: " + name);
        if (Digest(path) != item.at("sha256").get<std::string>()) throw std::runtime_error("exported file differs from manifest: " + name);
        files.emplace(Lower(name), item.at("role").get<std::string>());
        for (auto parent = Relative(name).parent_path(); !parent.empty(); parent = parent.parent_path())
            directories.insert(Lower(Utf8(parent)));
        if (IsBinary(files.at(Lower(name)))) Binary(path);
    }
    for (const auto& entry : fs::recursive_directory_iterator(output)) {
        if (Reparse(entry.path())) throw std::runtime_error("export contains a reparse point");
        if (entry.is_regular_file() && !files.contains(Lower(Utf8(entry.path().lexically_relative(output)))))
            throw std::runtime_error("unlisted file in export: " + Utf8(entry.path()));
        if (entry.is_directory() && !directories.contains(Lower(Utf8(entry.path().lexically_relative(output)))))
            throw std::runtime_error("unlisted directory in export: " + Utf8(entry.path()));
    }
    Closure(manifest, output, false);
    std::cout << "Verified shipping manifest, binary exclusions and dependency closure: " << Utf8(output) << '\n';
}

void CopyManifest(const fs::path& manifestPath, const fs::path& output) {
    const auto manifest = Read(manifestPath);
    CheckManifest(manifest);
    NoReparse(output);
    if (fs::exists(output) && (!fs::is_directory(output) || !fs::is_empty(output)))
        throw std::runtime_error("export destination must be empty or absent");
    for (const auto& item : manifest.at("files")) {
        const auto source = FromUtf8(item.at("source").get<std::string>());
        NoReparse(source);
        if (Digest(source) != item.at("sha256").get<std::string>()) throw std::runtime_error("export source changed after planning");
        if (IsBinary(item.at("role").get<std::string>())) Binary(source);
    }
    Closure(manifest, {}, true);
    fs::create_directories(output);
    for (const auto& item : manifest.at("files")) {
        const auto destination = output / Relative(item.at("destination").get<std::string>());
        fs::create_directories(destination.parent_path());
        fs::copy_file(FromUtf8(item.at("source").get<std::string>()), destination);
    }
    VerifyExport(manifestPath, output);
}

void ExportProject(const fs::path& folder, const fs::path& output) {
    NoReparse(folder);
    const auto project = pixie::OpenProjectFolder(fs::canonical(folder));
    if (!project.ok) throw std::runtime_error(project.error);
    NoReparse(output);
    const auto destination = fs::weakly_canonical(fs::absolute(output));
    if (destination == project.folder || Within(project.folder, destination))
        throw std::runtime_error("export output cannot replace the project or its ancestors");
    NoReparse(destination);
    if (fs::exists(destination) && !fs::is_empty(destination)) throw std::runtime_error("export destination is not empty");
    const auto work = project.folder / "build" / "pixie-export";
    NoReparse(work);
    const bool code = fs::is_regular_file(project.folder / "CMakeLists.txt");
    const auto source = code ? project.folder : FromUtf8(PIXIE_EXPORT_ENGINE_ROOT);
    const auto cmake = FromUtf8(PIXIE_EXPORT_CMAKE);
    std::vector<std::string> configure{"-S", Utf8(source), "-B", Utf8(work), "-G", PIXIE_EXPORT_GENERATOR,
            "-DPIXIE_ENGINE_DIR=" + std::string(PIXIE_EXPORT_ENGINE_ROOT),
            "-DPIXIE_OUTPUT_ROOT=" + Utf8(work / "bin"), "-DPIXIE_DEVELOPMENT=OFF", "-DPIXIE_DEV_CHANNEL=OFF",
            "-DPIXIE_BUILD_SAMPLES=OFF", "-DPIXIE_ENGINE_BUILD_TESTS=OFF", "-DPIXIE_BUILD_TOOLS=OFF"};
    if (std::string(PIXIE_EXPORT_GENERATOR).starts_with("Visual Studio")) {
        configure.push_back("-A"); configure.push_back("x64");
    } else configure.push_back("-DCMAKE_MAKE_PROGRAM=" + std::string(PIXIE_EXPORT_MAKE_PROGRAM));
    if (RunProcess(cmake, configure) != 0 ||
        RunProcess(cmake, {"--build", Utf8(work), "--config", "Release", "--parallel", "4"}) != 0)
        throw std::runtime_error("shipping build failed; output was not exported");
    const auto products = work / "pixie-export" / "Release";
    std::vector<fs::path> manifests;
    for (const auto& entry : fs::directory_iterator(products)) if (entry.path().extension() == ".json") manifests.push_back(entry.path());
    if (manifests.size() != 1) throw std::runtime_error("export expects exactly one game product in the project");
    const auto product = Read(manifests.front());
    NoReparse(project.folder);
    const auto currentProject = pixie::OpenProjectFolder(project.folder);
    if (!currentProject.ok) throw std::runtime_error(currentProject.error);
    Plan plan(project.folder);
    plan.Binaries(product);
    plan.Asset(Utf8(currentProject.path.filename()), "project");
    plan.Scenes(currentProject);
    for (const auto& asset : product.at("assets")) plan.Asset(asset.get<std::string>());
    const auto manifest = work / "export-manifest.json";
    CheckManifest(plan.manifest);
    Write(manifest, plan.manifest);
    CopyManifest(manifest, destination);
    std::cout << "Export manifest: " << Utf8(manifest) << '\n';
}
}
