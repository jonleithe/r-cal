#include "RCalHeaderFramework.h"
#include "SQLiteCalendarStore.h"
#include "NoteEditor.h"

#include <curses.h>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <memory>

int main() {
    using namespace rcal;
    const Date date{2026, 11, 6};
    auto* inputWindow = reinterpret_cast<WINDOW*>(1); // Delete does not access window geometry.
    const auto path = std::filesystem::temp_directory_path() /
        ("r-cal-delete-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".db");
    for (bool persistent : {false, true}) {
        HeaderOptions options;
        options.startDate = date;
        RCalHeaderFramework header(options);
        std::shared_ptr<SQLiteCalendarStore> store;
        if (persistent) {
            store = std::make_shared<SQLiteCalendarStore>(path);
            header.setCalendarStore(store);
        }
        assert(header.handleInput(KEY_DC, inputWindow) == InputResult::ignored);
        header.toggleSelectedWorkStatus(WorkStatus::workday);
        header.addNote(date, "First");
        header.addNote(date, "Keep");
        header.addTask(date, "Task");
        header.addEvent(date, Event{{}, "09:00", "Range", "2026-11-05", "2026-11-07", "17:00"});
        const auto eventId = header.selectedEvent()->id;
        header.moveNoteSelection(-3);
        assert(header.selectedItemText() == "First");
        assert(header.handleInput(KEY_DC, inputWindow) == InputResult::redraw);
        assert(header.selectedItemText() == "Keep");
        assert(header.dayInfo(date)->tasks.size() == 1);
        assert(header.dayInfo(date)->events[0].id == eventId);
        header.moveNoteSelection(1);
        assert(header.selectedTask());
        assert(header.handleInput(KEY_DC, inputWindow) == InputResult::redraw);
        assert(header.selectedEvent()->id == eventId);
        // Delete a shared event from an interior date; the prior item becomes selected.
        assert(header.handleInput(KEY_DC, inputWindow) == InputResult::redraw);
        assert(header.selectedItemText() == "Keep");
        header.selectDate({2026, 11, 5});
        assert(header.infoIndicator(header.selectedDate())[4] == ' ');
        header.selectDate({2026, 11, 7});
        assert(header.infoIndicator(header.selectedDate())[4] == ' ');
        header.selectDate(date);
        assert(header.selectedItemText() == "Keep");
        if (store) {
            SQLiteCalendarStore reopened(path);
            const auto saved = reopened.loadDay(date);
            assert(saved.notes.size() == 1 && saved.notes[0].text == "Keep");
            assert(saved.tasks.empty() && saved.events.empty());
            assert(reopened.loadDay({2026, 11, 7}).events.empty());
        }
        assert(header.handleInput(KEY_DC, inputWindow) == InputResult::redraw);
        assert(header.selectedItemText().empty());
        assert(header.dayInfo(date)->workStatus == WorkStatus::workday);
        assert(header.handleInput(KEY_DC, inputWindow) == InputResult::ignored);
        if (store) assert(store->loadDay(date).notes.empty());
    }
    std::filesystem::remove(path);
    // Delete inside a text editor edits text rather than deleting a calendar record.
    NoteEditor editor("abc");
    editor.handleInput(KEY_HOME, true);
    assert(editor.handleInput(KEY_DC, true) == NoteEditorResult::editing);
    assert(editor.note() == "bc");
}
