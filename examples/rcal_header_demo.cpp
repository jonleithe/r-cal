#include "RCalHeaderFramework.h"

#include <curses.h>
#include <exception>
#include <iostream>

int main(int argc, char** argv) {
    try {
        const std::string config = argc > 1 ? argv[1] : "config/rcal.conf.example";
        const auto header = rcal::RCalHeaderFramework::fromConfigFile(config);
        initscr();
        cbreak();
        noecho();
        header.render(stdscr);
        refresh();
        getch();
        endwin();
    } catch (const std::exception& error) {
        if (!isendwin()) endwin();
        std::cerr << "r-cal: " << error.what() << '\n';
        return 1;
    }
}
