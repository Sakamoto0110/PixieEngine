#pragma once
#include "Sdk.hpp"
#include <vector>

namespace pixie::packaging {
struct ProductBinary {
    std::filesystem::path source;
    std::string destination;
    std::string role;
};
// Build products and game manifests have their own role policy; neither is an SDK validator.
void CheckProductIdentity(const ExportSdk& sdk, const Json& product);
std::vector<ProductBinary> ProductBinaries(const ExportSdk& sdk, const Json& product);
void CheckExportIdentity(const ExportSdk& sdk, const Json& manifest);
Json ExportProvenance(const ExportSdk& sdk);
}
