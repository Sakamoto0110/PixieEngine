#include "BoidsProject.hpp"

#include "pixie/Run.hpp"
#include "pixie/RunOptions.hpp"
#include "pixie/runtime/ProjectApp.hpp"

#include <array>

int main(int argc, char** argv) {
    pixie::RunConfig config;
    config.title = "boids";
    BoidsScenario scenario;
    int seed = 1;
    const std::array options = {
        pixie::RunOption{"--count", "<n>", "number of boids, 0 to 200000 (default 1000)",
                       [&scenario](std::string_view v) { return pixie::ParseCount(v, scenario.count, 0, 200000); }},
        pixie::RunOption{"--seed", "<n>", "seed of the World's random numbers (default 1)",
                       [&seed](std::string_view v) { return pixie::ParseCount(v, seed, 0, 1 << 30); }},
    };
    if (!pixie::ParseRunOptions(argc, argv, "boids", config, options)) {
        return 2;
    }
    pixie::SessionSettings settings;
    settings.seed = static_cast<std::uint64_t>(seed);
    settings.updateRate = config.updateRate;
    pixie::ProjectApp app(std::make_unique<BoidsProject>(scenario), settings);
    return pixie::Run(app, config);
}
