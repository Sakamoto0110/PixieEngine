#include "HelloGlApp.hpp"

#include "pixie/core/Log.hpp"
#include "pixie/app/Run.hpp"

#include <charconv>
#include <cstdio>
#include <string_view>

namespace {

constexpr const char* kUsage =
    "usage: hello-gl [--screenshot <file.png>] [--ups <hz>] [--fps <hz>] [--vsync on|off] [--quit-after <seconds>]\n"
    "  --ups         updates per second (default 60)\n"
    "  --fps         frame cap; 0 means none (default 0)\n"
    "  --vsync       on or off (default on)\n"
    "  --quit-after  close after this many seconds; 0 means never (default 0)\n";

bool ParseNumber(std::string_view text, double& value) {
    const auto* end = text.data() + text.size();
    const auto result = std::from_chars(text.data(), end, value);
    return result.ec == std::errc() && result.ptr == end && value >= 0.0;
}

bool ParseArguments(int argc, char** argv, pixie::RunConfig& config) {
    for (int i = 1; i < argc; ++i) {
        const std::string_view option = argv[i];
        if (i + 1 >= argc) {
            return false;
        }
        const std::string_view value = argv[++i];
        if (option == "--ups") {
            if (!ParseNumber(value, config.updateRate) || config.updateRate <= 0.0) {
                return false;
            }
        } else if (option == "--fps") {
            if (!ParseNumber(value, config.frameRateCap)) {
                return false;
            }
        } else if (option == "--vsync") {
            if (value != "on" && value != "off") {
                return false;
            }
            config.vsync = value == "on";
        } else if (option == "--quit-after") {
            if (!ParseNumber(value, config.quitAfterSeconds)) {
                return false;
            }
        } else if (option == "--screenshot") {
            config.screenshotPath = value;
        } else {
            return false;
        }
    }
    return true;
}

} // namespace

int main(int argc, char** argv) {
    pixie::RunConfig config;
    config.title = "hello GL";
    if (!ParseArguments(argc, argv, config)) {
        std::fputs(kUsage, stderr);
        return 2;
    }
    HelloGlApp app;
    return pixie::Run(app, config);
}
