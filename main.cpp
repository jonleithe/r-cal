#include "RCalHeaderFramework.h"
#include "SQLiteCalendarStore.h"
#include "ColorSelector.h"
#include "CreateItemSelector.h"
#include "NoteEditor.h"
#include "EventEditor.h"
#include "TaskEditor.h"
#include "HelpWindow.h"
#include "Scheduling.h"

#include <curses.h>
#include <clocale>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {

std::filesystem::path defaultDataFile() {
    if (const char* dataHome = std::getenv("XDG_DATA_HOME"); dataHome && *dataHome)
        return std::filesystem::path(dataHome) / "r-cal" / "r-cal.db";
    if (const char* userHome = std::getenv("HOME"); userHome && *userHome)
        return std::filesystem::path(userHome) / ".local" / "share" / "r-cal" / "r-cal.db";
    throw std::runtime_error("cannot determine data directory; set XDG_DATA_HOME");
}

} // namespace

int main(int argc, char** argv)
{

    try{
        std::setlocale(LC_ALL, "");
        const std::string config = argc > 1 ? argv[1] : "config/rcal.conf.example";
        auto header = rcal::RCalHeaderFramework::fromConfigFile(config);
        const auto& configuredDataFile = header.options().dataFile;
        header.setCalendarStore(std::make_shared<rcal::SQLiteCalendarStore>(
            configuredDataFile.empty() ? defaultDataFile()
                                       : std::filesystem::path(configuredDataFile)));

        // Initializes curses and returns the default screen window, also available as stdscr.
        initscr();

        // Makes keyboard input available immediately instead of waiting for Enter.
        // Terminal-generated signals such as Ctrl+C still work.
        cbreak();

        // Prevents typed characters from appearing automatically on screen.
        noecho();

        // Enables curses key codes such as KEY_LEFT, KEY_RIGHT and KEY_HOME.
        keypad(stdscr, true);

        if (has_colors()) {
            start_color();
            use_default_colors();
        }

        timeout(1000); // Refresh overdue labels even while no key is pressed.
        bool running = true;
        std::string editorError;
        std::unique_ptr<rcal::ColorSelector> colorSelector;
        std::unique_ptr<rcal::CreateItemSelector> createItemSelector;
        std::unique_ptr<rcal::NoteEditor> noteEditor;
        std::unique_ptr<rcal::EventEditor> eventEditor;
        std::unique_ptr<rcal::TaskEditor> taskEditor;
        std::unique_ptr<rcal::HelpWindow> helpWindow;
        bool addingItem = false;
        rcal::DayItemType editingItemType = rcal::DayItemType::note;
        while (running){
            erase();
            header.render(stdscr, 0, 0, noteEditor && addingItem);

            // Compose the calendar and active dialog before updating the terminal.
            if (colorSelector) colorSelector->render(stdscr);
            if (createItemSelector) createItemSelector->render(stdscr);
            if (eventEditor) eventEditor->render(stdscr);
            if (taskEditor) taskEditor->render(stdscr);
            if (noteEditor) {
                const auto* info = header.dayInfo(header.selectedDate());
                const std::size_t index = addingItem
                    ? (info ? info->notes.size() + info->tasks.size() + info->events.size() : 0)
                    : header.selectedItemFlatIndex();
                const std::string hint = !editorError.empty() ? editorError :
                    "Note: Enter: save  Esc: cancel";
                mvhline(LINES - 1, 0, ' ', COLS);
                mvaddnstr(LINES - 1, 0, hint.c_str(), COLS - 1);
                noteEditor->render(stdscr, header.noteRow(stdscr, index), index);
            }

            if (helpWindow) helpWindow->render(stdscr);
            refresh();

            wint_t key = 0;
            const int inputType = get_wch(&key);
            if (inputType == ERR) continue;
            const bool keyCode = inputType == KEY_CODE_YES;
            if (helpWindow) {
                if (helpWindow->handleInput(static_cast<int>(key))) helpWindow.reset();
            } else if (colorSelector) {
                const auto result = colorSelector->handleInput(static_cast<int>(key));
                if (result == rcal::ColorSelectorResult::save) {
                    rcal::saveColorsToConfig(config, colorSelector->colors());
                    header.setColors(colorSelector->colors());
                    colorSelector.reset();
                } else if (result == rcal::ColorSelectorResult::cancel) {
                    colorSelector.reset();
                }
            } else if (createItemSelector) {
                const auto result = createItemSelector->handleInput(static_cast<int>(key));
                if (result == rcal::CreateItemResult::createNote ||
                    result == rcal::CreateItemResult::createTask ||
                    result == rcal::CreateItemResult::createEvent) {
                    createItemSelector.reset();
                    addingItem = true;
                    editingItemType = result == rcal::CreateItemResult::createTask
                        ? rcal::DayItemType::task : result == rcal::CreateItemResult::createEvent
                        ? rcal::DayItemType::event : rcal::DayItemType::note;
                    if (editingItemType == rcal::DayItemType::event) {
                        rcal::Event event;
                        event.startDate = event.endDate = rcal::calendarDateString(header.selectedDate());
                        eventEditor = std::make_unique<rcal::EventEditor>(event);
                    } else if (editingItemType == rcal::DayItemType::task) {
                        rcal::Task task;
                        task.dueDate = rcal::calendarDateString(header.selectedDate());
                        taskEditor = std::make_unique<rcal::TaskEditor>(task);
                    } else {
                        noteEditor = std::make_unique<rcal::NoteEditor>("");
                    }
                } else if (result == rcal::CreateItemResult::cancel) {
                    createItemSelector.reset();
                }
            } else if (taskEditor) {
                const auto result = taskEditor->handleInput(key, keyCode);
                if (result == rcal::NoteEditorResult::save) {
                    try {
                        if (addingItem) header.addTask(header.selectedDate(), taskEditor->task());
                        else header.updateSelectedTask(taskEditor->task());
                        taskEditor.reset();
                    } catch (const std::invalid_argument& error) {
                        taskEditor->setError(error.what());
                    }
                } else if (result == rcal::NoteEditorResult::cancel) {
                    taskEditor.reset();
                }
            } else if (eventEditor) {
                const auto result = eventEditor->handleInput(key, keyCode);
                if (result == rcal::NoteEditorResult::save) {
                    try {
                        if (addingItem) header.addEvent(header.selectedDate(), eventEditor->event());
                        else header.updateSelectedEvent(eventEditor->event());
                        eventEditor.reset();
                    } catch (const std::invalid_argument& error) {
                        eventEditor->setError(error.what());
                    }
                } else if (result == rcal::NoteEditorResult::cancel) {
                    eventEditor.reset();
                }
            } else if (noteEditor) {
                const auto result = noteEditor->handleInput(key, keyCode);
                if (result == rcal::NoteEditorResult::save) {
                    try {
                        if (addingItem) {
                            header.addNote(header.selectedDate(), noteEditor->note());
                        } else {
                            header.updateSelectedItem(noteEditor->note());
                        }
                        noteEditor.reset();
                        editorError.clear();
                    } catch (const std::invalid_argument& error) {
                        editorError = error.what();
                    }
                } else if (result == rcal::NoteEditorResult::cancel) {
                    noteEditor.reset();
                    editorError.clear();
                }
            } else if (!keyCode && key == L'?') {
                helpWindow = std::make_unique<rcal::HelpWindow>();
            } else if (!keyCode && (key == L'c' || key == L'C')) {
                colorSelector = std::make_unique<rcal::ColorSelector>(
                    header.colors(), has_colors() ? COLORS : 0);
            } else if (keyCode && key == KEY_IC) {
                createItemSelector = std::make_unique<rcal::CreateItemSelector>();
            } else if ((!keyCode && (key == L'\n' || key == L'\r')) ||
                       (keyCode && key == KEY_ENTER)) {
                const std::string item = header.selectedItemText();
                if (!item.empty()) {
                    addingItem = false;
                    if (const auto* event = header.selectedEvent())
                        eventEditor = std::make_unique<rcal::EventEditor>(*event);
                    else if (const auto* task = header.selectedTask())
                        taskEditor = std::make_unique<rcal::TaskEditor>(*task);
                    else noteEditor = std::make_unique<rcal::NoteEditor>(item);
                }
            } else {
                const rcal::InputResult result = header.handleInput(
                    static_cast<int>(key), stdscr);
                running = result != rcal::InputResult::quit;
            }
        }

        // Exits curses mode and restores normal terminal behavior, including echoing,
        // line buffering, and cursor handling.
        endwin();

    }
    catch (const std::exception& error){
        if (!isendwin()) endwin();
        std::cerr << "r-cal: " << error.what() << '\n';
        return 1;
    }
}
