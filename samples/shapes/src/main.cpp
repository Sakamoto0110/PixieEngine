#include "ShapesApp.hpp"

#include "SampleOptions.hpp"

#include "pixie/Run.hpp"

#include <array>

int main(int argc, char** argv) {
    pixie::RunConfig config;
    config.title = "shapes";
    int count = 10000;
    int seed = 1;
    const std::array options = {
        sample::Option{"--count", "<n>", "number of shapes, 0 to 1000000 (default 10000)",
                       [&count](std::string_view v) { return sample::ParseCount(v, count, 0, 1000000); }},
        sample::Option{"--seed", "<n>", "random seed for the shapes (default 1)",
                       [&seed](std::string_view v) { return sample::ParseCount(v, seed, 0, 1 << 30); }},
    };
    if (!sample::ParseOptions(argc, argv, "shapes", config, options)) {
        return 2;
    }
    ShapesApp app(count, static_cast<std::uint32_t>(seed));
    return pixie::Run(app, config);
}
