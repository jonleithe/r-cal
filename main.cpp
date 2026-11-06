#include "RCalHeaderFramework.h"
#include "SQLiteCalendarStore.h"
#include "ColorSelector.h"
#include "CreateItemSelector.h"
#include "NoteEditor.h"

#include <curses.h>
#include <clocale>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>

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

        bool running = true;
        std::unique_ptr<rcal::ColorSelector> colorSelector;
        std::unique_ptr<rcal::CreateItemSelector> createItemSelector;
        std::unique_ptr<rcal::NoteEditor> noteEditor;
        bool addingItem = false;
        rcal::DayItemType editingItemType = rcal::DayItemType::note;
        while (running){
            erase();
            header.render(stdscr, 0, 0, noteEditor && addingItem);

            // Copies changes made to stdscr to the physical terminal.
            refresh();
            if (colorSelector) colorSelector->render(stdscr);
            if (createItemSelector) createItemSelector->render(stdscr);
            if (noteEditor) {
                const auto* info = header.dayInfo(header.selectedDate());
                const std::size_t index = addingItem
                    ? (info ? info->notes.size() + info->tasks.size() + info->events.size() : 0)
                    : header.selectedItemFlatIndex();
                noteEditor->render(stdscr, header.noteRow(stdscr, index), index);
            }

            wint_t key = 0;
            const int inputType = get_wch(&key);
            if (inputType == ERR) continue;
            const bool keyCode = inputType == KEY_CODE_YES;
            if (colorSelector) {
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
                    noteEditor = std::make_unique<rcal::NoteEditor>("");
                } else if (result == rcal::CreateItemResult::cancel) {
                    createItemSelector.reset();
                }
            } else if (noteEditor) {
                const auto result = noteEditor->handleInput(key, keyCode);
                if (result == rcal::NoteEditorResult::save) {
                    if (addingItem) {
                        if (editingItemType == rcal::DayItemType::note)
                            header.addNote(header.selectedDate(), noteEditor->note());
                        else if (editingItemType == rcal::DayItemType::task)
                            header.addTask(header.selectedDate(), noteEditor->note());
                        else
                            header.addEvent(header.selectedDate(), noteEditor->note());
                    } else {
                        header.updateSelectedItem(noteEditor->note());
                    }
                    noteEditor.reset();
                } else if (result == rcal::NoteEditorResult::cancel) {
                    noteEditor.reset();
                }
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
                    noteEditor = std::make_unique<rcal::NoteEditor>(item);
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
