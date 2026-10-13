#include "ParticlesProject.hpp"
#include "ParticlesScene.hpp"

#include "SampleOptions.hpp"

#include "pixie/Run.hpp"
#include "pixie/runtime/ProjectApp.hpp"

#include <array>

int main(int argc, char** argv) {
    pixie::RunConfig config;
    config.title = "particles";
    ParticlesSettings settings;
    int seed = 1;
    const std::array extra = {
        sample::Option{"--count", "<n>", "particles at the start, 0 to 200000 (default 10000)",
                       [&](std::string_view v) { return sample::ParseCount(v, settings.count, 0, 200000); }},
        sample::Option{"--attractors", "<n>", "attractors at the start, 0 to 16 (default 1)",
                       [&](std::string_view v) {
                           return sample::ParseCount(v, settings.attractors, 0, kMaxAttractors);
                       }},
        sample::Option{"--interaction", "on|off", "particle-to-particle interaction (default on)",
                       [&](std::string_view v) {
                           settings.interaction = v == "on";
                           return v == "on" || v == "off";
                       }},
        sample::Option{"--seed", "<n>", "seed of the World's random numbers (default 1)",
                       [&](std::string_view v) { return sample::ParseCount(v, seed, 0, 1 << 30); }},
    };
    if (!sample::ParseOptions(argc, argv, "particles", config, extra)) {
        return 2;
    }
    pixie::SessionSettings session;
    session.seed = static_cast<std::uint64_t>(seed);
    session.updateRate = config.updateRate;
    pixie::ProjectApp app(std::make_unique<ParticlesProject>(settings), session);
    return pixie::Run(app, config);
}
