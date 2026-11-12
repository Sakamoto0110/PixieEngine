#pragma once

#include "pixie/app/Export.hpp"

#include "pixie/app/RunConfig.hpp"

#include <functional>
#include <span>
#include <string_view>

namespace pixie {

// One option of a player's own, beyond the ones every player takes.
struct RunOption {
    std::string_view name;      // "--count"
    std::string_view argument;  // "<n>", shown in the usage
    std::string_view help;
    // Takes the value that follows the name; returns false if it is not acceptable.
    std::function<bool(std::string_view)> parse;
};

/// Parses common option/value pairs and gives matching extra options precedence.
/// On a missing or invalid value or unknown option, writes usage to stderr and returns false.
/// Earlier assignments and option callback effects remain. Player callers usually return 2.
PIXIE_APP_API bool ParseRunOptions(int argc, char** argv, std::string_view program, RunConfig& config,
                     std::span<const RunOption> extra = {});

// A non-negative number.
PIXIE_APP_API bool ParseNumber(std::string_view text, double& value);
// A whole number in [min, max].
PIXIE_APP_API bool ParseCount(std::string_view text, int& value, int min, int max);

} // namespace pixie
