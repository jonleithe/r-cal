# r-cal

A C and C++ terminal calendar project. The first component is a configurable
ncurses timeline header with grouped year, month, and ISO-week rows plus date,
day-name, and time-information rows.

## Build and test

Requirements: a C++17 compiler, CMake 3.16 or newer, and curses development
headers and libraries.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Run the interactive example from the repository root:

```sh
./build/r-cal config/rcal.conf.example
```

Press any key to exit. Copy `config/rcal.conf.example` and edit its labels,
localized names, range, layout, and `color.*` settings to customize the header.

Colors use `foreground,background` syntax followed by optional `bold`,
`underline`, or `reverse` attributes. The eight standard curses color names,
`default`, and numeric terminal color IDs are supported.
