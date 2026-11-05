#include "Export.hpp"
#include "PeImage.hpp"
#include "Process.hpp"
#include "Product.hpp"
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
// Keep the full manifest digest in project-local caches without relying on the
// host's Windows long-path policy. These paths are for I/O; plans and logs keep
// the original absolute paths.
fs::path FileAccessPath(const fs::path& path) {
#ifdef _WIN32
    auto absolute = fs::absolute(path).lexically_normal().make_preferred();
    const auto& name = absolute.native();
    if (name.size() < MAX_PATH - 12 || name.starts_with(L"\\\\?\\")) return absolute;
    if (name.starts_with(L"\\\\")) return fs::path(L"\\\\?\\UNC\\" + name.substr(2));
    return fs::path(L"\\\\?\\" + name);
#else
    return path;
#endif
}
Json Read(const fs::path& path) {
    return ReadMetadata(FileAccessPath(path));
}
Json ReadScene(const fs::path& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot read " + Utf8(path));
    return Json::parse(input);
}
void Write(const fs::path& path, const Json& value) {
    std::ofstream output(FileAccessPath(path));
    output << value.dump(2) << '\n';
    if (!output) throw std::runtime_error("cannot write export manifest");
}
bool Reparse(const fs::path& path) {
#ifdef _WIN32
    const auto attributes = GetFileAttributesW(FileAccessPath(path).c_str());
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
            symbol == "pixie_game_candidate_contract" || symbol == "pixie_game_sdk_identity" ||
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
    if (manifest.at("format") != "pixie-export" || manifest.at("version") != 2 ||
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

#ifdef _WIN32
std::optional<std::wstring> Environment(const wchar_t* variable) {
    SetLastError(ERROR_SUCCESS);
    const auto length = GetEnvironmentVariableW(variable, nullptr, 0);
    if (length == 0) {
        if (GetLastError() == ERROR_ENVVAR_NOT_FOUND) return std::nullopt;
        return std::wstring{};
    }
    std::wstring value(length, L'\0');
    const auto actual = GetEnvironmentVariableW(variable, value.data(), length);
    if (actual >= length) throw std::runtime_error("consumer tool environment changed during discovery");
    value.resize(actual);
    return value;
}
#endif

fs::path ConsumerCMake() {
#ifdef _WIN32
    if (const auto selected = Environment(L"PIXIE_EXPORT_CMAKE")) {
        const fs::path path(*selected);
        if (!path.is_absolute() || Lower(Utf8(path.filename())) != "cmake.exe" || !fs::is_regular_file(FileAccessPath(path)))
            throw std::runtime_error("PIXIE_EXPORT_CMAKE must be the absolute path to the consumer's cmake.exe");
        return fs::canonical(path);
    }
    const auto search = Environment(L"PATH").value_or(L"");
    for (std::size_t first = 0; first < search.size();) {
        const auto last = search.find(L';', first);
        auto value = search.substr(first, last == std::wstring::npos ? last : last - first);
        if (value.size() >= 2 && value.front() == L'"' && value.back() == L'"') value = value.substr(1, value.size() - 2);
        const fs::path directory(value);
        if (directory.is_absolute()) {
            const auto candidate = directory / "cmake.exe";
            if (fs::is_regular_file(FileAccessPath(candidate))) return fs::canonical(candidate);
        }
        if (last == std::wstring::npos) break;
        first = last + 1;
    }
#endif
    throw std::runtime_error("code export needs the consumer's CMake and matching SDK toolchain; install cmake.exe on PATH or supply PIXIE_EXPORT_CMAKE");
}

void RequireCodeSupport(const ExportSdk& sdk) {
    if (!sdk.Catalog().contains("features") || !sdk.Catalog().at("features").value("projectGeneration", false))
        throw std::runtime_error("code export requires the selected SDK's installed shipping game-build support; source-checkout fallback is forbidden");
    for (const auto* path : {"cmake/PixieProject.cmake", "cmake/PixieGame.cmake", "cmake/PixieTargetDefaults.cmake",
                            "cmake/PixieExportProduct.cmake", "support/player/PlayerMain.cpp", "support/player/SceneGame.cpp",
                            "lib/cmake/PixieEngine/PixieEngineConfig.cmake"})
        (void)sdk.ResolveDeclared(path);
}

struct CodeBuild {
    fs::path cmake, build, bin, owner;
    Json identity;
};

void ValidateCodeCache(const CodeBuild& code) {
    NoReparse(code.owner);
    NoReparse(code.bin);
    if (fs::exists(FileAccessPath(code.owner))) {
        if (Read(code.owner) != code.identity) throw std::runtime_error("this export build cache belongs to another project, SDK, role or CMake; choose a new --work directory");
    } else if ((fs::exists(FileAccessPath(code.build)) &&
                (!fs::is_directory(FileAccessPath(code.build)) || !fs::is_empty(FileAccessPath(code.build)))) ||
               fs::exists(FileAccessPath(code.bin))) {
        throw std::runtime_error("refusing to reuse an unowned export build cache or binary directory; choose a new --work directory");
    }
}

CodeBuild PrepareCodeBuild(const ExportSdk& sdk, const fs::path& project, const fs::path& work) {
    RequireCodeSupport(sdk);
    NoReparse(project / "CMakeLists.txt");
    const auto cmake = ConsumerCMake();
    const auto build = work / "build";
    const auto bin = work / "bin";
    const auto owner = build / "pixie-export-owner.json";
    const Json requested{{"schemaVersion", 1}, {"project", Utf8(project)}, {"sdkRoot", Utf8(sdk.Root())},
        {"manifestSha256", sdk.ManifestDigest()}, {"sdkBuildId", sdk.Identity().at("sdkBuildId")},
        {"role", "shipping-export"}, {"configuration", "Release"}, {"cmakePath", Utf8(cmake)}, {"cmakeSha256", Digest(cmake)}};
    CodeBuild code{cmake, build, bin, owner, requested};
    ValidateCodeCache(code);
    return code;
}

// One exporter can mutate a work root at a time, including its build, binaries
// and manifest. A concurrent caller may retry or choose a different --work.
class WorkLease {
public:
    explicit WorkLease(const fs::path& work) : path_(work / ".pixie-export.lock") {
        NoReparse(path_);
        fs::create_directories(FileAccessPath(work));
#ifdef _WIN32
        const auto handle = CreateFileW(FileAccessPath(path_).c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                                       OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if (handle == INVALID_HANDLE_VALUE) {
            const auto error = GetLastError();
            if (error == ERROR_SHARING_VIOLATION || error == ERROR_LOCK_VIOLATION)
                throw std::runtime_error("another export owns this work directory; retry or choose a different --work directory");
            throw std::runtime_error("could not open export work lock (Windows error " + std::to_string(error) + ")");
        }
        BY_HANDLE_FILE_INFORMATION info{};
        if (!GetFileInformationByHandle(handle, &info) || (info.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY))) {
            CloseHandle(handle);
            throw std::runtime_error("export work lock is not a regular, unlinked file");
        }
        handle_ = handle;
#else
        if (!fs::create_directory(path_))
            throw std::runtime_error("another export owns this work directory; retry or choose a different --work directory");
#endif
    }
    WorkLease(const WorkLease&) = delete;
    WorkLease& operator=(const WorkLease&) = delete;
    ~WorkLease() {
#ifdef _WIN32
        CloseHandle(handle_);
#else
        std::error_code ignored;
        fs::remove(path_, ignored);
#endif
    }
private:
    fs::path path_;
#ifdef _WIN32
    HANDLE handle_ = INVALID_HANDLE_VALUE;
#endif
};

void ArchiveProductDeclarations(const fs::path& build) {
    const auto existing = build / "pixie-export";
    NoReparse(existing);
    if (!fs::exists(FileAccessPath(existing))) return;
    if (!fs::is_directory(FileAccessPath(existing))) throw std::runtime_error("the owned export product path is not a directory");
    const auto history = build / "previous-export-products";
    NoReparse(history);
    fs::create_directories(FileAccessPath(history));
    for (std::size_t index = 1;; ++index) {
        const auto destination = history / ("attempt-" + std::to_string(index));
        NoReparse(destination);
        if (fs::exists(FileAccessPath(destination))) continue;
        fs::rename(FileAccessPath(existing), FileAccessPath(destination));
        return;
    }
}

Json BuildCodeProduct(const ExportSdk& sdk, const fs::path& project, const CodeBuild& code) {
    const auto& [cmake, build, bin, owner, requested] = code;
    ValidateCodeCache(code);
    if (!fs::exists(FileAccessPath(owner))) {
        fs::create_directories(FileAccessPath(build));
        Write(owner, requested);
    }
    // Only declarations produced by this configure may select a target. Keep
    // prior declarations for diagnosis without letting a removed one survive.
    ArchiveProductDeclarations(build);

    std::cout << "Configuring the selected SDK's shipping/Release consumer project\n" << std::flush;
    if (RunProcess(cmake, {"-S", Utf8(project), "-B", Utf8(build), "-DPIXIE_ENGINE_MODE=PACKAGE", "-DPIXIE_ENGINE_DIR=",
            "-DPIXIE_PROJECT_BUILD_ROLE=shipping-export", "-DPIXIE_SDK_ROOT=" + Utf8(sdk.Root()), "-DPIXIE_SDK_PROFILE=shipping",
            "-DPIXIE_PROJECT_ADOPT_SDK=OFF", "-DPIXIE_OUTPUT_ROOT=" + Utf8(bin), "-DPIXIE_DEVELOPMENT=OFF", "-DPIXIE_DEV_CHANNEL=OFF",
            "-DPixieEngine_DIR=" + Utf8(sdk.Root() / "lib/cmake/PixieEngine"), "-DCMAKE_BUILD_TYPE=Release",
            "-DCMAKE_CONFIGURATION_TYPES=Release", "-DCMAKE_SYSTEM_VERSION=" + sdk.Identity().at("windowsSdkVersion").get<std::string>()}) != 0)
        throw std::runtime_error("could not configure the installed shipping export; check the selected SDK toolchain and build log");

    const auto products = build / "pixie-export" / "Release";
    NoReparse(products);
    if (!fs::is_directory(FileAccessPath(products))) throw std::runtime_error("configured project did not declare an installed shipping export product");
    std::vector<fs::path> declarations;
    for (const auto& item : fs::directory_iterator(FileAccessPath(products))) {
        const auto path = products / item.path().filename();
        NoReparse(path);
        if (item.is_regular_file() && path.extension() == ".json") declarations.push_back(path);
    }
    if (declarations.size() != 1) throw std::runtime_error("code export requires exactly one declared shipping product; use a clean cache or explicit project wiring");
    const auto productPath = declarations.front();
    const auto target = Utf8(productPath.stem());
    if (target.empty() || target.front() == '-' || !std::all_of(target.begin(), target.end(), [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '.' || c == '-' || c == '+';
    })) throw std::runtime_error("shipping product does not name a valid consumer build target");
    const auto readProduct = [&] {
        NoReparse(productPath);
        auto product = Read(productPath);
        CheckProductIdentity(sdk, product);
        const auto player = FromUtf8(product.at("player").at("source").get<std::string>());
        if (!player.is_absolute() || player.parent_path().lexically_normal() != (bin / "Release").lexically_normal() ||
            Utf8(player.filename()) != product.at("player").at("destination").get<std::string>())
            throw std::runtime_error("shipping player must be the declared executable in this export's owned Release binary directory");
        NoReparse(player);
        return product;
    };
    (void)readProduct();
    std::cout << "Building installed shipping target: " << target << '\n' << std::flush;
    if (RunProcess(cmake, {"--build", Utf8(build), "--config", "Release", "--target", target}) != 0)
        throw std::runtime_error("could not build the declared shipping game target; see the consumer build log");
    std::cout << "Build product: " << Utf8(productPath) << '\n';
    return readProduct();
}

class Plan {
public:
    Plan(fs::path project, const ExportSdk& sdk) : project_(fs::canonical(project)) {
        manifest = ExportProvenance(sdk);
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
    void Binaries(const ExportSdk& sdk, const Json& product) {
        const auto binaries = ProductBinaries(sdk, product);
        for (const auto& binary : binaries) {
            Add(binary.source, binary.destination, binary.role);
            if (binary.role == "player") manifest["player"] = binary.destination;
        }
        for (const auto& binary : binaries)
            for (const auto& name : InspectPe(binary.source).libraries)
                if (!sources_.contains(Lower(name)) && !IsWindowsLibrary(name))
                    throw std::runtime_error("missing explicitly declared native dependency: " + name);
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
            const auto data = ReadScene(scene);
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

void VerifyExport(const ExportSdk& sdk, const fs::path& manifestPath, const fs::path& output) {
    const auto manifest = Read(manifestPath);
    CheckExportIdentity(sdk, manifest);
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

void CopyManifest(const ExportSdk& sdk, const fs::path& manifestPath, const fs::path& output) {
    const auto manifest = Read(manifestPath);
    CheckExportIdentity(sdk, manifest);
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
    VerifyExport(sdk, manifestPath, output);
}

void ExportProject(const ExportSdk& sdk, const fs::path& folder, const fs::path& output, const std::optional<fs::path>& requestedWork) {
    NoReparse(folder);
    const bool hasRequirement = sdk.CheckRequirement(fs::canonical(folder));
    const auto project = pixie::OpenProjectFolder(fs::canonical(folder));
    if (!project.ok) throw std::runtime_error(project.error);
    NoReparse(output);
    const auto destination = fs::weakly_canonical(fs::absolute(output));
    if (destination == project.folder || Within(project.folder, destination))
        throw std::runtime_error("export output cannot replace the project or its ancestors");
    NoReparse(destination);
    if (fs::exists(destination) && !fs::is_empty(destination)) throw std::runtime_error("export destination is not empty");
    // The manifest digest already binds both SDK and shipping build identities.
    // Stacking two full digests exhausted ordinary Windows paths for the default cache.
    const auto work = requestedWork ? fs::absolute(*requestedWork) : project.folder / ".local" / "pixie-export" /
        "shipping" / sdk.ManifestDigest();
    NoReparse(work);
    const bool code = fs::is_regular_file(project.folder / "CMakeLists.txt");
    if (code && !hasRequirement) throw std::runtime_error("code export needs a compatible portable authoring SDK requirement; record it explicitly in the Editor");
    const auto codeBuild = code ? std::optional{PrepareCodeBuild(sdk, project.folder, work)} : std::nullopt;
    const WorkLease lease(work);
    const auto product = codeBuild ? BuildCodeProduct(sdk, project.folder, *codeBuild) : sdk.NoCodeProduct();
    if (!code) {
        const auto productPath = work / "pixie-export" / "Release" / "no-code.json";
        NoReparse(productPath);
        fs::create_directories(FileAccessPath(productPath.parent_path()));
        Write(productPath, product);
    }
    NoReparse(project.folder);
    const auto currentProject = pixie::OpenProjectFolder(project.folder);
    if (!currentProject.ok) throw std::runtime_error(currentProject.error);
    Plan plan(project.folder, sdk);
    plan.manifest["project"] = Utf8(currentProject.path.filename());
    plan.Binaries(sdk, product);
    plan.Asset(Utf8(currentProject.path.filename()), "project");
    plan.Scenes(currentProject);
    for (const auto& asset : product.at("assets")) plan.Asset(asset.get<std::string>());
    const auto manifest = work / "export-manifest.json";
    NoReparse(manifest);
    fs::create_directories(FileAccessPath(work));
    CheckManifest(plan.manifest);
    Write(manifest, plan.manifest);
    CopyManifest(sdk, manifest, destination);
    std::cout << "Export manifest: " << Utf8(manifest) << '\n';
}
}
