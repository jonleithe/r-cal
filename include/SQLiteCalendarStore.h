#pragma once

#include "CalendarStore.h"

#include <filesystem>

struct sqlite3;

namespace rcal {

class SQLiteCalendarStore final : public CalendarStore {
public:
    explicit SQLiteCalendarStore(const std::filesystem::path& databasePath);
    ~SQLiteCalendarStore() override;

    SQLiteCalendarStore(const SQLiteCalendarStore&) = delete;
    SQLiteCalendarStore& operator=(const SQLiteCalendarStore&) = delete;

    DayInfo loadDay(const Date& date) override;
    void saveWorkStatus(const Date& date, WorkStatus status) override;
    Note addNote(const Date& date, std::string text, std::size_t position) override;
    void updateNote(const NoteId& id, std::string text) override;
    void deleteNote(const NoteId& id) override;
    Task addTask(const Date& date, std::string text, std::size_t position,
                         std::string dueDate = "", std::string dueTime = "") override;
    void updateTask(const TaskId& id, std::string text,
                            std::string dueDate = "", std::string dueTime = "") override;
    void deleteTask(const TaskId& id) override;
    void setTaskCompleted(const TaskId& id, bool completed) override;
    Event addEvent(const Date& date, std::string time, std::string text,
                   std::size_t position, std::string startDate = "",
                           std::string endDate = "", std::string endTime = "") override;
    void updateEvent(const EventId& id, std::string time, std::string text,
                             std::string startDate = "",
                           std::string endDate = "", std::string endTime = "") override;
    void deleteEvent(const EventId& id) override;

private:
    sqlite3* database_ = nullptr;
};

} // namespace rcal
