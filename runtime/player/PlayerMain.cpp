// The player of a game built with pixie_game: the engine's main, linked into the game's executable.
// It opens the project folder it belongs to (--project; else the folder pixie_game's PROJECT_DIR
// names; else the first folder above the executable that holds a .pixie file) and plays its start
// scene in a window, with the options every player takes. Not part of the runtime library: only a
// game's executable has it.

#include "pixie/core/Log.hpp"
#include "pixie/app/Run.hpp"
#include "pixie/app/RunOptions.hpp"
#include "pixie/runtime/ProjectApp.hpp"
#include "pixie/runtime/ProjectFile.hpp"
#ifdef PIXIE_DEV_CHANNEL
#include "pixie/dev/PlayerChannel.hpp"
#endif

#include <array>
#include <charconv>
#include <filesystem>
#include <format>
#include <string>
#include <system_error>
#include <vector>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

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

int PlayerMain(int argc, char** argv) {
    pixie::RunConfig config;
    std::filesystem::path folder;
    std::uint32_t physicsDebug = 0;
#ifdef PIXIE_DEV_CHANNEL
    std::string channelName;
    std::filesystem::path captureDirectory;
#endif
    const std::array extra = {
#ifdef PIXIE_DEV_CHANNEL
        pixie::RunOption{"--dev-channel", "<name>", "opt-in local single-client World channel",
            [&](std::string_view v) { channelName = v; return !v.empty(); }},
        pixie::RunOption{"--dev-capture-dir", "<folder>", "fixed output folder for channel captures",
            [&](std::string_view v) { captureDirectory = std::filesystem::path(std::u8string(v.begin(), v.end())); return !v.empty(); }},
        pixie::RunOption{"--dev-hidden", "on|off", "hide a channel-driven player window",
            [&](std::string_view v) { config.hidden = v == "on"; return v == "on" || v == "off"; }},
#endif
        pixie::RunOption{"--project", "<folder>", "the project to play (default: the one this player is in)",
                         [&](std::string_view v) {
                             folder = std::filesystem::path(std::u8string(v.begin(), v.end()));
                             return !v.empty();
                         }},
        pixie::RunOption{"--physics-debug", "<0..127>", "native physics debug bitmask (default 0)",
                         [&](std::string_view v) {
                             const auto result = std::from_chars(v.data(), v.data() + v.size(), physicsDebug);
                             return result.ec == std::errc{} && result.ptr == v.data() + v.size() && physicsDebug <= pixie::kPhysicsDebugAll2D;
                         }},
    };
    if (!pixie::ParseRunOptions(argc, argv, "player", config, extra)) {
        return 2;
    }
#ifdef PIXIE_DEV_CHANNEL
    if ((!channelName.empty() && config.quitAfterSteps) ||
        (channelName.empty() && (!captureDirectory.empty() || config.hidden))) {
        pixie::LogError("player: channel stepping and finite-step capture require separate runs; channel options require --dev-channel");
        return 2;
    }
#endif
    if (folder.empty()) {
        folder = FindProjectFolder(std::filesystem::path(
            std::u8string(reinterpret_cast<const char8_t*>(argv[0]))).parent_path());
    }
#if defined(PIXIE_DEVELOPMENT) && defined(PIXIE_PLAYER_PROJECT_DIR)
    if (folder.empty()) {
        folder = std::filesystem::path(u8"" PIXIE_PLAYER_PROJECT_DIR);
    }
#endif
    if (folder.empty()) {
        pixie::LogError("player: no project here; this player plays the project folder it is in, or --project <folder>");
        return 1;
    }

    pixie::SessionSettings session;
    session.updateRate = config.updateRate;
#ifdef PIXIE_DEVELOPMENT
    session.seed = config.seed;
#endif
    pixie::ProjectApp app(pixie::CreateProject(), session);
    app.Session().SetPhysicsDebug(physicsDebug);
    if (!app.Session().OpenProject(folder)) {
        pixie::LogError(std::format("player: {}: {}", PathText(folder), app.Session().Error()));
        return 1;
    }
    config.title = std::string(app.Session().ProjectName());
#ifdef PIXIE_DEV_CHANNEL
    std::unique_ptr<pixie::dev::PlayerChannel> channel;
    if (!channelName.empty()) {
        channel = std::make_unique<pixie::dev::PlayerChannel>(app.Session());
        if (captureDirectory.empty()) captureDirectory = std::filesystem::path("build/captures") / channelName;
        if (!channel->Start(channelName, captureDirectory)) {
            pixie::LogError("player: " + channel->Error());
            return 1;
        }
        app.Session().GetInput().SetCursor(pxPoint(-1.0, -1.0));
        app.Session().GetInput().EndUpdate();
        config.advanceFrame = [&](double delta) { channel->Advance(delta); };
        config.beforeRender = [&] { channel->Drain(); };
        config.renderAlpha = [&] { return channel->RenderAlpha(); };
        config.afterRender = [&](pixie::FramebufferSize size) { channel->AfterRender(size); };
        config.beforeStop = [&] { channel->Stop(); };
    }
#endif
    return pixie::Run(app, config);
}

#ifdef _WIN32
// Windows supplies Unicode command lines. Convert once into the same UTF-8 domain used
// by the native C API, project paths and JSON, rather than interpreting ANSI argv as UTF-8.
int wmain(int argc, wchar_t** wide) {
    std::vector<std::string> storage;
    storage.reserve(static_cast<std::size_t>(argc));
    for (int i = 0; i < argc; ++i) {
        const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide[i], -1, nullptr, 0, nullptr, nullptr);
        if (size <= 0) return 2;
        std::string text(static_cast<std::size_t>(size), '\0');
        if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide[i], -1, text.data(), size, nullptr, nullptr)) return 2;
        text.pop_back(); storage.push_back(std::move(text));
    }
    std::vector<char*> utf8;
    for (auto& text : storage) utf8.push_back(text.data());
    return PlayerMain(argc, utf8.data());
}
#else
int main(int argc, char** argv) { return PlayerMain(argc, argv); }
#endif
