#include "TaskEditor.h"
#include "RCalHeaderFramework.h"
#include "SQLiteCalendarStore.h"
#include "Scheduling.h"

#include <curses.h>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <stdexcept>

int main() {
    using namespace rcal;
    const Date date{2026, 11, 6};
    Task task{{}, "09:10 is description text", false, "2000-01-01", "09:10"};
    TaskEditor editor(task);
    assert(editor.selectedField() == 0);
    editor.handleInput(KEY_DOWN, true);
    assert(editor.selectedField() == 1);
    editor.handleInput(L'\t', false);
    assert(editor.selectedField() == 2);
    editor.handleInput(KEY_BTAB, true);
    assert(editor.selectedField() == 1);
    editor.handleInput(KEY_UP, true);
    editor.handleInput(L'!', false);
    assert(editor.task().text == "09:10 is description text!");
    assert(editor.task().dueTime == "09:10");
    assert(editor.handleInput(KEY_ENTER, true) == NoteEditorResult::save);
    assert(editor.handleInput(27, false) == NoteEditorResult::cancel);

    const auto path = std::filesystem::temp_directory_path() /
        ("r-cal-task-form-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".db");
    for (bool persistent : {false, true}) {
        HeaderOptions options;
        options.startDate = date;
        RCalHeaderFramework header(options);
        std::shared_ptr<SQLiteCalendarStore> store;
        if (persistent) {
            store = std::make_shared<SQLiteCalendarStore>(path);
            header.setCalendarStore(store);
        }
        header.addTask(date, editor.task());
        assert(header.selectedTask());
        assert(header.selectedTask()->text == editor.task().text);
        assert(taskOverdue(*header.selectedTask(), "2026-11-06", "10:00"));
        const auto id = header.selectedTask()->id;
        header.toggleSelectedTask();
        Task edited = TaskEditor(*header.selectedTask()).task();
        edited.text = "due:2026-11-20 stays literal in description";
        edited.dueDate = "2026-11-10";
        edited.dueTime = "12:30";
        header.updateSelectedTask(edited);
        assert(header.selectedTask()->completed);
        assert(header.selectedTask()->id == id);
        assert(header.selectedTask()->text == edited.text);
        edited.dueTime = "25:00";
        try { header.updateSelectedTask(edited); assert(false); }
        catch (const std::invalid_argument&) {}
        assert(header.selectedTask()->dueTime == "12:30");
        edited.dueDate.clear();
        edited.dueTime = "09:10";
        try { header.updateSelectedTask(edited); assert(false); }
        catch (const std::invalid_argument&) {}
        assert(header.selectedTask()->dueDate == "2026-11-10");
        edited.dueTime.clear();
        header.updateSelectedTask(edited);
        assert(header.selectedTask()->dueDate.empty());
        if (store) {
            SQLiteCalendarStore reopened(path);
            const auto saved = reopened.loadDay(date).tasks;
            assert(saved.size() == 1 && saved[0].id == id && saved[0].completed);
            assert(saved[0].text == edited.text && saved[0].dueDate.empty() && saved[0].dueTime.empty());
        }
        edited.text.clear();
        header.updateSelectedTask(edited);
        assert(!header.selectedTask());
        if (store) assert(store->loadDay(date).tasks.empty());
    }
    std::filesystem::remove(path);

    // The task modal stays in the parent input window across an input refresh.
    FILE* output = std::tmpfile();
    FILE* input = std::tmpfile();
    assert(output && input);
    SCREEN* screen = newterm("xterm", output, input);
    assert(screen);
    resizeterm(30, 100);
    werase(stdscr);
    editor.render(stdscr);
    const int top = (30 - 11) / 2;
    const int left = (100 - 88) / 2;
    assert((mvwinch(stdscr, top + 4, left + 2) & A_CHARTEXT) == 'D');
    wtimeout(stdscr, 0);
    wint_t key = 0;
    while (wget_wch(stdscr, &key) != ERR) {}
    assert((mvwinch(curscr, top + 4, left + 2) & A_CHARTEXT) == 'D');
    endwin();
    delscreen(screen);
    std::fclose(input);
    std::fclose(output);
}
