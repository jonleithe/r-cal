#include "EventEditor.h"

#include <algorithm>
#include <curses.h>
#include <stdexcept>

namespace rcal {

EventEditor::EventEditor(Event event)
    : original_(event), fields_{{NoteEditor(event.text), NoteEditor(event.startDate),
          NoteEditor(event.time), NoteEditor(event.endDate), NoteEditor(event.endTime)}} {}

NoteEditorResult EventEditor::handleInput(wint_t key, bool keyCode) {
    if ((!keyCode && key == L'\t') || (keyCode && key == KEY_DOWN)) {
        selectedField_ = (selectedField_ + 1) % fields_.size();
        return NoteEditorResult::editing;
    }
    if (keyCode && (key == KEY_UP || key == KEY_BTAB)) {
        selectedField_ = (selectedField_ + fields_.size() - 1) % fields_.size();
        return NoteEditorResult::editing;
    }
    return fields_[selectedField_].handleInput(key, keyCode);
}

Event EventEditor::event() const {
    Event event = original_;
    event.text = fields_[0].note();
    event.startDate = fields_[1].note();
    event.time = fields_[2].note();
    event.endDate = fields_[3].note();
    event.endTime = fields_[4].note();
    return event;
}

void EventEditor::render(WINDOW* parent) const {
    int parentHeight = 0, parentWidth = 0;
    getmaxyx(parent, parentHeight, parentWidth);
    const int height = 13;
    const int width = std::min(parentWidth, 88);
    if (parentHeight < height || width < 40) {
        mvwaddnstr(parent, 0, 0, "Resize terminal to at least 40 x 13 to edit events", parentWidth);
        wrefresh(parent);
        return;
    }
    const int top = (parentHeight - height) / 2;
    const int left = (parentWidth - width) / 2;
    WINDOW* dialog = newwin(height, width, top, left);
    if (!dialog) throw std::runtime_error("cannot create event editor window");
    box(dialog, 0, 0);
    mvwaddnstr(dialog, 0, 2, " Event ", width - 4);
    mvwaddnstr(dialog, 1, 2, "Tab/Up/Down: field  Enter: save  Esc: cancel", width - 4);
    const char* labels[] = {"Description", "Start date", "Start time", "End date", "End time"};
    for (std::size_t i = 0; i < fields_.size(); ++i) {
        mvwaddstr(dialog, 3 + static_cast<int>(i), 2, labels[i]);
        fields_[i].renderField(dialog, 3 + static_cast<int>(i), 16, width - 18, false);
    }
    mvwaddnstr(dialog, 9, 2, "Dates: YYYY-MM-DD; times: HH:MM or blank", width - 4);
    mvwaddnstr(dialog, 10, 2, "Both endpoint dates included. Empty description deletes.", width - 4);
    mvwaddnstr(dialog, 11, 2, error_.c_str(), width - 4);
    fields_[selectedField_].renderField(dialog, 3 + static_cast<int>(selectedField_), 16, width - 18, true);
    curs_set(1);
    int cursorRow = 0, cursorColumn = 0;
    getyx(dialog, cursorRow, cursorColumn);
    // Input can implicitly refresh the parent, so it must contain the modal too.
    copywin(dialog, parent, 0, 0, top, left, top + height - 1, left + width - 1, 0);
    wmove(parent, top + cursorRow, left + cursorColumn);
    delwin(dialog);
    wrefresh(parent);
}

} // namespace rcal
