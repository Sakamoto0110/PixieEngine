#include "FlocksApp.hpp"

#include "SampleOptions.hpp"

#include "pixie/Run.hpp"

#include <array>

int main(int argc, char** argv) {
    pixie::RunConfig config;
    config.title = "flocks";
    int count = 5000;
    int seed = 1;
    const std::array options = {
        sample::Option{"--count", "<n>", "number of boids, 0 to 100000 (default 5000)",
                       [&count](std::string_view v) { return sample::ParseCount(v, count, 0, 100000); }},
        sample::Option{"--seed", "<n>", "seed of the World's random numbers (default 1)",
                       [&seed](std::string_view v) { return sample::ParseCount(v, seed, 0, 1 << 30); }},
    };
    if (!sample::ParseOptions(argc, argv, "flocks", config, options)) {
        return 2;
    }
    FlocksApp app(count, static_cast<std::uint64_t>(seed));
    return pixie::Run(app, config);
}
