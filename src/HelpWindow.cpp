#include "HelpWindow.h"

#include <algorithm>
#include <curses.h>
#include <stdexcept>
#include <string>
#include <vector>

namespace rcal {
namespace {
const char* const helpLines[] = {
    "CALENDAR",
    "Left / Right       Previous / next day",
    "Page Up / Down     Previous / next screen of dates",
    "Home or n          Return to today",
    "Up / Down          Select a note, task, or event",
    "Insert             Create a note, task, or event",
    "Enter              Edit the selected item",
    "Delete             Delete the selected item immediately",
    "x                  Complete / reopen the selected task",
    "c                  Open the color picker",
    "?                  Open this help window",
    "q / Q / Escape     Quit the calendar",
    "",
    "WORK STATUS (selected date)",
    "w: workday   t: travelling   o: out-of-office working",
    "a: compensatory leave   f: vacation   Space: clear status",
    "",
    "CREATING ITEMS",
    "Up / Down or n / t / e: choose Note / Task / Event",
    "Enter: create   Escape: cancel",
    "",
    "EDITORS",
    "Tab / Shift+Tab or Up / Down: change task/event field",
    "Left / Right, Home / End: move within text",
    "Backspace / Delete: remove a character",
    "Enter: save   Escape: cancel without saving",
    "An empty description deletes an existing item.",
    "Task deadlines: YYYY-MM-DD and optional local HH:MM.",
    "Clear both deadline fields for a task without a deadline.",
    "Overdue incomplete tasks show ! and OVERDUE; completed tasks do not.",
    "Events have start/end dates and optional local HH:MM times.",
    "Events appear on both endpoint dates and every date between them.",
    "Editing or deleting an event affects its whole date range.",
    "",
    "COLOR PICKER",
    "Up / Down: choose an element's foreground or background",
    "Left / Right: choose a color   b / u / r: bold / underline / reverse",
    "Enter: apply and save to config   Escape: discard changes",
    "Selected item controls the active row; Overdue task controls overdue rows.",
    "",
    "INFO INDICATORS",
    "Five positions: work status / holiday / note / task count / events",
    "Work: W / T / O / A / F   H: holiday   N: note   E: event",
    "Task count: 1-9, or + for ten or more tasks",
    "The bottom status bar expands the selected date's indicators."
};

std::vector<std::string> wrappedLines(int width) {
    std::vector<std::string> result;
    for (const auto* source : helpLines) {
        std::string line(source);
        while (line.size() > static_cast<std::size_t>(width)) {
            auto split = line.rfind(' ', static_cast<std::size_t>(width));
            if (split == std::string::npos || split == 0) split = static_cast<std::size_t>(width);
            result.push_back(line.substr(0, split));
            line.erase(0, split);
            const auto next = line.find_first_not_of(' ');
            line.erase(0, next == std::string::npos ? line.size() : next);
        }
        result.push_back(line);
    }
    return result;
}
}

bool HelpWindow::handleInput(int key) {
    const auto maximum = lineCount_ > visibleLines_ ? lineCount_ - visibleLines_ : 0;
    switch (key) {
        case '?': case 27: case 'q': case 'Q': case '\n': case '\r': case KEY_ENTER:
            return true;
        case KEY_UP: if (firstLine_) --firstLine_; break;
        case KEY_DOWN: firstLine_ = std::min(maximum, firstLine_ + 1); break;
        case KEY_PPAGE: firstLine_ -= std::min(firstLine_, visibleLines_); break;
        case KEY_NPAGE: firstLine_ = std::min(maximum, firstLine_ + visibleLines_); break;
        case KEY_HOME: firstLine_ = 0; break;
        case KEY_END: firstLine_ = maximum; break;
        default: break;
    }
    return false;
}

void HelpWindow::render(WINDOW* parent) {
    const int height = std::min(getmaxy(parent), 30);
    const int width = std::min(getmaxx(parent), 96);
    if (height < 7 || width < 32) {
        mvwaddnstr(parent, 0, 0, "Help: enlarge terminal; Esc closes", getmaxx(parent));
        curs_set(0);
        return;
    }
    const auto lines = wrappedLines(width - 4);
    lineCount_ = lines.size();
    visibleLines_ = static_cast<std::size_t>(height - 4);
    firstLine_ = std::min(firstLine_, lineCount_ > visibleLines_ ? lineCount_ - visibleLines_ : 0);
    const int top = (getmaxy(parent) - height) / 2;
    const int left = (getmaxx(parent) - width) / 2;
    WINDOW* dialog = newwin(height, width, top, left);
    if (!dialog) throw std::runtime_error("cannot create help window");
    box(dialog, 0, 0);
    mvwaddnstr(dialog, 0, 2, " r-cal help ", width - 4);
    for (std::size_t i = 0; i < visibleLines_ && firstLine_ + i < lines.size(); ++i)
        mvwaddnstr(dialog, 2 + static_cast<int>(i), 2, lines[firstLine_ + i].c_str(), width - 4);
    const std::string footer = "Up/Down PgUp/PgDn: scroll  ?/Esc: close  " +
        std::to_string(firstLine_ + 1) + "-" + std::to_string(std::min(lineCount_, firstLine_ + visibleLines_)) +
        "/" + std::to_string(lineCount_);
    mvwaddnstr(dialog, height - 2, 2, footer.c_str(), width - 4);
    copywin(dialog, parent, 0, 0, top, left, top + height - 1, left + width - 1, 0);
    delwin(dialog);
    curs_set(0);
    // The main loop refreshes the composed screen once, after the overlay.
}

} // namespace rcal
