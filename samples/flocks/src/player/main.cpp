#include "FlocksProject.hpp"

#include "SampleOptions.hpp"

#include "pixie/Run.hpp"
#include "pixie/runtime/ProjectApp.hpp"

#include <array>

int main(int argc, char** argv) {
    pixie::RunConfig config;
    config.title = "flocks";
    FlocksScenario scenario;
    int seed = 1;
    const std::array options = {
        sample::Option{"--count", "<n>", "number of boids, 0 to 100000 (default 5000)",
                       [&scenario](std::string_view v) { return sample::ParseCount(v, scenario.count, 0, 100000); }},
        sample::Option{"--seed", "<n>", "seed of the World's random numbers (default 1)",
                       [&seed](std::string_view v) { return sample::ParseCount(v, seed, 0, 1 << 30); }},
    };
    if (!sample::ParseOptions(argc, argv, "flocks", config, options)) {
        return 2;
    }
    pixie::SessionSettings settings;
    settings.seed = static_cast<std::uint64_t>(seed);
    settings.updateRate = config.updateRate;
    pixie::ProjectApp app(std::make_unique<FlocksProject>(scenario), settings);
    return pixie::Run(app, config);
}
