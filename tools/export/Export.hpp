#pragma once
#include <filesystem>
#include <optional>
#include "Sdk.hpp"
namespace pixie::packaging {
/// Plans and verifies a saved project export. The work root holds intermediate products.
void ExportProject(const ExportSdk& sdk, const std::filesystem::path& project, const std::filesystem::path& output,
    const std::optional<std::filesystem::path>& work = std::nullopt);
/// Replays a matching manifest after validating its source bytes and dependency closure.
void CopyManifest(const ExportSdk& sdk, const std::filesystem::path& manifest, const std::filesystem::path& output);
/// Checks the portable output tree against the manifest and selected SDK.
void VerifyExport(const ExportSdk& sdk, const std::filesystem::path& manifest, const std::filesystem::path& output);
}
