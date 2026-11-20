#pragma once
#include <filesystem>
#include <map>
#include <string>
#include <nlohmann/json.hpp>

namespace pixie::packaging {
using Json = nlohmann::json;
Json ReadMetadata(const std::filesystem::path& path);
std::filesystem::path PortablePath(const std::string& text);
void NoLinks(const std::filesystem::path& path);
std::filesystem::path ExportExecutable();
void MatchNativeIdentity(const Json& expected, const Json& actual);

// Selected metadata, loaded C identity and executable provenance must agree before
// the tool calls any C++ Project/descriptor API in the paired shipping runtime.
class ExportSdk {
public:
    /// Admits the public shipping tuple and this installed exporter before project work.
    static ExportSdk Load(const std::filesystem::path& root);
    std::filesystem::path Resolve(const std::string& relative, const std::string& digest) const;
    std::filesystem::path ResolveDeclared(const std::string& relative) const;
    Json ToolIdentity() const;
    Json NoCodeProduct() const;
    /// Returns false for absent project requirements and throws for incompatible requirements.
    bool CheckRequirement(const std::filesystem::path& project) const;
    const Json& Identity() const { return identity_; }
    const Json& NativeIdentity() const { return nativeIdentity_; }
    const Json& Tool() const { return tool_; }
    const Json& Catalog() const { return catalog_; }
    const std::filesystem::path& Root() const { return root_; }
    const std::string& ManifestDigest() const { return manifestDigest_; }
private:
    std::filesystem::path root_;
    Json catalog_, identity_, nativeIdentity_, tool_;
    std::map<std::string, Json> files_;
    std::string manifestDigest_;
};
}
