#include "ParticlesProject.hpp"
#include "ParticlesScene.hpp"

#include "pixie/Run.hpp"
#include "pixie/RunOptions.hpp"
#include "pixie/runtime/ProjectApp.hpp"

#include <array>

int main(int argc, char** argv) {
    pixie::RunConfig config;
    config.title = "particles";
    ParticlesScenario scenario;
    int seed = 1;
    const std::array extra = {
        pixie::RunOption{"--count", "<n>", "particles at the start, 0 to 200000 (default 10000)",
                       [&](std::string_view v) { return pixie::ParseCount(v, scenario.count, 0, 200000); }},
        pixie::RunOption{"--attractors", "<n>", "attractors at the start, 0 to 16 (default 1)",
                       [&](std::string_view v) {
                           return pixie::ParseCount(v, scenario.attractors, 0, kMaxAttractors);
                       }},
        pixie::RunOption{"--interaction", "on|off", "particle-to-particle interaction (default on)",
                       [&](std::string_view v) {
                           scenario.interaction = v == "on";
                           return v == "on" || v == "off";
                       }},
        pixie::RunOption{"--seed", "<n>", "seed of the World's random numbers (default 1)",
                       [&](std::string_view v) { return pixie::ParseCount(v, seed, 0, 1 << 30); }},
    };
    if (!pixie::ParseRunOptions(argc, argv, "particles", config, extra)) {
        return 2;
    }
    pixie::SessionSettings session;
    session.seed = static_cast<std::uint64_t>(seed);
    session.updateRate = config.updateRate;
    pixie::ProjectApp app(std::make_unique<ParticlesProject>(scenario), session);
    return pixie::Run(app, config);
}
