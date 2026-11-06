#include "CreateItemSelector.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>

#include <curses.h>

namespace rcal {
namespace {

const std::array<const char*, 3> itemLabels{{
    "Note", "Task (due date/time fields)", "Event (start and end fields)"
}};

} // namespace

CreateItemResult CreateItemSelector::handleInput(int key) {
    switch (key) {
        case KEY_UP:
            if (selectedItem_ > 0) --selectedItem_;
            break;
        case KEY_DOWN:
            if (selectedItem_ + 1 < itemLabels.size()) ++selectedItem_;
            break;
        case 'n': case 'N': selectedItem_ = 0; break;
        case 't': case 'T': selectedItem_ = 1; break;
        case 'e': case 'E': selectedItem_ = 2; break;
        case '\n': case '\r': case KEY_ENTER:
            if (selectedItem_ == 0) return CreateItemResult::createNote;
            if (selectedItem_ == 1) return CreateItemResult::createTask;
            if (selectedItem_ == 2) return CreateItemResult::createEvent;
            break;
        case 27: return CreateItemResult::cancel;
        default: break;
    }
    return CreateItemResult::editing;
}

void CreateItemSelector::render(WINDOW* parent) const {
    int parentHeight = 0;
    int parentWidth = 0;
    getmaxyx(parent, parentHeight, parentWidth);
    const int height = std::min(9, parentHeight);
    const int width = std::min(48, parentWidth);
    if (height < 7 || width < 24) return;

    const int top = (parentHeight - height) / 2;
    const int left = (parentWidth - width) / 2;
    WINDOW* dialog = newwin(height, width, top, left);
    if (!dialog) throw std::runtime_error("cannot create item selector window");
    box(dialog, 0, 0);
    mvwaddnstr(dialog, 0, 2, " Create new ", width - 4);
    mvwaddnstr(dialog, 1, 2, "Up/Down: choose  Enter: create  Esc: cancel", width - 4);
    for (std::size_t item = 0; item < itemLabels.size(); ++item) {
        if (item == selectedItem_) wattron(dialog, A_REVERSE);
        mvwaddnstr(dialog, static_cast<int>(item) + 3, 3, itemLabels[item], width - 6);
        if (item == selectedItem_) wattroff(dialog, A_REVERSE);
    }
    if (selectedItem_ == 1)
        mvwaddnstr(dialog, height - 2, 2, "Edit description, due date, and due time", width - 4);
    else if (selectedItem_ == 2)
        mvwaddnstr(dialog, height - 2, 2, "Edit start/end dates and times in the form", width - 4);

    copywin(dialog, parent, 0, 0, top, left, top + height - 1, left + width - 1, 0);
    delwin(dialog);
    wrefresh(parent);
}

std::size_t CreateItemSelector::selectedItem() const noexcept {
    return selectedItem_;
}

} // namespace rcal
