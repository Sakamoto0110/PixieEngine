#pragma once
#include <filesystem>
namespace pixie::packaging {
void ExportProject(const std::filesystem::path& project, const std::filesystem::path& output);
void CopyManifest(const std::filesystem::path& manifest, const std::filesystem::path& output);
void VerifyExport(const std::filesystem::path& manifest, const std::filesystem::path& output);
}
