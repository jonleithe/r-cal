# AGENTS.md

This file is the shared working guide for people and coding agents contributing
to `r-cal`. Keep it committed so the same project context is available on every
computer.

## Project status

`r-cal` is a C and C++ terminal-calendar project. Its initial timeline header is
implemented in C++17, built with CMake, rendered with curses, and tested through
CTest. C remains an enabled project language for future components.

## Source of truth

- `README.md` describes the project and how to use it.
- `PROJECT_HISTORY.md` records important decisions and milestones.
- `AGENTS.md` records durable contribution and workflow instructions.
- The Git history is the authoritative record of code changes.

When the project structure or workflow changes, update the relevant documents
in the same commit as the change.

## Working conventions

1. Start by reading `README.md`, `PROJECT_HISTORY.md`, and this file, then check
   `git status` so existing local work is not overwritten.
2. Keep changes focused. Do not modify or discard unrelated uncommitted work.
3. Prefer portable paths and commands; never commit machine-specific absolute
   paths, credentials, editor state, build products, or local configuration.
4. Add setup, build, test, lint, and run commands here once those tools exist.
5. Run the relevant checks before committing and record any checks that could
   not be run.
6. Add a short entry to `PROJECT_HISTORY.md` for architectural decisions,
   workflow changes, releases, and other milestones. Routine fixes do not need
   an entry unless they affect future work.
7. Write commit messages in the imperative mood and keep each commit coherent.
8. Use `#pragma once` for C and C++ header files instead of macro include
   guards.

## Cross-computer workflow

Before starting work on another computer:

```sh
git pull --rebase
git status
```

Before leaving a computer, commit all work that should travel with the project
and push it to the shared remote. Keep unfinished, machine-specific experiments
out of commits, or place them on a clearly named work-in-progress branch.

## Commands

Configure, build, and test from the repository root:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Run the curses demo from the repository root with:

```sh
./build/rcal_header_demo config/rcal.conf.example
```

The current code requires CMake 3.16 or newer, a C++17 compiler, and curses
development headers and libraries. Build artifacts belong in `build/` and must
not be committed.

## Keeping this file useful

Use repository-wide instructions here. Put component-specific instructions in
an `AGENTS.md` inside that component's directory if the repository later grows
distinct subsystems. More specific instructions should supplement or override
this root guide only within their directory tree.
