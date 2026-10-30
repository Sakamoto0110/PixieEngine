#include "Check.hpp"
#include "pixie/app/Run.hpp"
#include "pixie/app/RunOptions.hpp"

#include <limits>
#include <string>
#include <vector>

#ifdef PIXIE_DEVELOPMENT
namespace {
bool Options(pixie::RunConfig& config, std::vector<std::string> values) {
    std::vector<char*> arguments;
    for (auto& value : values) arguments.push_back(value.data());
    return pixie::ParseRunOptions(static_cast<int>(arguments.size()), arguments.data(), "capture-test", config);
}

struct CaptureProbe final : pixie::App {
    std::uint64_t steps = 0;
    bool valid = true;
    bool rendered = false;
    void Update(const pixie::FrameTime& time, const pixie::Input&) override {
        valid = valid && time.index == steps && time.delta == 1.0 / 60.0;
        ++steps;
    }
    void Render(const pixie::FrameTime& time, const pixie::Input&) override {
        rendered = true;
        valid = valid && time.alpha == 1.0;
    }
};
}

TEST(DeterministicCaptureUsesExactStepCountsAndAlphaOne) {
    for (const std::uint64_t count : {0ull, 1ull, 17ull}) {
        pixie::RunConfig config;
        config.width = config.height = 64;
        config.quitAfterSteps = count;
        config.quitAfterSeconds = 0.000001; // A step budget takes precedence over a wall timeout.
        config.vsync = false;
        CaptureProbe probe;
        CHECK(pixie::Run(probe, config) == 0);
        CHECK(probe.steps == count && probe.rendered && probe.valid);
    }
}

TEST(DeterministicCaptureOptionsRejectMalformedAndOverflowedValues) {
    pixie::RunConfig config;
    CHECK(Options(config, {"test", "--quit-after-steps", "0", "--seed", "18446744073709551615", "--size", "320x180"}));
    CHECK(config.quitAfterSteps == 0 && config.seed == std::numeric_limits<std::uint64_t>::max());
    CHECK(config.width == 320 && config.height == 180 && config.exactFramebufferSize);
    for (const auto* value : {"-1", "+1", "1.5", "18446744073709551616", "1x"}) {
        CHECK(!Options(config, {"test", "--quit-after-steps", value}));
        CHECK(!Options(config, {"test", "--seed", value}));
    }
    for (const auto* value : {"0x180", "320x0", "320x180x2", "320X180", "999999999999x1"}) {
        CHECK(!Options(config, {"test", "--size", value}));
    }
}

TEST(SampleSeedHandlersKeepTheirExistingValidation) {
    pixie::RunConfig config;
    int sampleSeed = 1;
    const pixie::RunOption sampleOption{"--seed", "<n>", "sample seed",
        [&](std::string_view value) { return pixie::ParseCount(value, sampleSeed, 0, 8); }};
    char name[] = "test", option[] = "--seed", value[] = "7";
    char* arguments[] = {name, option, value};
    CHECK(pixie::ParseRunOptions(3, arguments, "test", config, {&sampleOption, 1}));
    CHECK(sampleSeed == 7 && config.seed == 1);
    value[0] = '9';
    CHECK(!pixie::ParseRunOptions(3, arguments, "test", config, {&sampleOption, 1}));
}
#endif
