#include "Sdk.hpp"
#include "PeImage.hpp"
#include "Process.hpp"
#include "pixie_capi.h"
#include "pixie_sdk_build.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <set>
#include <stdexcept>
#include <vector>
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
std::string Fold(std::string text) {
    for (auto& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return text;
}
bool Hash(const std::string& text) {
    return text.size() == 64 && std::all_of(text.begin(), text.end(), [](char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    });
}
bool Linked(const fs::path& path) {
#ifdef _WIN32
    const auto flags = GetFileAttributesW(path.c_str());
    return flags != INVALID_FILE_ATTRIBUTES && (flags & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
#else
    return fs::is_symlink(path);
#endif
}
Json RuntimeIdentity() {
    const auto length = pixie_sdk_identity(nullptr, 0);
    if (length <= 0 || length > PIXIE_SDK_IDENTITY_MAX_LENGTH)
        throw std::runtime_error("loaded runtime returned an invalid SDK identity size");
    std::vector<char> buffer(static_cast<std::size_t>(length) + 1, '\0');
    if (pixie_sdk_identity(buffer.data(), static_cast<int32_t>(buffer.size())) != length ||
        std::find(buffer.begin(), buffer.end(), '\0') != buffer.end() - 1)
        throw std::runtime_error("loaded runtime returned an inconsistent SDK identity");
    return Json::parse(buffer.data(), buffer.data() + length);
}
void LoadedPath(const fs::path& expected, bool required = false) {
#ifdef _WIN32
    const auto module = GetModuleHandleW(expected.filename().c_str());
    if (!module) {
        if (required) throw std::runtime_error("selected tool runtime is not loaded");
        return; // CRT satellites may be declared without having loaded yet.
    }
    std::array<wchar_t, 32768> name{};
    const auto length = GetModuleFileNameW(module, name.data(), static_cast<DWORD>(name.size()));
    if (length == 0 || length >= name.size() || !fs::equivalent(fs::path(name.data()), expected))
        throw std::runtime_error("loaded tool dependency differs from the selected SDK path: " + Utf8(expected));
#else
    (void)expected;
    (void)required;
#endif
}
}

Json ReadMetadata(const fs::path& path) {
    if (!fs::is_regular_file(path) || fs::file_size(path) > 16 * 1024 * 1024)
        throw std::runtime_error("missing or oversized metadata: " + Utf8(path));
    bool malformed = false;
    std::vector<std::set<std::string>> keys;
    const auto callback = [&](int depth, Json::parse_event_t event, Json& value) {
        if (depth > 32) malformed = true;
        if (event == Json::parse_event_t::object_start) keys.emplace_back();
        else if (event == Json::parse_event_t::key && (keys.empty() || !keys.back().insert(value.get<std::string>()).second)) malformed = true;
        else if (event == Json::parse_event_t::object_end && !keys.empty()) keys.pop_back();
        return !malformed;
    };
    std::ifstream input(path, std::ios::binary);
    auto result = Json::parse(input, callback, false);
    if (malformed || result.is_discarded() || !result.is_object())
        throw std::runtime_error("malformed or duplicate metadata: " + Utf8(path));
    return result;
}

fs::path PortablePath(const std::string& text) {
    if (text.empty() || text.find_first_of("\\:") != std::string::npos ||
        std::any_of(text.begin(), text.end(), [](unsigned char c) { return c < 32; }))
        throw std::runtime_error("metadata path must be normalized and relative");
    const auto path = FromUtf8(text);
    if (path.is_absolute() || path.has_root_name()) throw std::runtime_error("absolute metadata path is forbidden");
    for (const auto& part : path) {
        const auto name = Utf8(part);
        if (name.empty() || name == "." || name == ".." || name.back() == '.' || name.back() == ' ')
            throw std::runtime_error("invalid metadata path component");
    }
    if (Utf8(path) != text || text.find("//") != std::string::npos) throw std::runtime_error("metadata path is not normalized");
    return path;
}

void NoLinks(const fs::path& path) {
    for (auto current = fs::absolute(path); !current.empty();) {
        if (Linked(current)) throw std::runtime_error("metadata paths cannot traverse reparse points");
        const auto parent = current.parent_path(); if (parent == current) break; current = parent;
    }
}

fs::path ExportExecutable() {
#ifdef _WIN32
    std::array<wchar_t, 32768> name{};
    const auto length = GetModuleFileNameW(nullptr, name.data(), static_cast<DWORD>(name.size()));
    if (length == 0 || length >= name.size()) throw std::runtime_error("cannot identify exporter executable");
    return fs::canonical(fs::path(name.data()));
#else
    return fs::canonical("/proc/self/exe");
#endif
}

void MatchNativeIdentity(const Json& expected, const Json& actual) {
    static constexpr std::array<const char*, 23> fields{
        "schemaVersion", "sdkId", "sdkBuildId", "cApiVersion", "moduleIdentityContract", "platform", "architecture",
        "compilerId", "compilerVersion", "msvcFullVersion", "stlVersion", "stlUpdate", "iteratorDebugLevel", "cppStandard",
        "configuration", "crt", "cpuFeatures", "profile", "distributionClass", "exceptions", "rtti", "windowsSdkVersion",
        "dependencyInventorySha256"};
    for (const auto* field : fields)
        if (!expected.contains(field) || !actual.contains(field) || expected.at(field).type() != actual.at(field).type() || expected.at(field) != actual.at(field))
            throw std::runtime_error("selected SDK/runtime/tool identity mismatch: " + std::string(field));
}

fs::path ExportSdk::Resolve(const std::string& relative, const std::string& digest) const {
    const auto path = root_ / PortablePath(relative); NoLinks(path);
    const auto found = files_.find(Fold(relative));
    if (!Hash(digest) || found == files_.end() || found->second.at("path") != relative || found->second.at("sha256") != digest ||
        !fs::is_regular_file(path) || Digest(path) != digest)
        throw std::runtime_error("selected SDK payload missing or mixed: " + relative);
    return path;
}

fs::path ExportSdk::ResolveDeclared(const std::string& relative) const {
    const auto found = files_.find(Fold(relative));
    if (found == files_.end()) throw std::runtime_error("selected SDK does not declare installed support: " + relative);
    return Resolve(relative, found->second.at("sha256").get<std::string>());
}

ExportSdk ExportSdk::Load(const fs::path& folder) {
    ExportSdk sdk; NoLinks(folder); sdk.root_ = fs::canonical(folder);
    sdk.catalog_ = ReadMetadata(sdk.root_ / "sdk.json"); sdk.manifestDigest_ = Digest(sdk.root_ / "sdk.json");
    if (!sdk.catalog_.at("schemaVersion").is_number_integer() || sdk.catalog_.at("schemaVersion") != 1 || sdk.catalog_.at("distributionClass") != "public" ||
        !Hash(sdk.catalog_.at("sdkId").get<std::string>()))
        throw std::runtime_error("export requires the selected public SDK; private/source fallback is forbidden");
    for (const auto& item : sdk.catalog_.at("files")) {
        const auto relative = item.at("path").get<std::string>();
        const auto path = sdk.root_ / PortablePath(relative); NoLinks(path);
        const auto folded = Fold(relative);
        if (folded == "sdk.json" || !sdk.files_.emplace(folded, item).second ||
            folded.find("local-development") != std::string::npos || folded.find("devchannel") != std::string::npos ||
            folded.find("pixie-mcp") != std::string::npos || folded.find("localcommandchannel.hpp") != std::string::npos)
            throw std::runtime_error("duplicate/recursive/private SDK payload inventory");
        if (!item.at("size").is_number_unsigned() || !fs::is_regular_file(path) || fs::file_size(path) != item.at("size").get<std::uintmax_t>())
            throw std::runtime_error("SDK payload size mismatch: " + relative);
        (void)sdk.Resolve(relative, item.at("sha256").get<std::string>());
    }
    std::set<std::string> actual;
    for (const auto& entry : fs::recursive_directory_iterator(sdk.root_)) {
        NoLinks(entry.path()); if (!entry.is_regular_file()) continue;
        const auto relative = Utf8(entry.path().lexically_relative(sdk.root_));
        if (!actual.insert(Fold(relative)).second || (relative != "sdk.json" && !sdk.files_.contains(Fold(relative))))
            throw std::runtime_error("unlisted or colliding SDK payload: " + relative);
    }
    bool selected = false; std::set<std::string> tuples;
    for (const auto& entry : sdk.catalog_.at("profiles")) {
        const auto profile = entry.at("profile").get<std::string>(), config = entry.at("configuration").get<std::string>();
        if ((profile != "authoring" && profile != "shipping") || (config != "Debug" && config != "Release") ||
            (profile == "shipping" && config != "Release") || !tuples.insert(profile + "/" + config).second)
            throw std::runtime_error("public SDK contains invalid/duplicate profile tuples");
        const auto relative = "profiles/" + profile + "/" + config + "/profile.json";
        if (entry.at("manifest") != relative) throw std::runtime_error("noncanonical SDK profile path");
        const auto identity = ReadMetadata(sdk.Resolve(relative, entry.at("manifestSha256").get<std::string>()));
        if (identity.at("sdkId") != sdk.catalog_.at("sdkId") || identity.at("sdkBuildId") != entry.at("sdkBuildId") ||
            identity.at("profile") != profile || identity.at("configuration") != config || identity.at("distributionClass") != "public" ||
            identity.at("productVersion") != sdk.catalog_.at("productVersion") || identity.at("capabilities").at("nativeAutomation") != false)
            throw std::runtime_error("mixed public SDK/profile identity");
        if (profile == "shipping" && config == "Release") { sdk.identity_ = identity; selected = true; }
    }
    if (!selected) throw std::runtime_error("selected public SDK has no shipping/Release tuple");
    if (sdk.identity_.at("runtime").at("path") != "profiles/shipping/Release/bin/pixie.dll" ||
        sdk.identity_.at("player").at("path") != "profiles/shipping/Release/bin/pixie-player.exe" ||
        !sdk.identity_.at("nativeDependencies").is_array() ||
        !sdk.identity_.at("externalRuntimeDependencies").is_array() || !sdk.identity_.at("externalRuntimeDependencies").empty())
        throw std::runtime_error("shipping profile has an invalid or external native dependency closure");
    sdk.nativeIdentity_ = RuntimeIdentity();
    MatchNativeIdentity(sdk.identity_, sdk.nativeIdentity_);
    MatchNativeIdentity(sdk.identity_, Json::parse(PIXIE_SDK_NATIVE_IDENTITY_JSON));
    if (sdk.nativeIdentity_.at("productVersion") != sdk.identity_.at("productVersion"))
        throw std::runtime_error("selected SDK product provenance does not describe the loaded runtime version");
    if (pixie_capabilities() != 4 || sdk.identity_.at("capabilities").at("moduleLoading") != false ||
        sdk.identity_.at("capabilities").at("candidateAuthoring") != true)
        throw std::runtime_error("exporter must use the exact shipping runtime without modules/channel");
    sdk.tool_ = sdk.catalog_.at("tools").at("exporter");
    if (sdk.catalog_.at("features").at("pairedExport") != true)
        throw std::runtime_error("selected SDK does not declare an available paired exporter");
    if (!sdk.tool_.at("schemaVersion").is_number_integer() || sdk.tool_.at("schemaVersion") != 1 || sdk.tool_.at("sdkId") != sdk.identity_.at("sdkId") ||
        sdk.tool_.at("sdkBuildId") != sdk.identity_.at("sdkBuildId") || sdk.tool_.at("profile") != "shipping" ||
        sdk.tool_.at("configuration") != "Release" || sdk.tool_.at("descriptorSchema") != 1 ||
        sdk.tool_.at("buildProductSchema") != 2 || sdk.tool_.at("exportManifestSchema") != 2 ||
        sdk.tool_.at("toolVersion") != PIXIE_EXPORT_TOOL_VERSION)
        throw std::runtime_error("SDK exporter metadata does not match this paired tool/schema");
    const auto executable = sdk.Resolve(sdk.tool_.at("path").get<std::string>(), sdk.tool_.at("sha256").get<std::string>());
    if (sdk.tool_.at("path") != "tools/export/Release/pixie-export.exe")
        throw std::runtime_error("exporter must use the declared SDK tools/export/Release location");
    if (!fs::equivalent(executable, ExportExecutable())) throw std::runtime_error("run the export tool from the explicitly selected SDK");
    const auto toolRuntime = sdk.Resolve(sdk.tool_.at("runtime").at("path").get<std::string>(), sdk.tool_.at("runtime").at("sha256").get<std::string>());
    if (Fold(Utf8(toolRuntime.filename())) != "pixie.dll" || sdk.tool_.at("runtime").at("sha256") != sdk.identity_.at("runtime").at("sha256") || toolRuntime.parent_path() != executable.parent_path())
        throw std::runtime_error("tool runtime is not the selected executable-side shipping runtime");
    LoadedPath(toolRuntime, true);
    std::map<std::string, fs::path> toolImages{{"pixie-export.exe", executable}, {"pixie.dll", toolRuntime}};
    std::map<std::string, std::string> nativeDependencies;
    for (const auto& dependency : sdk.identity_.at("nativeDependencies")) {
        const auto path = sdk.Resolve(dependency.at("path").get<std::string>(), dependency.at("sha256").get<std::string>());
        if (path.parent_path() != sdk.root_ / "profiles/shipping/Release/bin" ||
            !nativeDependencies.emplace(Fold(Utf8(path.filename())), dependency.at("sha256").get<std::string>()).second)
            throw std::runtime_error("invalid/duplicate shipping native dependency");
    }
    if (!sdk.tool_.at("dependencies").is_array()) throw std::runtime_error("tool dependencies must be explicit");
    for (const auto& dependency : sdk.tool_.at("dependencies")) {
        if (dependency.at("kind") != "native" && dependency.at("kind") != "crt") throw std::runtime_error("unknown tool dependency role");
        const auto path = sdk.Resolve(dependency.at("path").get<std::string>(), dependency.at("sha256").get<std::string>());
        if (path.parent_path() != executable.parent_path()) throw std::runtime_error("tool dependency must be staged beside its executable");
        const auto name = Fold(Utf8(path.filename()));
        const auto expected = nativeDependencies.find(name);
        const bool crt = name.starts_with("msvcp") || name.starts_with("vcruntime") || name.starts_with("concrt");
        if (expected == nativeDependencies.end() || expected->second != dependency.at("sha256").get<std::string>() ||
            dependency.at("kind") != (crt ? "crt" : "native"))
            throw std::runtime_error("tool dependency differs from the shipping native/CRT closure");
        nativeDependencies.erase(expected);
        if (!toolImages.emplace(name, path).second) throw std::runtime_error("duplicate executable-side tool image");
        LoadedPath(path);
    }
    if (!nativeDependencies.empty()) throw std::runtime_error("tool omits declared shipping native/CRT dependencies");
    // Even two matching manifests can omit an import. Check the actual PE
    // closure so a DLL found through PATH cannot silently complete the pairing.
    for (const auto& [name, path] : toolImages)
        for (const auto& import : InspectPe(path).libraries)
            if (!IsWindowsLibrary(import) && !toolImages.contains(Fold(import)))
                throw std::runtime_error("undeclared native import in exporter closure: " + import + " (" + name + ")");
    return sdk;
}

Json ExportSdk::ToolIdentity() const {
    return {{"toolVersion", PIXIE_EXPORT_TOOL_VERSION}, {"sdkId", identity_.at("sdkId")}, {"sdkBuildId", identity_.at("sdkBuildId")},
        {"runtimeIdentity", nativeIdentity_}, {"descriptorSchema", 1}, {"buildProductSchema", 2}, {"exportManifestSchema", 2},
        {"executableSha256", tool_.at("sha256")}, {"runtimeSha256", identity_.at("runtime").at("sha256")}};
}

Json ExportSdk::NoCodeProduct() const {
    const auto& player = identity_.at("player");
    Json dependencies = Json::array();
    if (!identity_.contains("nativeDependencies") || !identity_.at("nativeDependencies").is_array() ||
        (identity_.contains("externalRuntimeDependencies") && !identity_.at("externalRuntimeDependencies").empty()))
        throw std::runtime_error("shipping SDK has no complete declared native/CRT dependency inventory");
    for (const auto& item : identity_.at("nativeDependencies")) {
        const auto relative = item.at("path").get<std::string>();
        (void)Resolve(relative, item.at("sha256").get<std::string>());
        const auto filename = Fold(Utf8(FromUtf8(relative).filename()));
        const auto crt = filename.starts_with("msvcp") || filename.starts_with("vcruntime") || filename.starts_with("concrt");
        dependencies.push_back({{"sdkPath", relative}, {"sha256", item.at("sha256")}, {"role", crt ? "runtime" : "library"}});
    }
    const auto source = Resolve(player.at("path").get<std::string>(), player.at("sha256").get<std::string>());
    return {{"format", "pixie-product"}, {"version", 2}, {"configuration", "Release"}, {"development", false}, {"dev_channel", false},
        {"sdkId", identity_.at("sdkId")}, {"sdkBuildId", identity_.at("sdkBuildId")}, {"runtimeIdentity", nativeIdentity_},
        {"runtime", {{"sdkPath", identity_.at("runtime").at("path")}, {"sha256", identity_.at("runtime").at("sha256")}}},
        {"player", {{"source", Utf8(source)}, {"destination", Utf8(source.filename())}}}, {"dependencies", dependencies}, {"assets", Json::array()}};
}

bool ExportSdk::CheckRequirement(const fs::path& project) const {
    const auto path = project / ".pixie-code" / "sdk.json";
    NoLinks(path);
    if (!fs::exists(path)) return false; // Export never adopts a legacy project.
    const auto requirement = ReadMetadata(path);
    if (!requirement.at("schemaVersion").is_number_integer() || requirement.at("schemaVersion") != 1 || requirement.at("profile") != "authoring")
        throw std::runtime_error("public export requires an authoring project requirement, never private/shipping fallback");
    for (const auto* field : {"sdkId", "platform", "architecture", "compilerVersion"})
        if (requirement.at(field).type() != identity_.at(field).type() || requirement.at(field) != identity_.at(field))
            throw std::runtime_error("project/selected shipping SDK requirement mismatch: " + std::string(field));
    const auto cpuSet = [](const Json& list) {
        if (!list.is_array()) throw std::runtime_error("portable CPU features must be an array");
        std::set<std::string> features;
        for (const auto& feature : list)
            if (!features.insert(feature.get<std::string>()).second) throw std::runtime_error("duplicate portable CPU feature");
        return features;
    };
    // Portable requirements name a feature set. Native C1 identity matching
    // still compares the SDK's canonical typed CPU array exactly.
    if (cpuSet(requirement.at("cpuFeatures")) != cpuSet(identity_.at("cpuFeatures")))
        throw std::runtime_error("project/selected shipping SDK requirement mismatch: cpuFeatures");
    static const std::set<std::string> allowed{"schemaVersion", "sdkId", "profile", "platform", "architecture", "compilerVersion", "cpuFeatures", "requiredCapabilities"};
    for (const auto& [key, value] : requirement.items()) { (void)value; if (!allowed.contains(key)) throw std::runtime_error("unknown/path-bearing portable requirement"); }
    if (requirement.contains("requiredCapabilities")) {
        if (!requirement.at("requiredCapabilities").is_array())
            throw std::runtime_error("portable requiredCapabilities must be an array");
        std::set<std::string> names;
        for (const auto& item : requirement.at("requiredCapabilities")) {
            const auto name = item.get<std::string>();
            if ((name != "moduleLoading" && name != "candidateAuthoring") || !names.insert(name).second)
                throw std::runtime_error("project requires unsupported/private authoring capabilities for public export");
        }
    }
    return true;
}
}
