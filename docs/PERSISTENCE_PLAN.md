# Persistence plan

## Direction

`r-cal` uses local-first persistence backed by SQLite. SQLite is an
embedded library, so users do not need to install, configure, or operate a
MariaDB/PostgreSQL server. The optional `sqlite3` command-line program is useful
for inspection but is not required at runtime.

The TUI and `RCalHeaderFramework` must not call SQLite directly. Persistence
belongs behind a storage interface so another backend or synchronization layer
can be added later without rewriting calendar navigation and rendering.

## Source layout

```text
include/
  CalendarStore.h
  CalendarTypes.h
  SQLiteCalendarStore.h

src/
  SQLiteCalendarStore.cpp

tests/
  SQLiteCalendarStoreTest.cpp
```

`CalendarStore` defines operations and persistent record types. `CalendarTypes`
holds the shared date, work-status, note, and day-information types.
`SQLiteCalendarStore` owns the SQLite connection, prepared statements,
transactions, schema creation, and migrations.

## Implemented responsibilities

The first persistent data is user-entered metadata:

- Work status for a date: unspecified, workday, travelling, out-of-office
  working, compensatory leave, or vacation.
- Multiple independently editable diary/list notes for a date.
- Ordered tasks with a completion state.
- Date-bound events with an optional local `HH:MM` time.
- Stable identity and ordering for each record type.

Calendar facts such as ISO weeks and Norwegian holidays remain calculated by
the calendar model. Imported tasks and events will later retain their external
source and iCalendar UID so refreshes update records instead of duplicating
them.

## Interface

The exact signatures can evolve during implementation, but the boundary should
cover operations rather than expose SQL:

```cpp
class CalendarStore {
public:
    virtual ~CalendarStore() = default;

    virtual DayInfo loadDay(const Date& date) = 0;
    virtual void saveWorkStatus(const Date& date, WorkStatus status) = 0;
    virtual Note addNote(const Date& date, std::string text,
                         std::size_t position) = 0;
    virtual void updateNote(const NoteId& id, std::string text) = 0;
    virtual void deleteNote(const NoteId& id) = 0;
    virtual Task addTask(const Date& date, std::string text,
                         std::size_t position) = 0;
    virtual void setTaskCompleted(const TaskId& id, bool completed) = 0;
    virtual Event addEvent(const Date& date, std::string time, std::string text,
                           std::size_t position) = 0;
};
```

Accepted edits should be written immediately. Escape continues to cancel an
edit without changing the in-memory model or database.

`moveNote` remains deferred until the TUI exposes note reordering.

## Schema version 2

```sql
CREATE TABLE day_info (
    date TEXT PRIMARY KEY,
    work_status TEXT NOT NULL
);

CREATE TABLE notes (
    id TEXT PRIMARY KEY,
    date TEXT NOT NULL,
    position INTEGER NOT NULL,
    text TEXT NOT NULL,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);

CREATE INDEX notes_by_date_position ON notes(date, position);

CREATE TABLE tasks (
    id TEXT PRIMARY KEY,
    date TEXT NOT NULL,
    position INTEGER NOT NULL,
    text TEXT NOT NULL,
    completed INTEGER NOT NULL DEFAULT 0,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);

CREATE TABLE events (
    id TEXT PRIMARY KEY,
    date TEXT NOT NULL,
    position INTEGER NOT NULL,
    time TEXT NOT NULL DEFAULT '',
    text TEXT NOT NULL,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);
```

Dates use ISO `YYYY-MM-DD` text. Notes should receive stable IDs rather than be
identified by their current row number. UUIDs are a likely choice because they
also prepare the model for future multi-device synchronization.

Schema changes use transactions and SQLite's `PRAGMA user_version`. Every
migration must be forward-only, tested, and safe to run once when opening an
older database.

## Database location and configuration

The database does not belong in the Git repository or build directory. The
default location should follow the XDG base-directory convention:

```text
$XDG_DATA_HOME/r-cal/r-cal.db
```

When `XDG_DATA_HOME` is unset, the usual location is:

```text
~/.local/share/r-cal/r-cal.db
```

A `data_file` configuration setting allows an explicit alternative path.
Parent directories should be created safely on first use.

## Testing

Storage tests use a temporary database and currently cover:

- Initial schema creation and reopening an existing database.
- Work-status save/clear and persistence after reopening.
- Multiple notes on one date, including stable ordering.
- Note update and deletion without affecting other notes or day metadata.
- Task completion and timed-event persistence after reopening.
- Migration from schema version 1.
- UTF-8 note text.

Future coverage should add transaction rollback on failure and every supported
schema migration.

An in-memory `CalendarStore` implementation or test double can keep framework
and TUI tests independent of SQLite.

## Synchronization and online services

Online synchronization is deliberately deferred until the local data model is
stable. A live SQLite file should not be synchronized through Git, Dropbox, or
similar file replication while applications may write it concurrently.

Possible later steps are:

1. JSON export/import for backup and manual transfer.
2. CalDAV/iCalendar integration for externally managed tasks and events.
3. An append-only change log with stable IDs and deletion tombstones for
   offline multi-device synchronization.
4. A dedicated service implementing the same storage semantics if automatic
   synchronization becomes a priority.

MariaDB or another server database would only be considered as part of a
central service. It is not required for the first local persistence backend.

## Implementation sequence

1. Add backup/export before designing network synchronization.
2. Add note reordering and the corresponding transactional store operation.
3. Add event duration, recurrence, and imported-calendar support.

## Open questions

- Whether note IDs should change from the current SQLite-generated random IDs
  to UUIDv7 before synchronization work begins.
- Whether timestamps are required in the first schema or added when note
  history and synchronization are implemented.
- How imported task/event sources and refresh policies will be configured.
