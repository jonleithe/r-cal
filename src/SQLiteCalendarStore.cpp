#include "SQLiteCalendarStore.h"
#include "Scheduling.h"

#include <sqlite3.h>

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace rcal {
namespace {

class Statement {
  public:
    Statement(sqlite3 *database, const char *sql) {
        if (sqlite3_prepare_v2(database, sql, -1, &statement_, nullptr) !=
            SQLITE_OK)
            throw std::runtime_error("SQLite prepare failed: " +
                                     std::string(sqlite3_errmsg(database)));
    }

    ~Statement() { sqlite3_finalize(statement_); }

    sqlite3_stmt *get() const noexcept { return statement_; }

  private:
    sqlite3_stmt *statement_ = nullptr;
};

void execute(sqlite3 *database, const char *sql) {
    char *message = nullptr;
    if (sqlite3_exec(database, sql, nullptr, nullptr, &message) != SQLITE_OK) {
        const std::string error = message ? message : sqlite3_errmsg(database);
        sqlite3_free(message);
        throw std::runtime_error("SQLite error: " + error);
    }
}
// ——— End of execute() ————————————————————————————————————————————————————————



void bindText(sqlite3_stmt *statement, int index, const std::string &value) {
    if (sqlite3_bind_text(statement, index, value.c_str(), -1,
                          SQLITE_TRANSIENT) != SQLITE_OK)
        throw std::runtime_error("SQLite bind failed");
}
// ——— End of bindText() ———————————————————————————————————————————————————————



void bindPosition(sqlite3_stmt *statement, int index, std::size_t position) {
    if (sqlite3_bind_int64(statement, index,
                           static_cast<sqlite3_int64>(position)) != SQLITE_OK)
        throw std::runtime_error("SQLite bind failed");
}
// ——— End of bindPosition() ———————————————————————————————————————————————————



void expectDone(sqlite3 *database, sqlite3_stmt *statement) {
    if (sqlite3_step(statement) != SQLITE_DONE)
        throw std::runtime_error("SQLite step failed: " +
                                 std::string(sqlite3_errmsg(database)));
}
// ——— End of expectDone() —————————————————————————————————————————————————————



std::string dateString(const Date &date) {
    const bool leapYear =
        date.year % 4 == 0 && (date.year % 100 != 0 || date.year % 400 == 0);
    static const unsigned monthLengths[] = {31, 28, 31, 30, 31, 30,
                                            31, 31, 30, 31, 30, 31};
    if (date.year < 1 || date.year > 9999 || date.month < 1 ||
        date.month > 12 || date.day < 1 ||
        date.day > monthLengths[date.month - 1] +
                       (date.month == 2 && leapYear ? 1U : 0U)) {
        throw std::invalid_argument("invalid calendar date");
    }
    std::ostringstream value;
    value << std::setfill('0') << std::setw(4) << date.year << '-'
          << std::setw(2) << date.month << '-' << std::setw(2) << date.day;
    return value.str();
}
// ——— End of dateString() —————————————————————————————————————————————————————



const char *toDatabaseValue(WorkStatus status) {
    switch (status) {
    case WorkStatus::workday:
        return "workday";
    case WorkStatus::travelling:
        return "travelling";
    case WorkStatus::outOfOfficeWorking:
        return "out_of_office_working";
    case WorkStatus::compensatoryLeave:
        return "compensatory_leave";
    case WorkStatus::vacation:
        return "vacation";
    case WorkStatus::unspecified:
        return "unspecified";
    }
    throw std::runtime_error("unknown work status");
}
// ——— End of toDatabaseValue() ————————————————————————————————————————————————



WorkStatus fromDatabaseValue(const char *value) {
    const std::string status = value ? value : "";
    if (status == "workday")
        return WorkStatus::workday;
    if (status == "travelling")
        return WorkStatus::travelling;
    if (status == "out_of_office_working")
        return WorkStatus::outOfOfficeWorking;
    if (status == "compensatory_leave")
        return WorkStatus::compensatoryLeave;
    if (status == "vacation")
        return WorkStatus::vacation;
    throw std::runtime_error("invalid work status in SQLite database: " +
                             status);
}
// ——— End of fromDatabaseValue() ——————————————————————————————————————————————



int schemaVersion(sqlite3 *database) {
    Statement statement(database, "PRAGMA user_version");
    if (sqlite3_step(statement.get()) != SQLITE_ROW)
        throw std::runtime_error("could not read SQLite schema version");
    return sqlite3_column_int(statement.get(), 0);
}
// ——— End of schemaVersion() ——————————————————————————————————————————————————



std::string newRecordId(sqlite3 *database) {
    Statement statement(database, "SELECT lower(hex(randomblob(16)))");
    if (sqlite3_step(statement.get()) != SQLITE_ROW)
        throw std::runtime_error("could not generate note identifier");
    const auto *text = sqlite3_column_text(statement.get(), 0);
    return text ? reinterpret_cast<const char *>(text) : "";
}
// ——— End of newRecordId() ————————————————————————————————————————————————————



} // namespace

SQLiteCalendarStore::SQLiteCalendarStore(
    const std::filesystem::path &databasePath) {
    if (databasePath.empty())
        throw std::invalid_argument("SQLite database path must not be empty");
    const std::filesystem::path parent = databasePath.parent_path();
    if (!parent.empty())
        std::filesystem::create_directories(parent);

    if (sqlite3_open_v2(databasePath.string().c_str(), &database_,
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
                            SQLITE_OPEN_FULLMUTEX,
                        nullptr) != SQLITE_OK) {
        const std::string error =
            database_ ? sqlite3_errmsg(database_) : "could not open database";
        if (database_)
            sqlite3_close(database_);
        database_ = nullptr;
        throw std::runtime_error("could not open SQLite database '" +
                                 databasePath.string() + "': " + error);
    }

    try {
        execute(database_, "PRAGMA foreign_keys = ON");
        sqlite3_busy_timeout(database_, 3000);
        execute(database_, "BEGIN IMMEDIATE");
        const int version = schemaVersion(database_);
        if (version == 0) {
            execute(database_, "CREATE TABLE day_info ("
                               "date TEXT PRIMARY KEY, "
                               "work_status TEXT NOT NULL"
                               ")");
            execute(database_, "CREATE TABLE notes ("
                               "id TEXT PRIMARY KEY, "
                               "date TEXT NOT NULL, "
                               "position INTEGER NOT NULL, "
                               "text TEXT NOT NULL, "
                               "created_at TEXT NOT NULL, "
                               "updated_at TEXT NOT NULL"
                               ")");
            execute(
                database_,
                "CREATE INDEX notes_by_date_position ON notes(date, position)");
            execute(database_, "CREATE TABLE tasks ("
                               "id TEXT PRIMARY KEY, "
                               "date TEXT NOT NULL, "
                               "position INTEGER NOT NULL, "
                               "text TEXT NOT NULL, "
                               "completed INTEGER NOT NULL DEFAULT 0, "
                               "created_at TEXT NOT NULL, "
                               "updated_at TEXT NOT NULL"
                               ")");
            execute(
                database_,
                "CREATE INDEX tasks_by_date_position ON tasks(date, position)");
            execute(database_, "CREATE TABLE events ("
                               "id TEXT PRIMARY KEY, "
                               "date TEXT NOT NULL, "
                               "position INTEGER NOT NULL, "
                               "time TEXT NOT NULL DEFAULT '', "
                               "text TEXT NOT NULL, "
                               "created_at TEXT NOT NULL, "
                               "updated_at TEXT NOT NULL"
                               ")");
            execute(database_, "CREATE INDEX events_by_date_time_position ON "
                               "events(date, time, position)");
            execute(database_, "PRAGMA user_version = 2");
        } else if (version == 1) {
            execute(database_,
                    "CREATE TABLE tasks ("
                    "id TEXT PRIMARY KEY, date TEXT NOT NULL, position INTEGER "
                    "NOT NULL, "
                    "text TEXT NOT NULL, completed INTEGER NOT NULL DEFAULT 0, "
                    "created_at TEXT NOT NULL, updated_at TEXT NOT NULL"
                    ")");
            execute(
                database_,
                "CREATE INDEX tasks_by_date_position ON tasks(date, position)");
            execute(database_,
                    "CREATE TABLE events ("
                    "id TEXT PRIMARY KEY, date TEXT NOT NULL, position INTEGER "
                    "NOT NULL, "
                    "time TEXT NOT NULL DEFAULT '', text TEXT NOT NULL, "
                    "created_at TEXT NOT NULL, updated_at TEXT NOT NULL"
                    ")");
            execute(database_, "CREATE INDEX events_by_date_time_position ON "
                               "events(date, time, position)");
            execute(database_, "PRAGMA user_version = 2");
        } else if (version != 2 && version != 3 && version != 4) {
            throw std::runtime_error("unsupported SQLite schema version " +
                                     std::to_string(version));
        }
        if (version < 3) {
            execute(database_, "ALTER TABLE tasks ADD COLUMN due_date TEXT NOT NULL DEFAULT ''");
            execute(database_, "ALTER TABLE tasks ADD COLUMN due_time TEXT NOT NULL DEFAULT ''");
            execute(database_, "ALTER TABLE events ADD COLUMN start_date TEXT NOT NULL DEFAULT ''");
            execute(database_, "UPDATE events SET start_date = date");
            execute(database_, "PRAGMA user_version = 3");
        }
        if (version < 4) {
            execute(database_, "ALTER TABLE events ADD COLUMN end_date TEXT NOT NULL DEFAULT ''");
            execute(database_, "ALTER TABLE events ADD COLUMN end_time TEXT NOT NULL DEFAULT ''");
            execute(database_, "UPDATE events SET end_date = start_date");
            execute(database_, "CREATE INDEX events_by_range ON events(start_date, end_date)");
            execute(database_, "PRAGMA user_version = 4");
        }
        execute(database_, "COMMIT");
    } catch (...) {
        sqlite3_exec(database_, "ROLLBACK", nullptr, nullptr, nullptr);
        sqlite3_close(database_);
        database_ = nullptr;
        throw;
    }
}
// ——— End of SQLiteCalendarStore() ————————————————————————————————————————————



SQLiteCalendarStore::~SQLiteCalendarStore() {
    if (database_)
        sqlite3_close(database_);
}
// ——— End of ~SQLiteCalendarStore() ———————————————————————————————————————————



DayInfo SQLiteCalendarStore::loadDay(const Date &date) {
    DayInfo result;
    const std::string day = dateString(date);
    {
        Statement statement(database_,
                            "SELECT work_status FROM day_info WHERE date = ?");
        bindText(statement.get(), 1, day);
        const int outcome = sqlite3_step(statement.get());
        if (outcome == SQLITE_ROW)
            result.workStatus =
                fromDatabaseValue(reinterpret_cast<const char *>(
                    sqlite3_column_text(statement.get(), 0)));
        else if (outcome != SQLITE_DONE)
            throw std::runtime_error("SQLite step failed: " +
                                     std::string(sqlite3_errmsg(database_)));
    }
    {
        Statement statement(
            database_,
            "SELECT id, text FROM notes WHERE date = ? ORDER BY position, id");
        bindText(statement.get(), 1, day);
        int outcome = SQLITE_OK;
        while ((outcome = sqlite3_step(statement.get())) == SQLITE_ROW) {
            const auto *id = sqlite3_column_text(statement.get(), 0);
            const auto *text = sqlite3_column_text(statement.get(), 1);
            result.notes.push_back(
                {id ? reinterpret_cast<const char *>(id) : "",
                 text ? reinterpret_cast<const char *>(text) : ""});
        }
        if (outcome != SQLITE_DONE)
            throw std::runtime_error("SQLite step failed: " +
                                     std::string(sqlite3_errmsg(database_)));
    }
    {
        Statement statement(database_, "SELECT id, text, completed, due_date, due_time FROM tasks "
                                       "WHERE date = ? ORDER BY position, id");
        bindText(statement.get(), 1, day);
        int outcome = SQLITE_OK;
        while ((outcome = sqlite3_step(statement.get())) == SQLITE_ROW) {
            const auto *id = sqlite3_column_text(statement.get(), 0);
            const auto *text = sqlite3_column_text(statement.get(), 1);
            result.tasks.push_back(
                {id ? reinterpret_cast<const char *>(id) : "",
                 text ? reinterpret_cast<const char *>(text) : "",
                 sqlite3_column_int(statement.get(), 2) != 0,
                 reinterpret_cast<const char *>(sqlite3_column_text(statement.get(), 3)),
                 reinterpret_cast<const char *>(sqlite3_column_text(statement.get(), 4))});
        }
        if (outcome != SQLITE_DONE)
            throw std::runtime_error("SQLite step failed: " +
                                     std::string(sqlite3_errmsg(database_)));
    }
    {
        Statement statement(database_,
                            "SELECT id, time, text, start_date, end_date, end_time FROM events WHERE start_date <= ? AND end_date >= ? "
                            "ORDER BY start_date, CASE WHEN time = '' THEN 1 ELSE 0 END, "
                            "time, position, id");
        bindText(statement.get(), 1, day);
        bindText(statement.get(), 2, day);
        int outcome = SQLITE_OK;
        while ((outcome = sqlite3_step(statement.get())) == SQLITE_ROW) {
            const auto *id = sqlite3_column_text(statement.get(), 0);
            const auto *time = sqlite3_column_text(statement.get(), 1);
            const auto *text = sqlite3_column_text(statement.get(), 2);
            result.events.push_back(
                {id ? reinterpret_cast<const char *>(id) : "",
                 time ? reinterpret_cast<const char *>(time) : "",
                 text ? reinterpret_cast<const char *>(text) : "",
                 reinterpret_cast<const char *>(sqlite3_column_text(statement.get(), 3)),
                 reinterpret_cast<const char *>(sqlite3_column_text(statement.get(), 4)),
                 reinterpret_cast<const char *>(sqlite3_column_text(statement.get(), 5))});
        }
        if (outcome != SQLITE_DONE)
            throw std::runtime_error("SQLite step failed: " +
                                     std::string(sqlite3_errmsg(database_)));
    }
    return result;
}
// ——— End of loadDay() ————————————————————————————————————————————————————————



void SQLiteCalendarStore::saveWorkStatus(const Date &date, WorkStatus status) {
    const std::string day = dateString(date);
    if (status == WorkStatus::unspecified) {
        Statement statement(database_, "DELETE FROM day_info WHERE date = ?");
        bindText(statement.get(), 1, day);
        expectDone(database_, statement.get());
        return;
    }
    Statement statement(
        database_,
        "INSERT INTO day_info(date, work_status) VALUES (?, ?) "
        "ON CONFLICT(date) DO UPDATE SET work_status = excluded.work_status");
    bindText(statement.get(), 1, day);
    bindText(statement.get(), 2, toDatabaseValue(status));
    expectDone(database_, statement.get());
}
// ——— End of saveWorkStatus() —————————————————————————————————————————————————



Note SQLiteCalendarStore::addNote(const Date &date, std::string text,
                                  std::size_t position) {
    if (text.empty())
        throw std::invalid_argument("note text must not be empty");
    const std::string day = dateString(date);
    const std::string id = newRecordId(database_);
    try {
        execute(database_, "BEGIN IMMEDIATE");
        Statement shift(database_, "UPDATE notes SET position = position + 1 "
                                   "WHERE date = ? AND position >= ?");
        bindText(shift.get(), 1, day);
        bindPosition(shift.get(), 2, position);
        expectDone(database_, shift.get());
        Statement insert(
            database_,
            "INSERT INTO notes(id, date, position, text, created_at, "
            "updated_at) "
            "VALUES (?, ?, ?, ?, strftime('%Y-%m-%dT%H:%M:%fZ', 'now'), "
            "strftime('%Y-%m-%dT%H:%M:%fZ', 'now'))");
        bindText(insert.get(), 1, id);
        bindText(insert.get(), 2, day);
        bindPosition(insert.get(), 3, position);
        bindText(insert.get(), 4, text);
        expectDone(database_, insert.get());
        execute(database_, "COMMIT");
    } catch (...) {
        sqlite3_exec(database_, "ROLLBACK", nullptr, nullptr, nullptr);
        throw;
    }
    return {id, std::move(text)};
}
// ——— End of addNote() ————————————————————————————————————————————————————————



void SQLiteCalendarStore::updateNote(const NoteId &id, std::string text) {
    if (text.empty())
        throw std::invalid_argument("note text must not be empty");
    Statement statement(database_, "UPDATE notes SET text = ?, updated_at = "
                                   "strftime('%Y-%m-%dT%H:%M:%fZ', 'now') "
                                   "WHERE id = ?");
    bindText(statement.get(), 1, text);
    bindText(statement.get(), 2, id);
    expectDone(database_, statement.get());
    if (sqlite3_changes(database_) != 1)
        throw std::runtime_error("note does not exist");
}
// ——— End of updateNote() —————————————————————————————————————————————————————



void SQLiteCalendarStore::deleteNote(const NoteId &id) {
    Statement statement(database_, "DELETE FROM notes WHERE id = ?");
    bindText(statement.get(), 1, id);
    expectDone(database_, statement.get());
    if (sqlite3_changes(database_) != 1)
        throw std::runtime_error("note does not exist");
}
// ——— End of deleteNote() —————————————————————————————————————————————————————



Task SQLiteCalendarStore::addTask(const Date &date, std::string text,
                                  std::size_t position, std::string dueDate, std::string dueTime) {
    if (text.empty())
        throw std::invalid_argument("task text must not be empty");
    validateSchedule(dueDate, dueTime);
    if (dueDate.empty() && !dueTime.empty())
        throw std::invalid_argument("task due time requires a due date");
    const std::string id = newRecordId(database_);
    Statement statement(
        database_,
        "INSERT INTO tasks(id, date, position, text, due_date, due_time, completed, created_at, "
        "updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, 0, strftime('%Y-%m-%dT%H:%M:%fZ', 'now'), "
        "strftime('%Y-%m-%dT%H:%M:%fZ', 'now'))");
    bindText(statement.get(), 1, id);
    bindText(statement.get(), 2, dateString(date));
    bindPosition(statement.get(), 3, position);
    bindText(statement.get(), 4, text);
    bindText(statement.get(), 5, dueDate);
    bindText(statement.get(), 6, dueTime);
    expectDone(database_, statement.get());
    return {id, std::move(text), false, std::move(dueDate), std::move(dueTime)};
}
// ——— End of addTask() ————————————————————————————————————————————————————————



void SQLiteCalendarStore::updateTask(const TaskId &id, std::string text,
                                      std::string dueDate, std::string dueTime) {
    if (text.empty())
        throw std::invalid_argument("task text must not be empty");
    validateSchedule(dueDate, dueTime);
    if (dueDate.empty() && !dueTime.empty())
        throw std::invalid_argument("task due time requires a due date");
    Statement statement(
        database_,
        "UPDATE tasks SET text = ?, due_date = ?, due_time = ?, "
        "updated_at = strftime('%Y-%m-%dT%H:%M:%fZ', 'now') WHERE id = ?");
    bindText(statement.get(), 1, text);
    bindText(statement.get(), 2, dueDate);
    bindText(statement.get(), 3, dueTime);
    bindText(statement.get(), 4, id);
    expectDone(database_, statement.get());
    if (sqlite3_changes(database_) != 1)
        throw std::runtime_error("task does not exist");
}
// ——— End of updateTask() —————————————————————————————————————————————————————



void SQLiteCalendarStore::deleteTask(const TaskId &id) {
    Statement statement(database_, "DELETE FROM tasks WHERE id = ?");
    bindText(statement.get(), 1, id);
    expectDone(database_, statement.get());
    if (sqlite3_changes(database_) != 1)
        throw std::runtime_error("task does not exist");
}
// ——— End of deleteTask() —————————————————————————————————————————————————————



void SQLiteCalendarStore::setTaskCompleted(const TaskId &id, bool completed) {
    Statement statement(
        database_,
        "UPDATE tasks SET completed = ?, "
        "updated_at = strftime('%Y-%m-%dT%H:%M:%fZ', 'now') WHERE id = ?");
    if (sqlite3_bind_int(statement.get(), 1, completed ? 1 : 0) != SQLITE_OK)
        throw std::runtime_error("SQLite bind failed");
    bindText(statement.get(), 2, id);
    expectDone(database_, statement.get());
    if (sqlite3_changes(database_) != 1)
        throw std::runtime_error("task does not exist");
}
// ——— End of setTaskCompleted() ——————————————————————————————————————————————



Event SQLiteCalendarStore::addEvent(const Date &date, std::string time,
                                    std::string text, std::size_t position, std::string startDate,
                                    std::string endDate, std::string endTime) {
    if (text.empty()) throw std::invalid_argument("event text must not be empty");
    Event event{newRecordId(database_), time, text, startDate, endDate, endTime};
    normalizeEvent(event, dateString(date));
    Statement statement(database_,
        "INSERT INTO events(id, date, position, time, text, start_date, end_date, end_time, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, strftime('%Y-%m-%dT%H:%M:%fZ', 'now'), "
        "strftime('%Y-%m-%dT%H:%M:%fZ', 'now'))");
    bindText(statement.get(), 1, event.id);
    bindText(statement.get(), 2, dateString(date));
    bindPosition(statement.get(), 3, position);
    bindText(statement.get(), 4, event.time);
    bindText(statement.get(), 5, event.text);
    bindText(statement.get(), 6, event.startDate);
    bindText(statement.get(), 7, event.endDate);
    bindText(statement.get(), 8, event.endTime);
    expectDone(database_, statement.get());
    return event;
}

void SQLiteCalendarStore::updateEvent(const EventId &id, std::string time,
                                      std::string text, std::string startDate,
                                      std::string endDate, std::string endTime) {
    if (text.empty()) throw std::invalid_argument("event text must not be empty");
    std::string defaultDate;
    {
        Statement lookup(database_, "SELECT start_date FROM events WHERE id = ?");
        bindText(lookup.get(), 1, id);
        if (sqlite3_step(lookup.get()) != SQLITE_ROW) throw std::runtime_error("event does not exist");
        defaultDate = reinterpret_cast<const char*>(sqlite3_column_text(lookup.get(), 0));
    }
    Event event{id, time, text, startDate, endDate, endTime};
    normalizeEvent(event, defaultDate);
    Statement statement(database_,
        "UPDATE events SET time = ?, text = ?, start_date = ?, end_date = ?, end_time = ?, "
        "updated_at = strftime('%Y-%m-%dT%H:%M:%fZ', 'now') WHERE id = ?");
    bindText(statement.get(), 1, event.time);
    bindText(statement.get(), 2, event.text);
    bindText(statement.get(), 3, event.startDate);
    bindText(statement.get(), 4, event.endDate);
    bindText(statement.get(), 5, event.endTime);
    bindText(statement.get(), 6, id);
    expectDone(database_, statement.get());
    if (sqlite3_changes(database_) != 1) throw std::runtime_error("event does not exist");
}


void SQLiteCalendarStore::deleteEvent(const EventId &id) {
    Statement statement(database_, "DELETE FROM events WHERE id = ?");
    bindText(statement.get(), 1, id);
    expectDone(database_, statement.get());
    if (sqlite3_changes(database_) != 1)
        throw std::runtime_error("event does not exist");
}
// ——— End of deleteEvent() ————————————————————————————————————————————————————

} // namespace rcal
