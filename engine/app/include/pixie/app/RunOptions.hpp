#pragma once

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

// Parses the options every player takes (--ups, --fps, --vsync, --telemetry, --quit-after, --screenshot) into
// `config`, and the player's own `extra` ones. On an unknown option or a bad value, prints the
// usage to stderr and returns false; the player then exits with code 2.
bool ParseRunOptions(int argc, char** argv, std::string_view program, RunConfig& config,
                     std::span<const RunOption> extra = {});

// A non-negative number.
bool ParseNumber(std::string_view text, double& value);
// A whole number in [min, max].
bool ParseCount(std::string_view text, int& value, int min, int max);

} // namespace pixie
