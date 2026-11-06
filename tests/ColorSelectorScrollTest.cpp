#include "ColorSelector.h"

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
    rcal::ColorSelector selector({}, 8);
    auto firstLabel = [] {
        char label[25]{};
        mvwinnstr(stdscr, 2, 2, label, 24);
        return std::string(label);
    };
    auto selectedAt = [](int row) { return (mvwinch(stdscr, row, 2) & A_REVERSE) != 0; };
    selector.render(stdscr);
    const auto initialLabel = firstLabel();
    assert(selectedAt(2));
    // Selection moves down within the viewport before scrolling at the bottom.
    for (int i = 0; i < 7; ++i) {
        selector.handleInput(KEY_DOWN);
        selector.render(stdscr);
        assert(firstLabel() == initialLabel);
        assert(selectedAt(3 + i));
    }
    // Reach the last row, rendering after each key as the application does.
    for (;;) {
        const auto previous = selector.selectedRow();
        selector.handleInput(KEY_DOWN);
        selector.render(stdscr);
        if (selector.selectedRow() == previous) break;
    }
    const auto bottomLabel = firstLabel();
    assert(bottomLabel != initialLabel);
    assert(selectedAt(9));
    // Reversing direction keeps the viewport still until the top is reached.
    for (int i = 0; i < 7; ++i) {
        selector.handleInput(KEY_UP);
        selector.render(stdscr);
        assert(firstLabel() == bottomLabel);
        assert(selectedAt(8 - i));
    }
    selector.handleInput(KEY_UP);
    selector.render(stdscr);
    assert(firstLabel() != bottomLabel);
    assert(selectedAt(2));
    // Shrinking keeps the selection visible; growing removes excess bottom scroll.
    resizeterm(8, 80);
    selector.render(stdscr);
    assert(selectedAt(2));
    for (int i = 0; i < 100; ++i) selector.handleInput(KEY_UP);
    selector.render(stdscr);
    assert(firstLabel() == initialLabel);
    assert(selectedAt(2));
    resizeterm(30, 80);
    selector.render(stdscr);
    assert(firstLabel() == initialLabel);
    assert(selectedAt(2));
    endwin();
    delscreen(screen);
    std::fclose(input);
    std::fclose(output);
}
