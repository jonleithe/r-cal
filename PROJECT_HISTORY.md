# Project History

This is a concise, chronological record of major milestones and decisions for
`r-cal`. It is not a substitute for Git history or a release changelog.

## 2026-11-06

### In-app help added

- Added a scrollable `?` help window with navigation, item editing, work status,
  color-picker shortcuts, and Info-indicator explanations.
- Help adapts to terminal dimensions and closes without changing calendar data.


### Dedicated task editor added

- Tasks now use separate description, due-date, and due-time fields for creation
  and editing, so users can enter deadlines without text-prefix syntax.
- New tasks default to the selected date. Blank deadline fields allow an
  unscheduled task; invalid dates/times keep the form open with an error.
- Structured task edits preserve identity and completion, and feed the existing
  persistent deadline and overdue-rendering logic directly.


### Event ranges and dedicated editor added

- Replaced event text-prefix entry in the TUI with a form for description,
  start date/time, and end date/time.
- Events now appear on every date in their inclusive range. Each occurrence
  shares one stable record; edits and deletion refresh all daily views.
- SQLite schema version 4 adds end fields and a range index; existing events
  become one-day ranges on their start dates.
- Added range validation and coverage for migration, cross-buffer navigation,
  boundary dates, and editing/deleting from the middle of a range.


### Task deadlines and event starts added

- Added optional task due dates/times and event start dates independent of the
  filing date, with shared validation and editable schedule prefixes.
- Calculate overdue status for incomplete tasks using local wall-clock time;
  date-only deadlines expire after the date ends.
- SQLite schema version 3 preserves existing tasks without deadlines and
  initializes existing event start dates from their filing dates.
- Keep invalid edits open with an error; refresh overdue labels while idle.


### Separate item colors added

- Notes, tasks, and events now have independent configurable list styles and
  foreground/background entries in the color selector.
- Default item colors use white, yellow, and cyan for readability on dark terminals.


### Local tasks and events added

- Added SQLite schema version 2 with ordered task records, completion state,
  and date-bound event records with an optional local time.
- Extended the shared item editor and navigation so notes, tasks, and events
  can be created, selected, edited, and removed; `x` toggles a selected task.

## 2026-11-05

### SQLite day-metadata persistence implemented

- Added the `CalendarStore` boundary and SQLite implementation with schema
  versioning, transactional creation, stable note IDs, and temporary-database
  coverage.
- Work-status and confirmed note edits now persist immediately to the XDG data
  directory by default, with `data_file` available for an explicit database
  location.
- Kept calendar navigation and rendering independent of SQL; the framework
  loads timeline metadata through the store and retains an in-memory view.

### Local-first persistence direction documented

- Chose SQLite as the initial embedded persistence backend, isolated behind a
  `CalendarStore` interface rather than coupled to the TUI or calendar model.
- Documented the proposed schema, XDG data location, migrations, tests, and a
  path toward later export and multi-device synchronization.

### Navigable day notes added

- Replaced the note-presence flag with multiple independent UTF-8 note entries
  stored in each date's `DayInfo`.
- Notes render as navigable rows below the calendar; Insert opens a shared
  Note/Task/Event creation chooser, Up/Down selects a note, and Enter edits it.
- Note creation is active now; Task and Event entries reserve the interaction
  for their future editors.

### Keyboard work-status editing added

- Added lowercase `w`, `t`, `o`, `a`, and `f` shortcuts to toggle the selected
  date's work status, with Space available to clear it.
- Added vacation (ferie) as a first-class work status represented by `F`.

### Selected-day status bar added

- Added a bottom status bar that expands the selected date's compact Info codes
  into work, holiday, note, task, and event descriptions.
- Added `color.status_bar` and exposed it through the interactive color selector.

### Interactive color selector added

- Added a `c`-key modal editor for every configurable foreground, background,
  and text attribute exposed by the active terminal.
- Saving updates the loaded configuration file; cancelling leaves both the
  running palette and file unchanged.

### Persistent day-color highlights

- Current-day and selected-day highlights retain the ordinary foreground color
  while applying their configured background and attributes, so weekend and
  holiday colors remain recognizable.

## 2026-11-04

### Selection preserves day colors

- Selected dates retain their current-day, holiday, weekend, or row foreground
  color and attributes while using the configured selection background.

### Per-date information model added

- Added date-keyed `DayInfo` records for work status, notes, tasks, and events;
  these remain stable when the buffered timeline is rebuilt.
- The five-character Info cell now renders fixed status, holiday, note, task,
  and event positions.
- Moved full selected-holiday text to a separate Detail row so daily indicators
  remain visible.

### Selection highlight precedence corrected

- Selected-day styling now takes precedence over current-day styling, keeping
  navigation feedback visible when the selected date reaches today.

### Day-of-year row added

- Added a DOY row between Week and Date using the timeline's existing ordinal
  day values.
- Added configurable `label.day_of_year` and `color.day_of_year` settings.

### Build wrapper added

- Added `build.sh` modes for Debug, Release, or both configurations.
- Debug and Release artifacts use separate build directories.
- The wrapper maintains `build/r-cal` as a link to the selected configuration,
  preventing stale binaries from an older build layout from being launched.

### Nordic terminal text support added

- Switched rendering and linking to wide-character curses.
- Text clipping and centering now use terminal column widths instead of UTF-8
  byte counts, allowing Nordic characters without corrupting alignment.
- Updated the example configuration with Norwegian month and day names.

### Holiday explanations added

- The Info row now shows the selected holiday's full Norwegian name.
- Added a configurable `label.holiday_info` prefix and enabled the process
  locale so Norwegian characters render correctly in curses terminals.

### Norwegian public holidays added

- Added automatic calculation and highlighting of Norway's fixed and
  Easter-relative public holidays for every year in the timeline.
- Added `holiday_country` and `color.holiday` configuration settings, with
  `none` available to disable the holiday calendar.

### Buffered timeline navigation added

- Added keyboard navigation by day and by visible page, with shortcuts to
  return to today and quit.
- The selected date starts centered in the viewport, with a 90-day buffer on
  each side that rebuilds when fewer than 30 hidden days remain.
- Added distinct current-day and selected-day styles plus resize-aware viewport
  calculation.

### Step-by-step debugging configured

- Added VS Code tasks for Debug configuration, building, and tests.
- Added a GDB launch profile that runs the curses application in the integrated
  terminal with the example configuration.
- Enabled `compile_commands.json` generation for accurate C++ IntelliSense.

### Current day highlighting added

- The curses renderer hides the terminal cursor and highlights the current
  local date across its date, day-name, and time-information cells.
- The highlight is configurable through `color.current_day`.

### Application entry point promoted

- Moved the executable entry point from the examples directory to the project
  root as `main.cpp` and renamed the built executable to `r-cal`.

### Timeline rendering constrained to terminal width

- Curses output is clipped at the active window's horizontal boundary instead
  of wrapping into adjacent rows.
- Off-screen timeline cells are skipped, while partially visible year, month,
  and week headings are centered within the visible viewport.

### Header convention selected

- C and C++ headers use `#pragma once` instead of macro include guards.

### Configurable curses timeline header introduced

- Reworked the initial header stub into a C++17 timeline model and curses
  renderer with grouped year, month, and ISO-week rows.
- Added independently configurable styles for labels, years, months, week
  numbers, dates, day names, weekends, time information, and grid separators.
- Added a key/value configuration format with localized month and day names,
  an example program, CMake build, and calendar-boundary tests.

### C and C++ selected

- The project will be implemented in C and C++.
- The compiler, C and C++ language standards, build system, and test tooling
  remain to be selected.

### Repository initialized

- Created the repository with an MIT license, a starter `.gitignore`, and a
  minimal `README.md`.
- Initial commit: `a01cc1b` (`Initial commit`).

### Shared project context added

- Added `AGENTS.md` to keep working conventions consistent across computers and
  contributors.
- Added this history file so important decisions and milestones remain easy to
  find without reconstructing them from individual commits.
- The project is still in its bootstrap phase; its scope, architecture, and
  development commands have not yet been documented.

## Entry format

Add new entries in reverse chronological order using this structure:

```md
## YYYY-MM-DD

### Short milestone or decision title

- What changed.
- Why it changed, when that context will help future contributors.
- Relevant issue, pull request, release, or commit reference, if available.
```

Record durable context rather than a list of every file changed. Correct older
entries only when they are factually wrong; otherwise add a new dated entry so
the evolution of the project remains visible.
