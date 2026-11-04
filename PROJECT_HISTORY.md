# Project History

This is a concise, chronological record of major milestones and decisions for
`r-cal`. It is not a substitute for Git history or a release changelog.

## 2026-11-04

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
