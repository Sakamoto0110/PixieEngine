#pragma once

#include "pixie/platform/Input.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace pixie {

// The native player's debug UI state. No World, renderer, or operating-system UI is needed to edit
// a command. The host applies Settings after polling input, on the thread owning the GL context.
class DebugConsole {
public:
    struct Settings {
        bool vsync;
        double frameRateCap;
        std::optional<bool> telemetry;
    };

    DebugConsole(bool vsync, double frameRateCap, std::optional<bool> telemetry = {}) : settings_{vsync, frameRateCap, telemetry} {}

    // Returns true when the key belongs to the debug UI, including the event that closes it.
    bool KeyEvent(Key key, bool down, bool repeat = false);
    void Character(unsigned int codepoint);
    void Paste(std::string_view text);
    void FocusLost() { f3Held_ = false; }
    bool Execute(std::string_view command);

    bool ShowsRates() const { return showsRates_; }
    bool IsOpen() const { return open_; }
    const Settings& CurrentSettings() const { return settings_; }
    const std::string& Line() const { return line_; }
    std::size_t Cursor() const { return cursor_; }
    const std::string& Feedback() const { return feedback_; }

private:
    static constexpr std::size_t kMaxLength = 200;
    Settings settings_;
    bool showsRates_ = false;
    bool open_ = false;
    bool f3Held_ = false;
    bool ratesBeforeF3_ = false;
    std::string line_;
    std::size_t cursor_ = 0;
    std::string previous_;
    std::string feedback_ = "-vsync true|false | -fps-cap true <15+>|false | -telemetry true|false";
};

} // namespace pixie
