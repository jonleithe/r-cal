#include "SQLiteCalendarStore.h"
#include "RCalHeaderFramework.h"

#include <sqlite3.h>

#include <cassert>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>

int main() {
    const auto unique = std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const std::filesystem::path database = std::filesystem::temp_directory_path() /
        ("r-cal-store-test-" + unique + ".db");
    const std::filesystem::path migrationDatabase = std::filesystem::temp_directory_path() /
        ("r-cal-store-migration-test-" + unique + ".db");
    const rcal::Date date{2026, 11, 5};
    const rcal::Date otherDate{2026, 11, 6};
    rcal::Note first;
    rcal::Note second;
    rcal::Task task;
    rcal::Event event;

    {
        rcal::SQLiteCalendarStore store(database);
        assert(store.loadDay(date).workStatus == rcal::WorkStatus::unspecified);

        store.saveWorkStatus(date, rcal::WorkStatus::workday);
        store.saveWorkStatus(otherDate, rcal::WorkStatus::travelling);
        first = store.addNote(date, "Første notat", 0);
        second = store.addNote(date, "Andre notat", 1);
        task = store.addTask(date, "Send rapport", 0);
        event = store.addEvent(date, "09:30", "Planleggingsmøte", 0);
        const rcal::DayInfo saved = store.loadDay(date);
        assert(saved.workStatus == rcal::WorkStatus::workday);
        assert(saved.notes.size() == 2);
        assert(saved.notes[0].id == first.id);
        assert(saved.notes[0].text == "Første notat");
        assert(saved.notes[1].id == second.id);
        assert(saved.tasks.size() == 1);
        assert(saved.tasks[0].id == task.id);
        assert(!saved.tasks[0].completed);
        assert(saved.events.size() == 1);
        assert(saved.events[0].time == "09:30");

        store.updateNote(first.id, "Oppdatert notat");
        store.deleteNote(second.id);
        store.setTaskCompleted(task.id, true);
        store.updateEvent(event.id, "10:00", "Endret møte");
        store.saveWorkStatus(date, rcal::WorkStatus::unspecified);
    }

    {
        auto store = std::make_shared<rcal::SQLiteCalendarStore>(database);
        rcal::HeaderOptions options;
        options.startDate = date;
        rcal::RCalHeaderFramework header(options);
        header.setCalendarStore(store);
        assert(header.dayInfo(date)->notes[0].id == first.id);
        assert(header.infoIndicator(otherDate)[0] == 'T');
        header.toggleSelectedWorkStatus(rcal::WorkStatus::vacation);
        header.addNote(date, "Redigerbar");
        header.updateSelectedNote("Lagret fra rammeverket");

        const rcal::DayInfo saved = store->loadDay(date);
        assert(saved.workStatus == rcal::WorkStatus::vacation);
        assert(saved.notes.size() == 2);
        assert(saved.notes[1].text == "Lagret fra rammeverket");
        assert(saved.tasks[0].completed);
        assert(saved.events[0].text == "Endret møte");
    }

    {
        rcal::SQLiteCalendarStore reopened(database);
        const rcal::DayInfo saved = reopened.loadDay(date);
        assert(saved.workStatus == rcal::WorkStatus::vacation);
        assert(saved.notes.size() == 2);
        assert(saved.notes[0].id == first.id);
        assert(saved.notes[0].text == "Oppdatert notat");
        assert(saved.tasks.size() == 1 && saved.tasks[0].completed);
        assert(saved.events.size() == 1 && saved.events[0].time == "10:00");
    }

    std::filesystem::remove(database);

    sqlite3* legacyDatabase = nullptr;
    assert(sqlite3_open(migrationDatabase.string().c_str(), &legacyDatabase) == SQLITE_OK);
    assert(sqlite3_exec(legacyDatabase,
        "CREATE TABLE day_info (date TEXT PRIMARY KEY, work_status TEXT NOT NULL);"
        "CREATE TABLE notes (id TEXT PRIMARY KEY, date TEXT NOT NULL, position INTEGER NOT NULL, "
        "text TEXT NOT NULL, created_at TEXT NOT NULL, updated_at TEXT NOT NULL);"
        "CREATE INDEX notes_by_date_position ON notes(date, position);"
        "PRAGMA user_version = 1;", nullptr, nullptr, nullptr) == SQLITE_OK);
    assert(sqlite3_close(legacyDatabase) == SQLITE_OK);
    {
        rcal::SQLiteCalendarStore migrated(migrationDatabase);
        migrated.addTask(date, "Migrert oppgave", 0);
        migrated.addEvent(date, "", "Migrert hendelse", 0);
        const rcal::DayInfo saved = migrated.loadDay(date);
        assert(saved.tasks.size() == 1);
        assert(saved.events.size() == 1);
    }
    std::filesystem::remove(migrationDatabase);
    return 0;
}
