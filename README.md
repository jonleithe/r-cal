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
`start_date` accepts either an ISO date (`YYYY-MM-DD`) or `today`.

Colors use `foreground,background` syntax followed by optional `bold`,
`underline`, or `reverse` attributes. The eight standard curses color names,
`default`, and numeric terminal color IDs are supported. The current date is
highlighted with `color.current_day`, and the cursor is hidden while the header
is displayed.

## Navigation

The initially selected date is centered in the visible timeline. The framework
keeps a 90-day buffer on both sides and rebuilds it when fewer than 30 hidden
days remain. These values are configured with `buffer_days` and
`rebuild_threshold_days`.

- Left/Right or `h`/`l`: select the previous or next day.
- Page Up/Page Down: move by one visible screen.
- Home or `t`: return to today.
- `q`, `Q`, or Escape: quit.

Today remains highlighted with `color.current_day`; a different selected date
uses `color.selected_day`. Resizing the terminal recalculates the number of
visible days and keeps the selected date centered.

## Debugging in VS Code

Install Microsoft's C/C++ extension, open the repository folder, set a
breakpoint, and select **Run and Debug → Debug r-cal (GDB)** or press `F5`.
The launch profile creates a Debug build with symbols before starting GDB.

The application runs in VS Code's integrated terminal because curses requires
a terminal. Use `F10` to step over, `F11` to step into, `Shift+F11` to step out,
and `F5` to continue. If the program is paused while curses is active, terminal
output can look temporarily distorted; `endwin()` restores it when the program
exits normally.

The same Debug build can be prepared from a terminal:

```sh
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build/debug --parallel
gdb --args ./build/debug/r-cal config/rcal.conf.example
```
