#include "EventEditor.h"
#include "RCalHeaderFramework.h"
#include "SQLiteCalendarStore.h"
#include "Scheduling.h"

#include <curses.h>
#include <sqlite3.h>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <stdexcept>

namespace {
void checkRange(rcal::RCalHeaderFramework& header, rcal::Date date, bool present) {
    header.selectDate(date);
    assert((header.infoIndicator(date)[4] == 'E') == present);
    const auto* info = header.dayInfo(date);
    assert((info && !info->events.empty()) == present);
}
}

int main() {
    using namespace rcal;
    const auto path = std::filesystem::temp_directory_path() /
        ("r-cal-event-range-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".db");
    // Same behavior with and without persistence, including navigation far beyond buffers.
    for (bool persistent : {false, true}) {
        HeaderOptions options;
        options.startDate = {2026, 11, 6};
        RCalHeaderFramework header(options);
        std::shared_ptr<SQLiteCalendarStore> store;
        if (persistent) {
            store = std::make_shared<SQLiteCalendarStore>(path);
            header.setCalendarStore(store);
        }
        Event event{{}, "09:30", "Conference", "2027-01-30", "2027-02-02", "17:00"};
        header.addEvent({2026, 11, 6}, event);
        assert(header.selectedDate().month == 1);
        const auto id = header.selectedEvent()->id;
        checkRange(header, {2026, 11, 6}, false);
        checkRange(header, {2027, 1, 29}, false);
        checkRange(header, {2027, 1, 30}, true);
        checkRange(header, {2027, 1, 31}, true);
        assert(header.selectedEvent()->id == id);
        event = *header.selectedEvent();
        event.endDate = "2027-02-01";
        event.text = "Changed from the middle day";
        header.updateSelectedEvent(event);
        assert(header.selectedEvent()->text == event.text);
        checkRange(header, {2027, 2, 2}, false);
        checkRange(header, {2027, 2, 1}, true);
        assert(header.selectedEvent()->id == id);
        checkRange(header, {2027, 1, 30}, true);
        assert(header.selectedEvent()->endDate == "2027-02-01");
        event = *header.selectedEvent();
        event.endDate = "2027-01-29";
        try { header.updateSelectedEvent(event); assert(false); }
        catch (const std::invalid_argument&) {}
        assert(header.selectedEvent()->endDate == "2027-02-01");
        if (store) {
            SQLiteCalendarStore reopened(path);
            assert(reopened.loadDay({2027, 1, 31}).events[0].id == id);
            assert(reopened.loadDay({2027, 2, 2}).events.empty());
        }
        // Deleting on a middle date removes the single record and all occurrences.
        checkRange(header, {2027, 1, 31}, true);
        event = *header.selectedEvent();
        event.text.clear();
        header.updateSelectedEvent(event);
        checkRange(header, {2027, 1, 30}, false);
        checkRange(header, {2027, 1, 31}, false);
        checkRange(header, {2027, 2, 1}, false);
        if (store) assert(store->loadDay({2027, 1, 31}).events.empty());
        // Date-only, year boundary, leap day, and inclusive midnight endpoint.
        header.addEvent({2026, 12, 31}, Event{{}, "", "Holiday", "2026-12-31", "2027-01-01", ""});
        checkRange(header, {2027, 1, 1}, true);
        checkRange(header, {2027, 1, 2}, false);
        header.addEvent({2028, 2, 28}, Event{{}, "23:00", "Night", "2028-02-28", "2028-03-01", "00:00"});
        checkRange(header, {2028, 2, 29}, true);
        checkRange(header, {2028, 3, 1}, true);
        checkRange(header, {2028, 3, 2}, false);
        if (persistent) {
            store.reset();
            header.setCalendarStore(nullptr);
            std::filesystem::remove(path);
        }
    }
    Event invalid{{}, "10:00", "Backwards", "2026-11-06", "2026-11-06", "09:00"};
    try { normalizeEvent(invalid, "2026-11-06"); assert(false); }
    catch (const std::invalid_argument&) {}
    EventEditor editor(Event{{}, "09:00", "Møte", "2026-11-06", "2026-11-07", "17:00"});
    assert(editor.selectedField() == 0);
    editor.handleInput(L'!', false);
    editor.handleInput(L'\t', false);
    assert(editor.selectedField() == 1);
    editor.handleInput(KEY_DOWN, true);
    assert(editor.selectedField() == 2);
    editor.handleInput(KEY_BTAB, true);
    assert(editor.selectedField() == 1);
    assert(editor.event().text == "Møte!");
    assert(editor.event().endTime == "17:00");
    assert(editor.handleInput(L'\n', false) == NoteEditorResult::save);
    assert(editor.handleInput(KEY_ENTER, true) == NoteEditorResult::save);
    assert(editor.handleInput(27, false) == NoteEditorResult::cancel);

    // Reading input may implicitly refresh stdscr. The modal must survive it.
    FILE* terminalOutput = std::tmpfile();
    FILE* terminalInput = std::tmpfile();
    assert(terminalOutput && terminalInput);
    SCREEN* screen = newterm("xterm", terminalOutput, terminalInput);
    assert(screen);
    resizeterm(30, 100);
    werase(stdscr);
    wrefresh(stdscr);
    editor.render(stdscr);
    const int labelRow = (30 - 13) / 2 + 3;
    const int labelColumn = (100 - 88) / 2 + 2;
    assert((mvwinch(curscr, labelRow, labelColumn) & A_CHARTEXT) == 'D');
    wtimeout(stdscr, 0);
    wint_t input = 0;
    // resizeterm can queue KEY_RESIZE before the nonblocking read times out.
    while (wget_wch(stdscr, &input) != ERR) {}
    assert((mvwinch(curscr, labelRow, labelColumn) & A_CHARTEXT) == 'D');
    assert((mvwinch(stdscr, labelRow, labelColumn) & A_CHARTEXT) == 'D');
    endwin();
    delscreen(screen);
    std::fclose(terminalInput);
    std::fclose(terminalOutput);

    // Version 3 events keep their start and become one-day ranges on migration.
    sqlite3* db = nullptr;
    assert(sqlite3_open(path.string().c_str(), &db) == SQLITE_OK);
    assert(sqlite3_exec(db,
        "CREATE TABLE day_info(date TEXT, work_status TEXT);"
        "CREATE TABLE notes(id TEXT, date TEXT, position INTEGER, text TEXT);"
        "CREATE TABLE tasks(id TEXT, date TEXT, position INTEGER, text TEXT, completed INTEGER, due_date TEXT, due_time TEXT);"
        "CREATE TABLE events(id TEXT, date TEXT, position INTEGER, time TEXT, text TEXT, start_date TEXT);"
        "INSERT INTO events VALUES('old-event', '2026-11-06', 0, '09:00', 'Existing', '2026-11-12');"
        "PRAGMA user_version=3;", nullptr, nullptr, nullptr) == SQLITE_OK);
    sqlite3_close(db);
    for (int reopen = 0; reopen < 2; ++reopen) {
        SQLiteCalendarStore store(path);
        assert(store.loadDay({2026, 11, 6}).events.empty());
        const auto events = store.loadDay({2026, 11, 12}).events;
        assert(events.size() == 1 && events[0].id == "old-event");
        assert(events[0].endDate == "2026-11-12" && events[0].endTime.empty());
    }
    std::filesystem::remove(path);
}
