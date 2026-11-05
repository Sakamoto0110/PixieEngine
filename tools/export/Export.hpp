#pragma once
#include <filesystem>
#include <optional>
#include "Sdk.hpp"
namespace pixie::packaging {
void ExportProject(const ExportSdk& sdk, const std::filesystem::path& project, const std::filesystem::path& output,
    const std::optional<std::filesystem::path>& work = std::nullopt);
void CopyManifest(const ExportSdk& sdk, const std::filesystem::path& manifest, const std::filesystem::path& output);
void VerifyExport(const ExportSdk& sdk, const std::filesystem::path& manifest, const std::filesystem::path& output);
}
