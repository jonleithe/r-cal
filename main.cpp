#include "RCalHeaderFramework.h"

#include <curses.h>
#include <exception>
#include <iostream>

int main(int argc, char** argv)
{

    try{
        const std::string config = argc > 1 ? argv[1] : "config/rcal.conf.example";
        auto header = rcal::RCalHeaderFramework::fromConfigFile(config);

        // Initializes curses and returns the default screen window, also available as stdscr.
        initscr();

        // Makes keyboard input available immediately instead of waiting for Enter.
        // Terminal-generated signals such as Ctrl+C still work.
        cbreak();

        // Prevents typed characters from appearing automatically on screen.
        noecho();

        // Enables curses key codes such as KEY_LEFT, KEY_RIGHT and KEY_HOME.
        keypad(stdscr, true);

        bool running = true;
        while (running){
            erase();
            header.render(stdscr);

            // Copies changes made to stdscr to the physical terminal.
            refresh();

            const rcal::InputResult result = header.handleInput(getch(), stdscr);
            running = result != rcal::InputResult::quit;
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
