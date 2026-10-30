#include "pixie/app/RunOptions.hpp"

#include <charconv>
#include <cstdio>
#include <format>
#include <string>

namespace pixie {

namespace {

void PrintUsage(std::string_view program, std::span<const RunOption> extra) {
    std::string usage = std::format("usage: {} [options]\n", program);
    const auto line = [&usage](std::string_view name, std::string_view argument,
                               std::string_view help) {
        usage += std::format("  {:<26}{}\n", std::format("{} {}", name, argument), help);
    };
    line("--ups", "<hz>", "updates per second (default 60)");
    line("--fps", "<hz>", "frame cap; 0 means none (default 0)");
    line("--vsync", "on|off", "vertical sync (default on)");
    line("--telemetry", "on|off", "override host and app CPU metric collection (F3 displays it)");
    line("--quit-after", "<seconds>", "close after this long; 0 means never (default 0)");
    line("--screenshot", "<file.png>", "with --quit-after, save the last frame as a PNG");
#ifdef PIXIE_DEVELOPMENT
    line("--quit-after-steps", "<n>", "capture after exactly n fixed steps, including zero");
    line("--seed", "<n>", "initial World seed; an authored scene seed takes precedence");
    line("--size", "<w>x<h>", "capture framebuffer size in pixels");
#endif
    for (const RunOption& option : extra) {
        line(option.name, option.argument, option.help);
    }
    std::fputs(usage.c_str(), stderr);
}

bool ParseCommon(std::string_view option, std::string_view value, RunConfig& config,
                 bool& known) {
    known = true;
#ifdef PIXIE_DEVELOPMENT
    if (option == "--quit-after-steps" || option == "--seed") {
        std::uint64_t number = 0;
        const auto result = std::from_chars(value.data(), value.data() + value.size(), number);
        if (result.ec != std::errc{} || result.ptr != value.data() + value.size()) return false;
        if (option == "--quit-after-steps") config.quitAfterSteps = number;
        else config.seed = number;
        return true;
    }
    if (option == "--size") {
        const auto split = value.find('x');
        int width = 0, height = 0;
        if (split == std::string_view::npos || !ParseCount(value.substr(0, split), width, 1, 16384) ||
            !ParseCount(value.substr(split + 1), height, 1, 16384)) return false;
        config.width = width;
        config.height = height;
        config.exactFramebufferSize = true;
        return true;
    }
#endif
    if (option == "--ups") {
        return ParseNumber(value, config.updateRate) && config.updateRate > 0.0;
    }
    if (option == "--fps") {
        return ParseNumber(value, config.frameRateCap);
    }
    if (option == "--vsync") {
        config.vsync = value == "on";
        return value == "on" || value == "off";
    }
    if (option == "--telemetry") {
        if (value != "on" && value != "off") return false;
        config.telemetry = value == "on";
        return true;
    }
    if (option == "--quit-after") {
        return ParseNumber(value, config.quitAfterSeconds);
    }
    if (option == "--screenshot") {
        config.screenshotPath = std::filesystem::path(std::u8string(value.begin(), value.end()));
        return !value.empty();
    }
    known = false;
    return false;
}

} // namespace

bool ParseNumber(std::string_view text, double& value) {
    const auto* end = text.data() + text.size();
    const auto result = std::from_chars(text.data(), end, value);
    return result.ec == std::errc() && result.ptr == end && value >= 0.0;
}

bool ParseCount(std::string_view text, int& value, int min, int max) {
    const auto* end = text.data() + text.size();
    int parsed = 0;
    const auto result = std::from_chars(text.data(), end, parsed);
    if (result.ec != std::errc() || result.ptr != end || parsed < min || parsed > max) {
        return false;
    }
    value = parsed;
    return true;
}

bool ParseRunOptions(int argc, char** argv, std::string_view program, RunConfig& config,
                     std::span<const RunOption> extra) {
    for (int i = 1; i < argc; ++i) {
        const std::string_view option = argv[i];
        if (i + 1 >= argc) {
            PrintUsage(program, extra);
            return false;
        }
        const std::string_view value = argv[++i];
        bool known = false;
        bool ok = false;
        // Preserve a sample's own options, including shapes' pre-existing --seed contract.
        for (const RunOption& candidate : extra) {
            if (candidate.name == option) {
                known = true;
                ok = candidate.parse(value);
                break;
            }
        }
        if (!known) ok = ParseCommon(option, value, config, known);
        if (!known || !ok) {
            PrintUsage(program, extra);
            return false;
        }
    }
    return true;
}

} // namespace pixie
