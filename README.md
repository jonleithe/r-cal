# r-cal

A C and C++ terminal calendar project. The first component is a configurable
ncurses timeline header with grouped year, month, and ISO-week rows plus day of
year (DOY), date, day-name, and time-information rows.

## Build and test

Requirements: a C++17 compiler, CMake 3.16 or newer, SQLite development
headers and library, and curses development headers and libraries with
wide-character support (`ncursesw` on Linux). The build locates `ncursesw`
through `pkg-config`.

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
- `?`: open help. Use Up/Down or Page Up/Page Down to scroll; Escape or `?`
  closes help and returns to the calendar.
- `c`: open the color selector.
- Insert: open the create-item chooser for a Note, Task, or Event.
- Up/Down: select a note, task, or event below the calendar.
- Enter: edit the selected item.
- Delete: remove the selected note, task, or event immediately. A multi-day
  event is removed from every covered date.
- `x`: toggle completion of the selected task.
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

The item list has separate `color.notes`, `color.tasks`, and `color.events`
styles, available as Notes, Tasks, and Events in the color selector. Defaults
are white, yellow, and cyan respectively, on the terminal background. The
**Selected item** picker entry (`color.selected_item`) controls the full
foreground, background, and attributes of the highlighted note, task, or event.
It defaults to bold black on cyan. `color.time_info`
continues to control the compact calendar Info row.

Each date can contain multiple independent diary/list-style notes, tasks, and
events. Press Insert to open the create-item chooser, then use Up/Down in the
normal view to select an existing item and Enter to edit it. Saving an existing
item as empty removes it. Delete removes the selected item and selects the
next item, or the previous one when deleting the last item. Within an editor,
Delete removes the character at the cursor. Press `x` to complete or reopen the selected task.

Tasks open a form with separate **Description**, **Due date**, and **Due time**
fields. Use Tab, Shift+Tab, or Up/Down to change fields, Enter to save, and
Escape to cancel. Dates use `YYYY-MM-DD` and optional times use local 24-hour
`HH:MM`. New tasks default to the selected date; clear both deadline fields
for a task without a deadline. A time requires a date. For example, enter
`Send report` as the description, `2026-11-10` as the due date, and `16:00` as
the due time. Times typed in Description remain ordinary text.

Incomplete tasks show a leading `!` and `OVERDUE` after their deadline in local
time. The **Overdue task** color-picker entry (`color.overdue_task`) defaults
to bold red. Selected overdue tasks keep the **Selected item** colors and the
`!` marker. Completed tasks use normal task styling. A date-only
deadline remains valid through that entire day. Completed tasks are never
marked overdue. Tasks without a prefix have no deadline.

Events open a form with separate **Description**, **Start date**, **Start time**,
**End date**, and **End time** fields. Use Tab, Shift+Tab, or Up/Down to move
between fields, Enter to save, and Escape to cancel. Dates use `YYYY-MM-DD`;
times use local 24-hour `HH:MM` and may be left blank. New events default to the
selected date for both endpoints. A blank end date defaults to the start date.

Events appear in the list and show an `E` indicator on every date from the
start date through the end date, including both endpoints, even when the end
time is midnight. With no times, the range is all-day; a missing start time is
interpreted as the beginning of its day and a missing end time as the end of
its day for validation. An end before the start is rejected without closing
the editor.

For example, an event starting on `2026-11-06` at `09:00` and ending on
`2026-11-08` at `17:00` appears on 6, 7, and 8 November. Each occurrence refers
to the same event: editing or deleting it from any of those dates updates the
whole range. Changing its dates removes it from the old dates and shows it
on the new ones. Creating an event outside the currently selected date moves
the selection to its start date.

A task's due date is independent of the date where it is filed. Reopening a
task with Enter loads its existing deadline into the form; clear both deadline
fields to remove it. Editing preserves task identity and completion state.
Clearing a note's text or a task/event's Description deletes it. Invalid schedules keep the editor open with an error.
Overdue labels refresh while idle. Events load in start-date/time order, with
timed events before all-day events on the same start date. Item text supports
UTF-8, including Norwegian characters.

## Persistence

User-entered work statuses, notes, tasks, and events are stored immediately in a local SQLite
database. By default this is `$XDG_DATA_HOME/r-cal/r-cal.db`, or
`~/.local/share/r-cal/r-cal.db` when `XDG_DATA_HOME` is unset. Set `data_file`
in the configuration to use an explicit database path instead. The SQLite
backend is isolated behind a storage interface; its schema, migration strategy,
and future synchronization options are documented in
[docs/PERSISTENCE_PLAN.md](docs/PERSISTENCE_PLAN.md).

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
