#pragma once
#include <filesystem>
#include <string>
#include <vector>
namespace pixie::packaging {
struct PeImage {
    std::vector<std::string> libraries;
    std::vector<std::string> imports;
    std::vector<std::string> exports;
    bool developmentMarker = false;
};
/// Reads PE64 imports, delay imports, exports and forwarders without loading the image.
PeImage InspectPe(const std::filesystem::path& path);
std::string Digest(const std::filesystem::path& path);
/// Checks the explicit Windows system-library policy before app-local closure checks.
bool IsWindowsLibrary(const std::string& name);
}
