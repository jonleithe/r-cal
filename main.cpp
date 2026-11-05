#include "RCalHeaderFramework.h"
#include "ColorSelector.h"
#include "CreateItemSelector.h"
#include "NoteEditor.h"

#include <curses.h>
#include <clocale>
#include <exception>
#include <iostream>
#include <memory>

int main(int argc, char** argv)
{

    try{
        std::setlocale(LC_ALL, "");
        const std::string config = argc > 1 ? argv[1] : "config/rcal.conf.example";
        auto header = rcal::RCalHeaderFramework::fromConfigFile(config);

        
        // rcal::DayInfo todayInfo;
        // todayInfo.workStatus = rcal::WorkStatus::workday;
        // todayInfo.notes.push_back("Example note");
        // todayInfo.taskCount = 3;
        // todayInfo.eventCount = 1;

        // header.setDayInfo(header.selectedDate(), todayInfo);

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
        bool addingNote = false;
        while (running){
            erase();
            header.render(stdscr);

            // Copies changes made to stdscr to the physical terminal.
            refresh();
            if (colorSelector) colorSelector->render(stdscr);
            if (createItemSelector) createItemSelector->render(stdscr);
            if (noteEditor) noteEditor->render(stdscr);

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
                if (result == rcal::CreateItemResult::createNote) {
                    createItemSelector.reset();
                    addingNote = true;
                    noteEditor = std::make_unique<rcal::NoteEditor>("");
                } else if (result == rcal::CreateItemResult::cancel) {
                    createItemSelector.reset();
                }
            } else if (noteEditor) {
                const auto result = noteEditor->handleInput(key, keyCode);
                if (result == rcal::NoteEditorResult::save) {
                    if (addingNote) header.addNote(header.selectedDate(), noteEditor->note());
                    else header.updateSelectedNote(noteEditor->note());
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
                if (const std::string* note = header.selectedNote()) {
                    addingNote = false;
                    noteEditor = std::make_unique<rcal::NoteEditor>(*note);
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
