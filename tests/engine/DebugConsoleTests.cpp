#include "Check.hpp"
#include "DebugConsole.hpp"

using pixie::DebugConsole;
using pixie::Key;

namespace {
void Chord(DebugConsole& console) {
    console.KeyEvent(Key::F3, true);
    console.KeyEvent(Key::C, true);
    console.KeyEvent(Key::C, false);
    console.KeyEvent(Key::F3, false);
}
}

TEST(DebugShortcutsDoNotLeakOrChangeTheOtherPanel) {
    DebugConsole console(false, 0);
    CHECK(!console.KeyEvent(Key::A, true));
    CHECK(console.KeyEvent(Key::F3, true));
    CHECK(console.ShowsRates());
    console.KeyEvent(Key::F3, true, true);
    CHECK(console.ShowsRates());
    console.KeyEvent(Key::F3, false);
    Chord(console);
    CHECK(console.ShowsRates() && console.IsOpen());
    CHECK(console.KeyEvent(Key::Escape, true));
    CHECK(!console.IsOpen());
    console.KeyEvent(Key::F3, true);
    console.KeyEvent(Key::F3, false);
    CHECK(!console.ShowsRates());
    Chord(console);
    CHECK(console.IsOpen() && !console.ShowsRates());
    Chord(console);
    CHECK(!console.IsOpen() && !console.ShowsRates());
}

TEST(DebugCommandValidationKeepsIndependentSettings) {
    DebugConsole console(true, 100);
    CHECK(console.Execute("-fps-cap true 15"));
    CHECK(console.CurrentSettings().frameRateCap == 15 && console.CurrentSettings().vsync);
    CHECK(console.Execute(" -fps-cap\ttrue 60 "));
    CHECK(console.Execute("-vsync false"));
    CHECK(console.CurrentSettings().frameRateCap == 60 && !console.CurrentSettings().vsync);
    const char* invalid[] = {"", "-vsync", "-vsync 1", "-vsync True", "-vsync true extra",
        "-fps-cap true", "-fps-cap 60", "-fps-cap false 60", "-fps-cap true 14",
        "-fps-cap true 0", "-fps-cap true -60", "-fps-cap true 60.5", "-fps-cap true 60fps",
        "-fps-cap true 99999999999999999999", "-fps-cap true nan", "-fps-cap true inf",
        "-fps-cap true 60 extra", "-unknown true"};
    for (const char* command : invalid) {
        CHECK(!console.Execute(command));
        CHECK(console.CurrentSettings().frameRateCap == 60 && !console.CurrentSettings().vsync);
    }
    CHECK(console.Execute("-fps-cap false"));
    CHECK(console.Execute("-vsync true"));
    CHECK(console.CurrentSettings().frameRateCap == 0 && console.CurrentSettings().vsync);
}

TEST(DebugCommandEditingHandlesCaretHistoryPasteAndShortcutCharacter) {
    DebugConsole console(false, 0);
    console.Character('x');
    CHECK(console.Line().empty());
    console.KeyEvent(Key::F3, true);
    console.KeyEvent(Key::C, true);
    console.Character('c');
    console.KeyEvent(Key::C, false);
    console.KeyEvent(Key::F3, false);
    CHECK(console.Line().empty());
    console.Paste("-vsync falsX");
    console.KeyEvent(Key::Left, true);
    console.KeyEvent(Key::Delete, true);
    console.Character('e');
    CHECK(console.Line() == "-vsync false");
    console.KeyEvent(Key::Home, true);
    console.Character('!');
    console.KeyEvent(Key::Backspace, true);
    console.KeyEvent(Key::End, true);
    CHECK(console.Cursor() == console.Line().size());
    console.KeyEvent(Key::Enter, true);
    CHECK(!console.CurrentSettings().vsync && console.Line().empty());
    console.KeyEvent(Key::Up, true);
    CHECK(console.Line() == "-vsync false");
    console.KeyEvent(Key::Down, true);
    console.Paste("abc\n\r\t");
    console.Character(0xE9);
    CHECK(console.Line() == "abc");
    console.Paste(std::string(1000, 'x'));
    CHECK(console.Line().size() == 200 && console.Cursor() == 200);
    console.FocusLost();
    CHECK(console.KeyEvent(Key::A, true));
}
