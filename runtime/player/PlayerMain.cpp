// The player of a game built with pixie_game: the engine's main, linked into the game's executable.
// It opens the project folder it belongs to (--project; else the folder pixie_game's PROJECT_DIR
// names; else the first folder above the executable that holds a .pixie file) and plays its start
// scene in a window, with the options every player takes. Not part of the runtime library: only a
// game's executable has it.

#include "pixie/Log.hpp"
#include "pixie/Run.hpp"
#include "pixie/RunOptions.hpp"
#include "pixie/runtime/ProjectApp.hpp"
#include "pixie/runtime/ProjectFile.hpp"

#include <array>
#include <filesystem>
#include <format>
#include <string>
#include <system_error>

namespace {

// The first folder, from `start` up, that holds a project file; empty when none does.
std::filesystem::path FindProjectFolder(std::filesystem::path start) {
    std::error_code error;
    for (auto folder = std::filesystem::absolute(start, error); !folder.empty(); folder = folder.parent_path()) {
        for (const auto& entry : std::filesystem::directory_iterator(folder, error)) {
            if (entry.path().extension() == std::filesystem::path(pixie::kProjectExtension)) {
                return folder;
            }
        }
        if (folder == folder.parent_path()) {
            break;
        }
    }
    return {};
}

std::string PathText(const std::filesystem::path& path) {
    const std::u8string text = path.u8string();
    return std::string(reinterpret_cast<const char*>(text.data()), text.size());
}

} // namespace

int main(int argc, char** argv) {
    pixie::RunConfig config;
    std::filesystem::path folder;
    const std::array extra = {
        pixie::RunOption{"--project", "<folder>", "the project to play (default: the one this player is in)",
                         [&](std::string_view v) {
                             folder = std::filesystem::path(std::u8string(v.begin(), v.end()));
                             return !v.empty();
                         }},
    };
    if (!pixie::ParseRunOptions(argc, argv, "player", config, extra)) {
        return 2;
    }
#ifdef PIXIE_PLAYER_PROJECT_DIR
    if (folder.empty()) {
        folder = std::filesystem::path(u8"" PIXIE_PLAYER_PROJECT_DIR);
    }
#endif
    if (folder.empty()) {
        folder = FindProjectFolder(std::filesystem::path(argv[0]).parent_path());
    }
    if (folder.empty()) {
        pixie::LogError("player: no project here; this player plays the project folder it is in, or --project <folder>");
        return 1;
    }

    pixie::SessionSettings session;
    session.updateRate = config.updateRate;
    pixie::ProjectApp app(pixie::CreateProject(), session);
    if (!app.Session().OpenProject(folder)) {
        pixie::LogError(std::format("player: {}: {}", PathText(folder), app.Session().Error()));
        return 1;
    }
    config.title = std::string(app.Session().ProjectName());
    return pixie::Run(app, config);
}
