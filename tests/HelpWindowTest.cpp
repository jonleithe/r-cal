#include "HelpWindow.h"

#include <curses.h>
#include <cassert>
#include <cstdio>
#include <string>

int main() {
    FILE* output = std::tmpfile();
    FILE* input = std::tmpfile();
    assert(output && input);
    SCREEN* screen = newterm("xterm", output, input);
    assert(screen);
    resizeterm(12, 80);
    rcal::HelpWindow help;
    auto firstLine = [] {
        char text[77]{};
        mvwinnstr(stdscr, 2, 2, text, 76);
        return std::string(text);
    };
    help.render(stdscr);
    wrefresh(stdscr);
    const auto initial = firstLine();
    assert(initial.find("CALENDAR") == 0);
    assert(!help.handleInput(KEY_NPAGE));
    help.render(stdscr);
    assert(firstLine() != initial);
    help.handleInput(KEY_HOME);
    help.render(stdscr);
    assert(firstLine() == initial);
    help.handleInput(KEY_END);
    help.render(stdscr);
    assert(firstLine() != initial);
    assert(!help.handleInput(KEY_DC));
    resizeterm(30, 96);
    help.render(stdscr);
    wrefresh(stdscr);
    wtimeout(stdscr, 0);
    wint_t key = 0;
    while (wget_wch(stdscr, &key) != ERR) {}
    assert((mvwinch(curscr, 0, 3) & A_CHARTEXT) == 'r');
    assert(help.handleInput('?'));
    assert(help.handleInput(27));
    resizeterm(5, 20);
    help.render(stdscr);
    assert(help.handleInput(27));
    endwin();
    delscreen(screen);
    std::fclose(input);
    std::fclose(output);
}
