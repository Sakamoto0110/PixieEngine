#pragma once
#include <filesystem>
#include <string>
#include <vector>
namespace pixie::packaging {
struct PeImage {
    std::vector<std::string> libraries;
    std::vector<std::string> imports;
    std::vector<std::string> exports;
    bool moduleLoaderMarker = false;
};
// Reads import, delay-import and export tables without loading or executing an image.
PeImage InspectPe(const std::filesystem::path& path);
std::string Digest(const std::filesystem::path& path);
bool IsWindowsLibrary(const std::string& name);
}
