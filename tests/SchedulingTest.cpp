#include "Scheduling.h"
#include "RCalHeaderFramework.h"
#include "SQLiteCalendarStore.h"

#include <sqlite3.h>
#include <curses.h>
#include <cstdio>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <stdexcept>

int main() {
    using namespace rcal;
    for (const auto& invalid : {"due:2026-02-29 report", "due:2026-11-06 24:00 report",
                                "due: report", "due:2026-11-06", "due:2026-11-06 09:30"}) {
        bool rejected = false;
        try { parseScheduledText(invalid, true); }
        catch (const std::invalid_argument&) { rejected = true; }
        assert(rejected);
    }
    const auto parsed = parseScheduledText("due:2028-02-29 09:30 Send report", true);
    assert(parsed.date == "2028-02-29" && parsed.time == "09:30" && parsed.text == "Send report");
    assert(parseScheduledText("Ordinary task", true).date.empty());
    assert(parseScheduledText("09:30 Meeting", false).time == "09:30");
    Task task{"id", "Report", false, "2026-11-06", "09:30"};
    assert(!taskOverdue(task, "2026-11-06", "09:30"));
    assert(taskOverdue(task, "2026-11-06", "09:31"));
    task.completed = true;
    assert(!taskOverdue(task, "2026-11-07", "10:00"));
    task.completed = false;
    task.dueTime.clear();
    assert(!taskOverdue(task, "2026-11-06", "23:59"));
    assert(taskOverdue(task, "2026-11-07", "00:00"));

    const auto path = std::filesystem::temp_directory_path() /
        ("r-cal-schedule-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".db");
    const Date day{2026, 11, 6};
    TaskId taskId;
    EventId eventId;
    {
        auto store = std::make_shared<SQLiteCalendarStore>(path);
        HeaderOptions options;
        options.startDate = day;
        RCalHeaderFramework header(options);
        header.setCalendarStore(store);
        header.addTask(day, "due:2026-11-10 16:00 Send report");
        taskId = header.dayInfo(day)->tasks[0].id;
        assert(header.selectedItemText() == "due:2026-11-10 16:00 Send report");
        header.toggleSelectedTask();
        header.updateSelectedItem("due:2026-11-11 Revised report");
        assert(header.dayInfo(day)->tasks[0].completed);
        assert(header.dayInfo(day)->tasks[0].dueTime.empty());
        try { header.updateSelectedItem("due:2026-02-30 Invalid"); assert(false); }
        catch (const std::invalid_argument&) {}
        assert(header.dayInfo(day)->tasks[0].text == "Revised report");
        header.addEvent(day, "start:2026-11-12 09:30 Meeting");
        eventId = header.selectedEvent()->id;
        assert(header.selectedItemText() == "start:2026-11-12 09:30 Meeting");
        header.updateSelectedItem("start:2026-11-13 All day meeting");
    }
    {
        SQLiteCalendarStore store(path);
        auto info = store.loadDay(day);
        assert(info.tasks[0].id == taskId && info.tasks[0].dueDate == "2026-11-11");
        assert(info.tasks[0].completed && info.tasks[0].dueTime.empty());
        assert(info.events.empty());
        const auto events = store.loadDay({2026, 11, 13}).events;
        assert(events.size() == 1 && events[0].id == eventId);
        assert(events[0].startDate == "2026-11-13" && events[0].time.empty());
        store.updateTask(taskId, "No deadline");
        assert(store.loadDay(day).tasks[0].dueDate.empty());
    }
    std::filesystem::remove(path);

    // Version 2 migration preserves existing data and derives event starts.
    sqlite3* database = nullptr;
    assert(sqlite3_open(path.string().c_str(), &database) == SQLITE_OK);
    assert(sqlite3_exec(database,
        "CREATE TABLE day_info(date TEXT, work_status TEXT);"
        "CREATE TABLE notes(id TEXT, date TEXT, position INTEGER, text TEXT);"
        "CREATE TABLE tasks(id TEXT, date TEXT, position INTEGER, text TEXT, completed INTEGER);"
        "CREATE TABLE events(id TEXT, date TEXT, position INTEGER, time TEXT, text TEXT);"
        "INSERT INTO tasks VALUES('task', '2026-11-06', 0, 'Existing', 1);"
        "INSERT INTO events VALUES('event', '2026-11-06', 0, '09:30', 'Existing');"
        "PRAGMA user_version=2;", nullptr, nullptr, nullptr) == SQLITE_OK);
    sqlite3_close(database);
    for (int reopen = 0; reopen < 2; ++reopen) {
        SQLiteCalendarStore store(path);
        const auto info = store.loadDay(day);
        assert(info.tasks[0].completed && info.tasks[0].dueDate.empty());
        assert(info.events[0].startDate == "2026-11-06" && info.events[0].time == "09:30");
    }
    std::filesystem::remove(path);
    // Check the visible marker and actual curses colors, including selection precedence.
    FILE* output = std::tmpfile();
    FILE* input = std::tmpfile();
    assert(output && input);
    SCREEN* screen = newterm("xterm-256color", output, input);
    assert(screen);
    resizeterm(30, 120);
    start_color();
    use_default_colors();
    HeaderOptions renderOptions;
    renderOptions.startDate = day;
    RCalHeaderFramework rendered(renderOptions);
    DayInfo items;
    items.notes.push_back({"note", "Selected note"});
    items.tasks.push_back({"late", "Late task", false, "2000-01-01", ""});
    items.tasks.push_back({"done", "Completed task", true, "2000-01-01", ""});
    items.tasks.push_back({"future", "Future task", false, "9999-12-31", ""});
    items.tasks.push_back({"unscheduled", "No deadline", false, "", ""});
    rendered.setDayInfo(day, items);
    auto rowText = [](int row) {
        char text[121]{};
        mvwinnstr(stdscr, row, 0, text, 120);
        return std::string(text);
    };
    auto foreground = [](int row) {
        short fg = 0, bg = 0;
        assert(pair_content(PAIR_NUMBER(mvwinch(stdscr, row, 1)), &fg, &bg) == OK);
        return fg;
    };
    rendered.render(stdscr);
    assert(rowText(11).find("! [ ] Task: Late task") != std::string::npos);
    assert(foreground(11) == COLOR_RED);
    assert((mvwinch(stdscr, 11, 1) & A_BOLD) != 0);
    for (int row : {12, 13, 14}) {
        assert(rowText(row).find('!') == std::string::npos);
        assert(foreground(row) == COLOR_YELLOW);
    }
    rendered.moveNoteSelection(1);
    rendered.render(stdscr);
    assert(rowText(11).find("! [ ] Task: Late task") != std::string::npos);
    assert(foreground(11) == COLOR_BLACK);
    rendered.toggleSelectedTask();
    werase(stdscr);
    rendered.render(stdscr);
    assert(rowText(11).find('!') == std::string::npos);
    assert(rowText(11).find("OVERDUE") == std::string::npos);
    rendered.moveNoteSelection(-1);
    rendered.render(stdscr);
    assert(foreground(11) == COLOR_YELLOW);
    endwin();
    delscreen(screen);
    std::fclose(input);
    std::fclose(output);

}
