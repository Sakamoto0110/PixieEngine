#pragma once

#include "pixie/RunConfig.hpp"

#include <functional>
#include <span>
#include <string_view>

namespace sample {

// One option of a sample's own, beyond the ones every sample takes.
struct Option {
    std::string_view name;      // "--count"
    std::string_view argument;  // "<n>", shown in the usage
    std::string_view help;
    // Takes the value that follows the name; returns false if it is not acceptable.
    std::function<bool(std::string_view)> parse;
};

// Parses the options every sample takes (--ups, --fps, --vsync, --quit-after, --screenshot) into
// `config`, and the sample's own `extra` ones. On an unknown option or a bad value, prints the
// usage to stderr and returns false; the sample then exits with code 2.
bool ParseOptions(int argc, char** argv, std::string_view program, pixie::RunConfig& config,
                  std::span<const Option> extra = {});

// A non-negative number.
bool ParseNumber(std::string_view text, double& value);
// A whole number in [min, max].
bool ParseCount(std::string_view text, int& value, int min, int max);

} // namespace sample
