#include "BoidsApp.hpp"

#include "SampleOptions.hpp"

#include "pixie/Run.hpp"

#include <array>

int main(int argc, char** argv) {
    pixie::RunConfig config;
    config.title = "boids";
    int count = 1000;
    int seed = 1;
    const std::array options = {
        sample::Option{"--count", "<n>", "number of boids, 0 to 200000 (default 1000)",
                       [&count](std::string_view v) { return sample::ParseCount(v, count, 0, 200000); }},
        sample::Option{"--seed", "<n>", "seed of the World's random numbers (default 1)",
                       [&seed](std::string_view v) { return sample::ParseCount(v, seed, 0, 1 << 30); }},
    };
    if (!sample::ParseOptions(argc, argv, "boids", config, options)) {
        return 2;
    }
    BoidsApp app(count, static_cast<std::uint64_t>(seed));
    return pixie::Run(app, config);
}
