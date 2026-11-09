#include "Product.hpp"
#include "Process.hpp"
#include <algorithm>
#include <cctype>
#include <set>
#include <stdexcept>

namespace pixie::packaging {
namespace {
namespace fs = std::filesystem;
std::string Lower(std::string text) {
    for (auto& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return text;
}
void Version(const Json& value, const char* format) {
    if (!value.contains("version") || !value.at("version").is_number_integer() || value.at("version") != 2)
        throw std::runtime_error("legacy/unsupported product or export manifest; create a fresh export with the selected SDK (scene v1 remains supported)");
    if (value.at("format") != format || value.at("development") != false || value.at("dev_channel") != false)
        throw std::runtime_error("export requires a development-free shipping manifest/product");
}
void Selected(const ExportSdk& sdk, const Json& value) {
    if (value.at("sdkId") != sdk.Identity().at("sdkId") || value.at("sdkBuildId") != sdk.Identity().at("sdkBuildId"))
        throw std::runtime_error("mixed selected SDK/product identity");
    MatchNativeIdentity(sdk.Identity(), value.at("runtimeIdentity"));
}
}

void CheckProductIdentity(const ExportSdk& sdk, const Json& product) {
    Version(product, "pixie-product");
    if (product.contains("sourceDevelopment") || product.at("configuration") != "Release")
        throw std::runtime_error("shipping export requires an explicit installed shipping/Release product");
    Selected(sdk, product);
    const auto& runtime = product.at("runtime");
    if (runtime.at("sdkPath") != sdk.Identity().at("runtime").at("path") || runtime.at("sha256") != sdk.Identity().at("runtime").at("sha256"))
        throw std::runtime_error("product runtime is not the selected SDK shipping runtime");
    if (!product.at("player").is_object() || !product.at("dependencies").is_array() || !product.at("assets").is_array())
        throw std::runtime_error("invalid shipping product player/dependencies/assets schema");
}

std::vector<ProductBinary> ProductBinaries(const ExportSdk& sdk, const Json& product) {
    CheckProductIdentity(sdk, product);
    const auto& runtime = product.at("runtime");
    const auto runtimePath = sdk.Resolve(runtime.at("sdkPath").get<std::string>(), runtime.at("sha256").get<std::string>());
    const auto& player = product.at("player");
    const auto playerPath = FromUtf8(player.at("source").get<std::string>());
    const auto destination = player.at("destination").get<std::string>();
    if (!playerPath.is_absolute() || PortablePath(destination).has_parent_path() || Lower(Utf8(FromUtf8(destination).extension())) != ".exe")
        throw std::runtime_error("product player requires an explicit source and portable exe basename");
    std::vector<ProductBinary> result{{playerPath, destination, "player"}, {runtimePath, "pixie.dll", "library"}};
    std::set<std::string> destinations{Lower(destination), "pixie.dll"};
    for (const auto& dependency : product.at("dependencies")) {
        if (dependency.contains("sdkPath") == dependency.contains("source"))
            throw std::runtime_error("product dependency requires exactly one SDK or local source reference");
        const auto role = dependency.at("role").get<std::string>();
        if (role != "library" && role != "runtime") throw std::runtime_error("invalid native dependency role");
        fs::path source; std::string name;
        if (dependency.contains("sdkPath")) {
            source = sdk.Resolve(dependency.at("sdkPath").get<std::string>(), dependency.at("sha256").get<std::string>());
            name = Utf8(source.filename());
        } else {
            source = FromUtf8(dependency.at("source").get<std::string>());
            name = dependency.at("destination").get<std::string>();
            if (!source.is_absolute() || role == "runtime") throw std::runtime_error("CRT inputs must come from the selected SDK inventory");
        }
        if (PortablePath(name).has_parent_path() || Lower(Utf8(FromUtf8(name).extension())) != ".dll" || !destinations.insert(Lower(name)).second)
            throw std::runtime_error("duplicate/invalid native dependency destination");
        NoLinks(source);
        if (!fs::is_regular_file(source)) throw std::runtime_error("missing explicitly declared native dependency: " + Utf8(source));
        result.push_back({source, name, role});
    }
    for (const auto& asset : product.at("assets")) (void)PortablePath(asset.get<std::string>());
    return result;
}

Json ExportProvenance(const ExportSdk& sdk) {
    const auto& identity = sdk.Identity();
    return {{"format", "pixie-export"}, {"version", 2}, {"development", false}, {"dev_channel", false},
        {"engine_version", sdk.NativeIdentity().at("productVersion")}, {"sdkId", identity.at("sdkId")}, {"sdkBuildId", identity.at("sdkBuildId")},
        {"runtimeIdentity", sdk.NativeIdentity()}, {"exporter", {{"toolVersion", sdk.Tool().at("toolVersion")}, {"sha256", sdk.Tool().at("sha256")}}},
        {"descriptor", {{"schemaVersion", 1}, {"sdkBuildId", identity.at("sdkBuildId")}, {"runtimeSha256", identity.at("runtime").at("sha256")}}},
        {"files", Json::array()}};
}

void CheckExportIdentity(const ExportSdk& sdk, const Json& manifest) {
    Version(manifest, "pixie-export"); Selected(sdk, manifest);
    if (!manifest.at("files").is_array()) throw std::runtime_error("export files must be an array");
    if (manifest.at("engine_version") != sdk.Identity().at("productVersion") || manifest.at("descriptor").at("schemaVersion") != 1 ||
        manifest.at("descriptor").at("sdkBuildId") != sdk.Identity().at("sdkBuildId") ||
        manifest.at("descriptor").at("runtimeSha256") != sdk.Identity().at("runtime").at("sha256") ||
        manifest.at("exporter").at("toolVersion") != sdk.Tool().at("toolVersion") || manifest.at("exporter").at("sha256") != sdk.Tool().at("sha256"))
        throw std::runtime_error("mixed exporter/descriptor/runtime provenance");
    std::map<std::string, std::pair<std::string, std::string>> required;
    required.emplace("pixie.dll", std::pair{sdk.Identity().at("runtime").at("sha256").get<std::string>(), std::string("library")});
    for (const auto& dependency : sdk.Identity().at("nativeDependencies")) {
        const auto name = Lower(Utf8(FromUtf8(dependency.at("path").get<std::string>()).filename()));
        const bool crt = name.starts_with("msvcp") || name.starts_with("vcruntime") || name.starts_with("concrt");
        if (!required.emplace(name, std::pair{dependency.at("sha256").get<std::string>(), std::string(crt ? "runtime" : "library")}).second)
            throw std::runtime_error("duplicate selected shipping dependency");
    }
    std::set<std::string> seen;
    for (const auto& file : manifest.at("files"))
        if (const auto found = required.find(Lower(file.at("destination").get<std::string>())); found != required.end()) {
            if (!seen.insert(found->first).second || file.at("sha256") != found->second.first || file.at("role") != found->second.second)
                throw std::runtime_error("game manifest differs from the mandatory selected shipping native/CRT closure");
        }
    if (seen.size() != required.size()) throw std::runtime_error("game manifest omits mandatory selected shipping native/CRT dependencies");
}
}
