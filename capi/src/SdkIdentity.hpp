#pragma once

#include "pixie_capi.h"
#include "pixie_sdk_build.h"
#include <nlohmann/json.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace pixie::sdk {

inline constexpr std::uint64_t Capabilities() noexcept {
    std::uint64_t result = PIXIE_CAPABILITY_CANDIDATE_AUTHORING;
#ifdef PIXIE_DEVELOPMENT
    result |= PIXIE_CAPABILITY_MODULE_LOADING;
#endif
#ifdef PIXIE_DEV_CHANNEL
    result |= PIXIE_CAPABILITY_NATIVE_AUTOMATION;
#endif
    return result;
}
static_assert(Capabilities() == PIXIE_SDK_EXPECTED_CAPABILITIES,
    "actual native facilities must match the selected SDK profile");

inline constexpr std::string_view RuntimeIdentity() noexcept {
    return PIXIE_SDK_RUNTIME_IDENTITY_JSON;
}

// The schema owns only these typed compatibility fields. Payload files, product display
// metadata, operation capabilities and per-module candidate admission are separate axes.
inline constexpr std::array<std::string_view, 23> kNativeMatchFields = {
    "schemaVersion", "sdkId", "sdkBuildId", "cApiVersion", "moduleIdentityContract",
    "platform", "architecture", "compilerId", "compilerVersion", "msvcFullVersion",
    "stlVersion", "stlUpdate", "iteratorDebugLevel", "cppStandard", "configuration",
    "crt", "cpuFeatures", "profile", "distributionClass", "exceptions", "rtti",
    "windowsSdkVersion", "dependencyInventorySha256",
};

inline bool Compatible(const char* text, std::string& diagnostic) {
    if (!text) { diagnostic = "game module SDK identity is null"; return false; }
    std::size_t length = 0;
    while (length <= PIXIE_SDK_IDENTITY_MAX_LENGTH && text[length] != '\0') ++length;
    if (length > PIXIE_SDK_IDENTITY_MAX_LENGTH) {
        diagnostic = "game module SDK identity exceeds the supported byte limit";
        return false;
    }
    // Reject duplicate keys and excessive nesting as malformed, rather than accepting
    // a parser's last duplicate value. The loaded module remains cooperative native code.
    bool malformed = false;
    std::vector<std::unordered_set<std::string>> keys;
    const auto callback = [&](int depth, nlohmann::json::parse_event_t event, nlohmann::json& value) {
        if (depth > 16) { malformed = true; return false; }
        if (event == nlohmann::json::parse_event_t::object_start) keys.emplace_back();
        else if (event == nlohmann::json::parse_event_t::key) {
            if (keys.empty() || !keys.back().insert(value.get<std::string>()).second) malformed = true;
        } else if (event == nlohmann::json::parse_event_t::object_end && !keys.empty()) keys.pop_back();
        return !malformed;
    };
    const auto candidate = nlohmann::json::parse(text, text + length, callback, false);
    if (malformed || candidate.is_discarded() || !candidate.is_object()) {
        diagnostic = "game module SDK identity is malformed";
        return false;
    }
    const auto expected = nlohmann::json::parse(PIXIE_SDK_NATIVE_IDENTITY_JSON);
    for (const auto field : kNativeMatchFields) {
        const auto actual = candidate.find(std::string(field));
        const auto wanted = expected.find(std::string(field));
        if (actual == candidate.end() || actual->type() != wanted->type() || *actual != *wanted) {
            diagnostic = "game module SDK identity mismatch: " + std::string(field) +
                "; rebuild with the selected SDK/profile/configuration/toolchain";
            return false;
        }
    }
    return true;
}

} // namespace pixie::sdk
