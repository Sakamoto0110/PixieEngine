#include "DebugConsole.hpp"

#include <charconv>
#include <format>
#include <vector>

namespace pixie {
namespace {

std::vector<std::string_view> Words(std::string_view text) {
    std::vector<std::string_view> words;
    while (!text.empty()) {
        const auto start = text.find_first_not_of(" \t\r\n");
        if (start == std::string_view::npos) break;
        text.remove_prefix(start);
        const auto end = text.find_first_of(" \t\r\n");
        words.push_back(text.substr(0, end));
        if (end == std::string_view::npos) break;
        text.remove_prefix(end);
    }
    return words;
}

} // namespace

bool DebugConsole::KeyEvent(Key key, bool down, bool repeat) {
    if (key == Key::F3) {
        if (!down) {
            f3Held_ = false;
        } else if (!repeat && !f3Held_) {
            f3Held_ = true;
            ratesBeforeF3_ = showsRates_;
            showsRates_ = !showsRates_;
        }
        return true;
    }
    if (key == Key::C && f3Held_) {
        if (down && !repeat) {
            // A chord opens the command bar without also changing the read-only panel.
            showsRates_ = ratesBeforeF3_;
            open_ = !open_;
        }
        return true;
    }
    if (!open_) return false;
    if (!down) return true;
    switch (key) {
    case Key::Escape: open_ = false; break;
    case Key::Enter:
    case Key::KeypadEnter:
        if (!repeat) {
            Execute(line_);
            previous_ = line_;
            line_.clear();
            cursor_ = 0;
        }
        break;
    case Key::Backspace:
        if (cursor_ > 0) line_.erase(--cursor_, 1);
        break;
    case Key::Delete:
        if (cursor_ < line_.size()) line_.erase(cursor_, 1);
        break;
    case Key::Left: if (cursor_ > 0) --cursor_; break;
    case Key::Right: if (cursor_ < line_.size()) ++cursor_; break;
    case Key::Home: cursor_ = 0; break;
    case Key::End: cursor_ = line_.size(); break;
    case Key::Up: line_ = previous_; cursor_ = line_.size(); break;
    case Key::Down: line_.clear(); cursor_ = 0; break;
    default: break;
    }
    return true;
}

void DebugConsole::Character(unsigned int codepoint) {
    // GLFW emits a character for the chord's C as well. It is a shortcut, not command text.
    // Commands use printable ASCII; do not keep partial UTF-8 sequences in this small text editor.
    if (open_ && !f3Held_ && codepoint >= 32 && codepoint <= 126 && line_.size() < kMaxLength) {
        line_.insert(cursor_++, 1, static_cast<char>(codepoint));
    }
}

void DebugConsole::Paste(std::string_view text) {
    for (const unsigned char character : text) Character(character);
}

bool DebugConsole::Execute(std::string_view command) {
    const auto words = Words(command);
    if (words.size() == 2 && words[0] == "-telemetry" && (words[1] == "true" || words[1] == "false")) {
        settings_.telemetry = words[1] == "true";
        feedback_ = std::format("CPU telemetry collection: {} (F3 displays it)", *settings_.telemetry ? "on" : "off");
        return true;
    }
    if (words.size() == 2 && words[0] == "-vsync" && (words[1] == "true" || words[1] == "false")) {
        settings_.vsync = words[1] == "true";
        feedback_ = std::format("VSync: {}", settings_.vsync ? "on" : "off");
        return true;
    }
    if (words.size() == 2 && words[0] == "-fps-cap" && words[1] == "false") {
        settings_.frameRateCap = 0;
        feedback_ = "FPS cap: off (VSync still applies when enabled)";
        return true;
    }
    if (words.size() == 3 && words[0] == "-fps-cap" && words[1] == "true") {
        int rate = 0;
        const auto text = words[2];
        const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), rate);
        if (error == std::errc{} && end == text.data() + text.size() && rate >= 15) {
            settings_.frameRateCap = rate;
            feedback_ = std::format("FPS cap: {} (VSync still applies when enabled)", rate);
            return true;
        }
    }
    feedback_ = "Usage: -vsync true|false | -fps-cap true <15+>|false | -telemetry true|false";
    return false;
}

} // namespace pixie
