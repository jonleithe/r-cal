# r-cal

A C and C++ terminal calendar project. The first component is a configurable
ncurses timeline header with grouped year, month, and ISO-week rows plus day of
year (DOY), date, day-name, and time-information rows.

## Build and test

Requirements: a C++17 compiler, CMake 3.16 or newer, and curses development
headers and libraries with wide-character support (`ncursesw` on Linux).
The build locates `ncursesw` through `pkg-config`.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Alternatively, use the project build wrapper:

```sh
./build.sh debug
./build.sh release
./build.sh all
```

Debug and Release outputs are kept separately in `build/debug` and
`build/release`. After a successful build, `build/r-cal` points to the selected
configuration; `all` selects the Release executable.

Run the interactive example from the repository root:

```sh
./build/r-cal config/rcal.conf.example
```

Use the navigation keys below and press `q`, `Q`, or Escape to exit. Copy
`config/rcal.conf.example` and edit its labels, localized names, range, layout,
and `color.*` settings to customize the header. `start_date` accepts either an
ISO date (`YYYY-MM-DD`) or `today`.

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

- Left/Right: select the previous or next day.
- Page Up/Page Down: move by one visible screen.
- Home or `n`: return to today.
- `c`: open the color selector.
- Insert: open the create-item chooser for a Note, Task, or Event.
- Up/Down: select a note below the calendar.
- Enter: edit the selected note.
- `w`, `t`, `o`, `a`, or `f`: toggle the selected date's work status.
- Space: clear the selected date's work status.
- `q`, `Q`, or Escape: quit.

The color selector lists separate foreground and background rows for every
configurable element. Use Up/Down to choose a row, Left/Right to choose among
the colors supported by the current terminal, and `b`, `u`, or `r` to toggle
bold, underline, or reverse on that element. Enter applies the edited palette
and writes it to the loaded configuration file. Escape closes the selector
without changing the active palette or the file.

Today and the selected date retain their normal foreground color, including
weekend and holiday colors. `color.current_day` and `color.selected_day` apply
their background and attributes as highlight overlays. Selection styling takes
precedence when today is selected; the current-day styling returns as soon as
selection moves away. Resizing the terminal recalculates the number of visible
days and keeps the selected date centered.

## Per-day information

Each date can have a `DayInfo` record without tying that data to a buffered
timeline index. Records survive timeline rebuilds and are added through
`setDayInfo(Date, DayInfo)`.

The five-character Info cell has fixed positions:

```text
[work status][holiday][note][task count][events]
```

- Work status: `W` workday, `T` travelling, `O` out-of-office working, `A`
  compensatory leave (avspasering), or `F` vacation (ferie).
- Holiday: `H` when the calendar date is a Norwegian public holiday.
- Note: `N` when the date has one or more non-empty user-entered notes.
- Task count: `1`–`9`, or `+` for ten or more tasks.
- Events: `E` when one or more events exist.

For example, `WHN3E` means a working public holiday with a note, three tasks,
and at least one event. `cell_width` must be at least 5. Full selected-day text,
currently the holiday name, is shown on the Detail row below the indicators.
The status bar at the bottom expands the selected date's active codes and
counts into readable descriptions. Its appearance is configured with
`color.status_bar` and can also be changed in the color selector.

Each date can contain multiple independent diary or list-style notes. They are
shown one per row below the calendar. Press Insert to open the create-item
chooser, select Note, and press Enter. Use Up/Down in the normal view to select
an existing note, then press Enter to edit it directly on its note row. New notes are edited on
the next row in the list, which scrolls when necessary to keep the editor visible.
The editor supports normal text entry, Left/Right, Home/End, Backspace, and
Delete. Enter saves the note; Escape cancels the edit. Saving an existing note
as empty removes it. Notes are UTF-8 text, including Norwegian characters.

The create-item chooser also reserves Task and Event entries. Their editors
are not implemented yet and are labelled accordingly in the menu.

## Norwegian holidays

Set `holiday_country=NO` to highlight Norway's named public holidays with
`color.holiday`. The calendar calculates Easter-relative holidays for every
year, along with New Year's Day, 1 May, 17 May, and the two Christmas days.
Set `holiday_country=none` to disable national-holiday highlighting. Ordinary
Sundays remain covered by the weekend style.

When a holiday is selected, the Info row displays its full Norwegian name, for
example `Helligdag: Kristi himmelfartsdag`. Configure or remove the prefix with
`label.holiday_info`.

Calendar text is rendered through wide-character curses, so Nordic characters
such as `Ø`, `Æ`, and `Å` (and their lowercase forms) are supported in labels,
month/day names, and holiday descriptions. The application initializes the
terminal locale before starting curses.

The implemented dates follow Norway's official
[Public Holiday Act](https://lovdata.no/dokument/NL/lov/1995-02-24-12/%C2%A72)
and the [Act relating to 1 and 17 May](https://lovdata.no/dokument/NL/lov/1947-04-26-1).

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
